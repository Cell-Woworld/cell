#pragma once
#include "internal/DNA.h"
#include "exprtk.hpp"
#include <math.h>
/*
template <typename T>
struct string_format : public exprtk::igeneric_function<T>
{
	typedef exprtk::igeneric_function<T> igenfunct_t;
	typedef typename igenfunct_t::generic_type generic_t;
	typedef typename igenfunct_t::parameter_list_t parameter_list_t;
	typedef typename generic_t::string_view string_t;

	string_format()
		: exprtk::igeneric_function<T>("S?*", igenfunct_t::e_rtrn_string)
	{}

	inline T operator()(std::string& result,
		parameter_list_t parameters)
	{
		result.clear();

		string_t string(parameters[0]);
		int _size = sprintf((char*)result.data(), string.begin(), parameters[1]);
		result.assign(_size + 1, 0);
		_size = sprintf((char*)result.data(), string.begin(), parameters[1]);
		return T(0);
	}
};
*/
class ActionEval
{
	static const char TAG[];
public:
	ActionEval(BioSys::DNA* owner)
		: owner_(owner)
	{
		assert(owner_ != nullptr);
	};

public:
	void Eval(const String& param);
	void Eval(const String& param, const String& target_model_name);
	bool findOperator(const String& cstrSentence, bool arithmetic_sign_only = false);

private:
	enum T_TokenType
	{
		SOURCE_VAR,
		TARGET_VAR,
		SENTENCE,
		STRING,
		INTEGER,
		DOUBLE,
		OPERATOR,
		// -> Insert a new token type here
		INVALID_TOKEN_TYPE,
	};
	enum T_Operator
	{
		EQUAL,
		NOT_EQUAL,
		GREATER,
		GREATER_EQUAL,
		LESS,
		LESS_EQUAL,
		AND,
		OR,
		AGGREGATE,
		// -> Insert a new operator here
		INVALID_OPERATOR,
	};
	T_TokenType _getToken(const String& strSentence, String& strToken, int& nParamPos);
	T_TokenType _getCalcToken(const String& strSentence, String& strToken, int& nParamPos);
	template<typename T> void Collect(const String& srcSource, T_Operator enumOperator, const T& srcTarget);
	template<typename T> bool Calculate(const String& expression_string, T& result, bool& is_integer);
	//bool Calculate(const String& szParam, double& dResult, bool& is_integer);
	bool Brackets(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult, bool& is_integer);
	bool MulDiv(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult, bool& is_integer);
	bool PlusMinus(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult, bool& is_integer);
	bool searchToken(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken);
	void RetrieveData(const String& model_name, String& target, const String& default_value="");
	template <typename T> String to_string_with_precision(const T a_value, const int n = 6);

private:
	BioSys::DNA* owner_;
};

const char ActionEval::TAG[] = "ActionEval";

ActionEval::T_TokenType ActionEval::_getToken(const String& cstrSentence, String& strToken, int& nParamPos)
{
	T_TokenType enumType = INVALID_TOKEN_TYPE;
	strToken = "";

	if (cstrSentence.size() == 0)
		return enumType;

	int nFirstPos = (int)cstrSentence.find_first_not_of(' ');
	String strSentence = cstrSentence.substr(nFirstPos);
	nParamPos += nFirstPos;
	switch (strSentence[0])
	{
	case '"':
	case '\'':
		if (strSentence.front() == strSentence.back())
		{
			//strToken = strSentence.substr(1, strSentence.size() - 2);
			strToken = strSentence;
			nParamPos += (int)strToken.size();
			enumType = STRING;
		}
		break;
	case '(':
	{
		int nCounter = 1;
		int nPos = 1;
		while ((nPos < strSentence.size()) && (nCounter > 0))
		{
			if (strSentence[nPos] == '(')
				nCounter++;
			else if (strSentence[nPos] == ')')
				nCounter--;
			nPos++;
		}
		if ((nPos >= 2) && (nCounter == 0))
		{
			enumType = SENTENCE;
			strToken = strSentence.substr(0, nPos);
		}
		else
		{
			LOG_E(TAG,  "!!!!!!!!!!  Invalid operator in _getToken(%s)  !!!!!!!!!!", strSentence.c_str());
			assert(false);
		}
		nParamPos += (int)strToken.size();
		break;
	}
	case '=':
		enumType = OPERATOR;
		strToken = strSentence.substr(0, 1);
		nParamPos += (int)strToken.size();
		break;
	case '+':
		if (strSentence.size() >= 2 && strSentence[1] == '=')
		{
			enumType = OPERATOR;
			strToken = strSentence.substr(0, 2);
			nParamPos += (int)strToken.size();
			break;
		}
	default:
	{
		int nPos = 1;
		bool bContinue = true;
		bool bEvaluatingNumber = true;
		while ((bContinue) && (nPos < strSentence.size()))
		{
			switch (strSentence[nPos])
			{
			case '&':
			case '|':
				if (nPos < strSentence.size() - 1 && strSentence[nPos + 1] == strSentence[nPos])
					bContinue = false;
				else
					nPos++;
				break;
			case '>':
			case '<':
				if (bEvaluatingNumber)
					bContinue = false;
				else
					nPos++;
				break;
			case '=':
				enumType = SOURCE_VAR;
				bContinue = false;
				break;
			case '+':
				//if (bEvaluatingNumber && nPos < strSentence.size() - 1 && strSentence[nPos + 1] == '=')
				if (nPos < strSentence.size() - 1 && strSentence[nPos + 1] == '=')		// "+=" is always a keyword and should not be ignored anytime
				{
					enumType = SOURCE_VAR;
					bContinue = false;
				}
				else {
					nPos++;
				}
				break;
			case '(':
			case ')':
				//LOG_E(TAG,  "!!!!!!!!!!  Invalid operator in _getToken(%s)  !!!!!!!!!!", strSentence.c_str());
				//assert(false);
				//break;
			default:
				if (bEvaluatingNumber && (strSentence[nPos] < '0' || strSentence[nPos] > '9') && strSentence[nPos] != ' ')
					bEvaluatingNumber = false;
				nPos++;
				break;
			}
		}
		strToken = strSentence.substr(0, nPos);
		nParamPos += (int)strToken.size();
		strToken.erase(strToken.find_last_not_of(' ') + 1);
		if (enumType != SOURCE_VAR)
		{
			if ((strToken[0] != '\"') && (strToken[0] != '\''))
			{
				bool bIsNumberType = true;
				bool bIsInteger = true;
				for (int i = 0; i < strToken.size(); i++)
				{
					if (!((strToken[i] == '.') || ((strToken[i] >= '0') && (strToken[i] <= '9'))))
					{
						bIsNumberType = false;
						break;
					}
					else if (strToken[i] == '.')
					{
						bIsInteger = false;
					}
				}
				if (bIsNumberType == true)
				{
					if (bIsInteger)
						enumType = INTEGER;
					else
						enumType = DOUBLE;
				}
				else
				{
					enumType = TARGET_VAR;
				}
			}
			else
			{
				if (strToken.front() == strToken.back())
					enumType = STRING;
				else
				{
					LOG_E(TAG,  "!!!!!!!!!!  Invalid operator in _getToken(%s)  !!!!!!!!!!", strSentence.c_str());
					assert(false);
				}
			}
		}
		break;
	}
	}
	return enumType;
}

ActionEval::T_TokenType ActionEval::_getCalcToken(const String& cstrSentence, String& strToken, int& nParamPos)
{
	T_TokenType enumType = INVALID_TOKEN_TYPE;
	strToken = "";

	if (cstrSentence.size() == 0)
		return enumType;

	int nFirstPos = (int)cstrSentence.find_first_not_of(' ');
	String strSentence = cstrSentence.substr(nFirstPos);
	nParamPos += nFirstPos;
	switch (strSentence[0])
	{
	case '(':
	{
		int nCounter = 1;
		int nPos = 1;
		while ((nPos < strSentence.size()) && (nCounter > 0))
		{
			if (strSentence[nPos] == '(')
				nCounter++;
			else if (strSentence[nPos] == ')')
				nCounter--;
			nPos++;
		}
		if ((nPos >= 2) && (nCounter == 0))
		{
			enumType = SENTENCE;
			strToken = strSentence.substr(0, nPos);
		}
		else
		{
			LOG_E(TAG,  "!!!!!!!!!!  Invalid operator in _getCalcToken(%s)  !!!!!!!!!!", strSentence.c_str());
			assert(false);
		}
		nParamPos += (int)strToken.size();
		break;
	}
	case '+':
	case '-':
	case '*':
	case '/':
	case '&':
	case '|':
		enumType = OPERATOR;
		strToken = strSentence.substr(0, 1);
		nParamPos += (int)strToken.size();
		break;
	case '>':
	case '<':
		if (strSentence[1] == strSentence[0])
		{
			enumType = OPERATOR;
			strToken = strSentence.substr(0, 2);
		}
		nParamPos += (int)strToken.size();
		break;
	default:
	{
		int nPos = 1;
		bool bContinue = true;
		while ((bContinue) && (nPos < strSentence.size()))
		{
			switch (strSentence[nPos])
			{
			case '+':
			case '-':
			case '*':
			case '/':
			case '&':
			case '|':
			case '>':
			case '<':
				enumType = SOURCE_VAR;
				bContinue = false;
				break;
			case '(':
			case ')':
				//LOG_E(TAG,  "!!!!!!!!!!  Invalid operator in _getCalcToken(%s)  !!!!!!!!!!", strSentence.c_str());
				//assert(false);
				//break;
			default:
				nPos++;
				break;
			}
		}
		strToken = strSentence.substr(0, nPos);
		nParamPos += (int)strToken.size();
		strToken.erase(strToken.find_last_not_of(' ') + 1);
		if (enumType != SOURCE_VAR)
		{
			if ((strToken[0] != '\"') && (strToken[0] != '\''))
			{
				bool bIsNumberType = true;
				bool bIsInteger = true;
				for (int i = 0; i < strToken.size(); i++)
				{
					if (!((strToken[i] == '.') || ((strToken[i] >= '0') && (strToken[i] <= '9'))))
					{
						bIsNumberType = false;
						break;
					}
					else if (strToken[i] == '.')
					{
						bIsInteger = false;
					}
				}
				if (bIsNumberType == true)
				{
					if (bIsInteger)
						enumType = INTEGER;
					else
						enumType = DOUBLE;
				}
				else
				{
					enumType = TARGET_VAR;
				}
			}
			else
			{
				if (strToken.front() == strToken.back())
					enumType = STRING;
				else
				{
					LOG_E(TAG,  "!!!!!!!!!!  Invalid operator in _getCalcToken(%s)  !!!!!!!!!!", strSentence.c_str());
					assert(false);
				}
			}
		}
		break;
	}
	}
	return enumType;
}

template <typename T>
bool ActionEval::Calculate(const String& expression_string, T& result, bool& is_integer)
{
	exprtk::symbol_table<T> symbol_table;
	symbol_table.add_constants();
	//string_format<T> stringFormat;
	//symbol_table.add_function("string_format", stringFormat);
	exprtk::expression<T> expression;
	expression.register_symbol_table(symbol_table);

	exprtk::parser<T> parser;
	try {
		
		if (!parser.compile(expression_string, expression))
		{
			LOG_T(TAG, "Calculate() - %s   Expression: %s",
				parser.error().c_str(),
				expression_string.c_str());

			return false;
		}
	}
	catch (const std::exception&) {
		LOG_E(TAG, "Calculate() - %s   Expression: %s",
			parser.error().c_str(),
			expression_string.c_str());

		return false;
	}

	if (!exprtk::expression_helper<T>::is_constant(expression))
	{
		LOG_T(TAG, "Calculate() - Expression did not compile to a constant!   Expression: %s",
			expression_string.c_str());

		return false;
	}
	if (expression_string.front() == '[' && expression_string.back() == ']')
	{
		return false;
	}
	else if (expression_string == "true" || expression_string == "false")
	{
		return false;
	}
	else
	{
		result = expression.value();
		int _decimal_count = 0;
		owner_->Read("Bio.Cell.Model.NumberWithDecimalCount", _decimal_count);
		is_integer = (_decimal_count <= 0);
		return true;
	}
}
/*
bool ActionEval::Calculate(const String& szParam, double& dResult, bool& is_integer)
{
	dResult = 0.0;
	int nPos = 0;
	T_TokenType _enumTokenType;
	String strToken;
	if (searchToken(szParam, nPos, _enumTokenType, strToken) == true)
		return PlusMinus(szParam, nPos, _enumTokenType, strToken, dResult, is_integer);
	else
		return false;
}
*/
bool ActionEval::Brackets(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult, bool& is_integer)
{
	switch (enumTokenType)
	{
	case SOURCE_VAR:
	case INTEGER:
	case DOUBLE:
	{
		try
		{
			String _result = "";
			RetrieveData(strToken, _result, strToken);
			dResult = stod(_result);
		}
		catch (...)
		{
			dResult = 0.0;
		}
		searchToken(strSentence, nPos, enumTokenType, strToken);
		return true;
	}
	case OPERATOR:
		if (strToken == "-")
		{
			searchToken(strSentence, nPos, enumTokenType, strToken);
			if (Brackets(strSentence, nPos, enumTokenType, strToken, dResult, is_integer) == true)
				dResult = -dResult;
		}
	case SENTENCE:
	{
		int nNewPos = 0;
		T_TokenType enumNewTokenType;
		String strNewToken;
		searchToken(strToken, nNewPos, enumNewTokenType, strNewToken);
		bool _is_integer = true;
		if (PlusMinus(strToken, nNewPos, enumNewTokenType, strNewToken, dResult, _is_integer) == false)
			return false;
		is_integer &= _is_integer;
		searchToken(strSentence, nPos, enumTokenType, strToken);
		return true;
	}
	case INVALID_TOKEN_TYPE:
		dResult = 0.0;
		return true;
	default:
		break;
	}
	return false;
}

bool ActionEval::MulDiv(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult, bool& is_integer)
{
	if (Brackets(strSentence, nPos, enumTokenType, strToken, dResult, is_integer) == false)
		return false;
	while (strToken == "*" || strToken == "/")
	{
		double Value = 0.0;
		if (strToken == "*")
		{
			searchToken(strSentence, nPos, enumTokenType, strToken);
			if (strToken != "")
			{
				if (enumTokenType == DOUBLE)
					is_integer = false;
				if (Brackets(strSentence, nPos, enumTokenType, strToken, Value, is_integer) == false)
					return false;
				dResult *= Value;
			}
		}
		else if (strToken == "/")
		{
			searchToken(strSentence, nPos, enumTokenType, strToken);
			if (strToken != "")
			{
				if (enumTokenType == DOUBLE)
					is_integer = false;
				if (Brackets(strSentence, nPos, enumTokenType, strToken, Value, is_integer) == false)
					return false;
				dResult /= Value;
			}
		}
	}
	return true;
}

bool ActionEval::PlusMinus(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult, bool& is_integer)
{
	if (MulDiv(strSentence, nPos, enumTokenType, strToken, dResult, is_integer) == false)
		return false;
	while (strToken == "+" || strToken == "-")
	{
		double Value = 0.0;
		if (strToken == "+")
		{
			searchToken(strSentence, nPos, enumTokenType, strToken);
			if (enumTokenType == OPERATOR && strToken == "+")
			{
				dResult += 1.0;
			}
			else if (strToken != "")
			{
				if (enumTokenType == DOUBLE)
					is_integer = false;
				if (MulDiv(strSentence, nPos, enumTokenType, strToken, Value, is_integer) == false)
					return false;
				dResult += Value;
			}
		}
		else if (strToken == "-")
		{
			searchToken(strSentence, nPos, enumTokenType, strToken);
			if (enumTokenType == OPERATOR && strToken == "-")
			{
				dResult -= 1.0;
			}
			else if (strToken != "")
			{
				if (enumTokenType == DOUBLE)
					is_integer = false;
				if (MulDiv(strSentence, nPos, enumTokenType, strToken, Value, is_integer) == false)
					return false;
				dResult -= Value;
			}
		}
	}
	return true;
}

bool ActionEval::searchToken(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken)
{
	bool bRet = false;
	if (strSentence.empty() || strSentence.front() == '[' && strSentence.back() == ']'
		|| strSentence.front() == '{' && strSentence.back() == '}')
	{
		return false;				// empty or JSON format, ignored
	}
	strToken = "";
	enumTokenType = INVALID_TOKEN_TYPE;
	if ((enumTokenType = _getCalcToken(strSentence.substr(nPos), strToken, nPos)) != INVALID_TOKEN_TYPE)
	{
		switch (enumTokenType)
		{
		case SOURCE_VAR:
		case TARGET_VAR:
		{
			String strOperand = "";
			RetrieveData(strToken, strOperand, strToken);
			//strToken = strOperand;
			bRet = true;
			//if (strOperand != "")
			//{	// it must be number
			//	for (int i = 0; i < strOperand.size(); i++)
			//	{
			//		if (!((strOperand[i] == '.') || ((strOperand[i] >= '0') && (strOperand[i] <= '9'))))
			//		{
			//			return false;
			//		}
			//	}
			//	strToken = strOperand;
			//	bRet = true;
			//}
			//else
			//{
			//	//LOG_E(TAG,  "!!!!!!!!!!  Invalid operator in searchToken(%s)  !!!!!!!!!!", strSentence.c_str());
			//	//assert(false);
			//	return false;
			//}
		}
		break;
		case SENTENCE:
			strToken = strToken.substr(1, strToken.size() - 2);
			bRet = true;
			break;
		case STRING:
			LOG_E(TAG,  "!!!!!!!!!!  Invalid operator in searchToken(%s)  !!!!!!!!!!", strSentence.c_str());
			assert(false);
			return false;
			break;
		case INTEGER:
		case DOUBLE:
			bRet = true;
			break;
		case OPERATOR:
			break;
		default:
			LOG_E(TAG, "!!!!!!!!!!  Invalid operator in searchToken(%s)  !!!!!!!!!!", strSentence.c_str());
			assert(false);
			return false;
		}
	};
	return bRet;
}

void ActionEval::Eval(const String& param)
{
	String strToken = "";
	T_TokenType enumType = INVALID_TOKEN_TYPE;
	int nPos = 0;
	String strOperand = "";
	T_Operator enumOperator = INVALID_OPERATOR;
	while ((enumType = _getToken(param.substr(nPos), strToken, nPos)) != INVALID_TOKEN_TYPE)
	{
		switch (enumType)
		{
		case SOURCE_VAR:
			//double dOperand;
			//if (Calculate(strToken, dOperand) == true)
			//{
			//	strOperand = std::to_string(dOperand);
			//}
			//else
			{
				//strOperand = strToken;
				//strOperand = "";
				//RetrieveData(strToken, strOperand, strToken);
				//if (strOperand != "")
				//	bRet = true;
				//else
				//{
				//	LOG_E(TAG, "!!!!!!!!!!  Invalid operator in C_Eval(%s)  !!!!!!!!!!", szParam);
				//	assert(false);
				//	return false;
				//}
			}
			strOperand = strToken;
			break;
		case TARGET_VAR:
		{
			double dTargetOperand;
			bool _is_integer = true;
			if (Calculate(strToken, dTargetOperand, _is_integer) == true)
			{
				if (strOperand == "")	// it must be ++ or --
				{
					int _pos = 0;
					_getCalcToken(strToken, strOperand, _pos);
					enumOperator = EQUAL;
				}
				assert(strOperand != "");
				if (_is_integer == true)
					Collect(strOperand, enumOperator, (long long)round(dTargetOperand));
				else
					Collect(strOperand, enumOperator, dTargetOperand);
			}
			else
			{
				String strTargetOperand = "";
				RetrieveData(strToken, strTargetOperand, strToken);
				//strTargetOperand = std::regex_replace(strTargetOperand, std::regex("\\n"), "\n");
				size_t _pos = String::npos;
				while ((_pos = strTargetOperand.find("\\r\\n")) != String::npos)
				{
					strTargetOperand.replace(_pos, sizeof("\\r\\n") - 1, "\r\n");
				}
				Collect(strOperand, enumOperator, strTargetOperand);
			}
		}
		break;
		case SENTENCE:
			strToken = strToken.substr(1, strToken.size() - 2);
			switch (enumOperator)
			{
			case INVALID_OPERATOR:
				Eval(strToken);
				break;
			case EQUAL:
			case AGGREGATE:
			{
				double dTargetOperand;
				bool _is_integer = true;
				if (Calculate(strToken, dTargetOperand, _is_integer) == true)
				{
					if (strOperand == "")	// it must be ++ or --
					{
						int _pos = 0;
						_getCalcToken(strToken, strOperand, _pos);
						enumOperator = EQUAL;
					}
					assert(strOperand != "");
					if (_is_integer == true)
						Collect(strOperand, enumOperator, (long long)round(dTargetOperand));
					else
						Collect(strOperand, enumOperator, dTargetOperand);
				}
				else
				{
					String strTargetOperand = "";
					RetrieveData(strToken, strTargetOperand, strToken);
					//strTargetOperand = std::regex_replace(strTargetOperand, std::regex("\\n"), "\n");
					size_t _pos = String::npos;
					while ((_pos = strTargetOperand.find("\\r\\n")) != String::npos)
					{
						strTargetOperand.replace(_pos, sizeof("\\r\\n") - 1, "\r\n");
					}
					Collect(strOperand, enumOperator, strTargetOperand);
				}
				break;
			}
			default:
				LOG_E(TAG, "!!!!!!!!!!  Invalid operator in ActionEval(%s), adding the outer bracket may solve this problem !!!!!!!!!!", param.c_str());
				assert(false);
				break;
			}
			break;
		case STRING:
		{
			//assert(strOperand != "");
			strToken = strToken.substr(1, strToken.size() - 2);
			//strToken = std::regex_replace(strToken, std::regex("\\n"), "\n");
			size_t _pos = String::npos;
			while ((_pos = strToken.find("\\r\\n")) != String::npos)
			{
				strToken.replace(_pos, sizeof("\\r\\n") - 1, "\r\n");
			}
			Collect(strOperand, enumOperator, strToken);
			break;
		}
		case INTEGER:
			assert(strOperand != "");
			Collect(strOperand, enumOperator, stoi(strToken));
			break;
		case DOUBLE:
			assert(strOperand != "");
			Collect(strOperand, enumOperator, stod(strToken));
			break;
		case OPERATOR:
			if (strToken == "=")
				enumOperator = EQUAL;
			else if (strToken == "+=")
				enumOperator = AGGREGATE;
			else
			{
				LOG_E(TAG, "!!!!!!!!!!  Invalid operator in ActionEval(%s)  !!!!!!!!!!", param.c_str());
				assert(false);
			}
			break;
		default:
			LOG_E(TAG, "!!!!!!!!!!  Invalid operator in ActionEval(%s)  !!!!!!!!!!", param.c_str());
			assert(false);
		}
	};
}

void ActionEval::Eval(const String& param, const String& target_model_name)
{
	double dTargetOperand;
	bool _is_integer = true;
	if (Calculate(param, dTargetOperand, _is_integer) == true)
	{
		if (_is_integer == true)
			owner_->Write(target_model_name, (long long)round(dTargetOperand));
		else
		{
			int _decimal_count = 0;
			owner_->Read("Bio.Cell.Model.NumberWithDecimalCount", _decimal_count);
			owner_->Write(target_model_name, to_string_with_precision(dTargetOperand, _decimal_count));
		}
	}
	else
	{
		size_t nFirstPos = (int)param.find_first_not_of(' ');
		if (nFirstPos == String::npos)
			return;
		String strSentence = param.substr(nFirstPos);
		if (strSentence[0] == '"' || strSentence[0] == '\'')
		{
			if (strSentence.front() == strSentence.back())
			{
				strSentence = strSentence.substr(1, strSentence.size() - 2);
			}
		};
		String strTargetOperand = "";
		RetrieveData(strSentence, strTargetOperand, strSentence);
		//strTargetOperand = std::regex_replace(strTargetOperand, std::regex("\\n"), "\n");
		size_t _pos = String::npos;
		while ((_pos = strTargetOperand.find("\\r\\n")) != String::npos)
		{
			strTargetOperand.replace(_pos, sizeof("\\r\\n") - 1, "\r\n");
		}
		owner_->Write(target_model_name, strTargetOperand);
	}
}

template<typename T>
void ActionEval::Collect(const String& srcSource, T_Operator enumOperator, const T& srcTarget)
{
	switch (enumOperator)
	{
	case EQUAL:
		if (srcSource[0]==':' && srcSource[1] == ':')
			owner_->Write(srcSource.substr(2), srcTarget);
		else
			owner_->Write(srcSource, srcTarget);
		break;
	case AGGREGATE:
	{
		String _srcSource = srcSource;
		if (srcSource[0] == ':' && srcSource[1] == ':')
			_srcSource = srcSource.substr(2);
		Array<T> _list;
		owner_->Read(_srcSource, _list);
		_list.push_back(srcTarget);
		owner_->Write(_srcSource, _list);
		break;
	}
	case INVALID_OPERATOR:
		LOG_E(TAG, "!!!!!!!!!! BioUnit::Collect() Invalid operator in ActionEval()  !!!!!!!!!!");
		assert(false);
	default:
		LOG_W("DNA", "Warning BioUnit::Collect() Unsupported operator in ActionEval()  !!!!!!!!!!");
		break;
	}
}

bool ActionEval::findOperator(const String& cstrSentence, bool arithmetic_sign_only)
{
	T_TokenType enumType = INVALID_TOKEN_TYPE;

	if (cstrSentence.size() == 0)
		return false;

	for (int i = 0; i < cstrSentence.size() && enumType != OPERATOR; i++)
	{
		switch (cstrSentence[i])
		{
		case '&':
		case '|':
		case '=':
		case '>':
		case '<':
			if (arithmetic_sign_only == false)
				enumType = OPERATOR;
			break;
		case '+':
		case '-':
		case '*':
		case '/':
			enumType = OPERATOR;
			break;
		case '\0':
			return false;
		default:
			break;
		}
	}

	return (enumType == OPERATOR && cstrSentence.find('\0') == String::npos);
}

void ActionEval::RetrieveData(const String& model_name, String& target, const String& default_value)
{
	if (model_name[0] == ':' && model_name[1] == ':')
	{
		if (model_name.find('[') == String::npos)
		{ 
			owner_->Read<String>(model_name.substr(2), target);
			if (target == "")
				target = default_value;
		}
		else
		{
			size_t _start_pos = model_name.find('[');
			size_t _end_pos = model_name.find(']', _start_pos);
			if (_end_pos != String::npos)
			{
				if (_end_pos - _start_pos == 1)
				{
					String _model_name = "";
					Array<String> _operand;
					owner_->Read(model_name.substr(2, _start_pos - 2), _operand);
					if (model_name.substr(_end_pos + 1, sizeof(".size")-1) == ".size" || model_name.substr(_end_pos + 1, sizeof(".length")-1) == ".length")
					{
						target = std::to_string(_operand.size());
					}
				}
				else
				{
					Array<String> _operand;
					owner_->Read(model_name.substr(2, _start_pos - 2), _operand);
					int _index = stoi(model_name.substr(_start_pos + 1, _end_pos - _start_pos - 1));
					if (_index < _operand.size())
						target = _operand[_index];
				}
			}
		}
	}
	else
	{
		target = default_value;
	}
}

template <typename T>
String ActionEval::to_string_with_precision(const T a_value, const int n)
{
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return out.str();
}
