#pragma once
#include "internal/DNA.h"

class ConditionEval
{
	static const char EQUAL_UTF8[];
	static const char NOT_EQUAL_UTF8[];
	static const char GREATER_UTF8[];
	static const char LESS_UTF8[];
	static const char GREATER_EQUAL_UTF8[];
	static const char LESS_EQUAL_UTF8[];
	static const char PLUS_UTF8[];
	static const char MINUS_UTF8[];
	static const char PRODUCT_UTF8[];
	static const char DIVID_UTF8[];

public:
	ConditionEval(BioSys::DNA* owner)
		: owner_(owner)
	{
		assert(owner_ != nullptr);
	};

public:
	bool Eval(const String& param, bool fill_default_model_name = true);
	bool findOperator(const String& cstrSentence);

private:
	enum T_TokenType
	{
		SOURCE_VAR,
		TARGET_VAR,
		SENTENCE,
		STRING,
		NUMBER,
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
		// -> Insert a new operator here
		INVALID_OPERATOR,
	};
	T_TokenType _getToken(const String& strSentence, String& strToken, int& nParamPos);
	T_TokenType _getCalcToken(const String& strSentence, String& strToken, int& nParamPos);
	template<typename T> bool Compare(const T& srcSource, T_Operator enumOperator, const T& srcTarget);
	bool Calculate(const String& szParam, double& dResult);
	bool Brackets(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult);
	bool MulDiv(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult);
	bool PlusMinus(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult);
	bool searchToken(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken);
	void RetrieveData(const String& src, String& target, const String& default_value = "");
	bool IsNumberType(const String& strToken);
	void CompareSourceTarget(const String& strSource, T_Operator enumOperator, const String& strTarget, bool& bRet);

private:
	BioSys::DNA* owner_;
};

const char ConditionEval::EQUAL_UTF8[] = { (char)0xef,(char)0xbc,(char)0x9d, (char)0x0 };
const char ConditionEval::GREATER_UTF8[] = { (char)0xef,(char)0xbc,(char)0x9e, (char)0x0 };
const char ConditionEval::LESS_UTF8[] = { (char)0xef,(char)0xbc,(char)0x9c, (char)0x0 };
const char ConditionEval::PLUS_UTF8[] = { (char)0xef,(char)0xbc,(char)0x8b, (char)0x0 };
const char ConditionEval::MINUS_UTF8[] = { (char)0xef,(char)0xbc,(char)0x8d, (char)0x0 };
const char ConditionEval::NOT_EQUAL_UTF8[] = { (char)0xe2,(char)0x89,(char)0xa0, (char)0x0 };
const char ConditionEval::GREATER_EQUAL_UTF8[] = { (char)0xe2,(char)0x89,(char)0xa7, (char)0x0 };
const char ConditionEval::LESS_EQUAL_UTF8[] = { (char)0xe2,(char)0x89,(char)0xa6, (char)0x0 };
const char ConditionEval::PRODUCT_UTF8[] = { (char)0xc3,(char)0x97, (char)0x0 };
const char ConditionEval::DIVID_UTF8[] = { (char)0xc3,(char)0xb7, (char)0x0 };

ConditionEval::T_TokenType ConditionEval::_getToken(const String& cstrSentence, String& strToken, int& nParamPos)
{
	T_TokenType enumType = INVALID_TOKEN_TYPE;
	strToken = "";

	if (cstrSentence.size() == 0)
		return enumType;

	size_t nFirstPos = cstrSentence.find_first_not_of(' ');
	if (nFirstPos == String::npos)
	{
		return enumType;
	}
	String strSentence = cstrSentence.substr(nFirstPos);
	nParamPos += (int)nFirstPos;
	bool _treat_as_string = false;
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
			LOG_E("DNA",  "!!!!!!!!!!  Invalid operator in _getToken(%s)  !!!!!!!!!!", strSentence.c_str());
			assert(false);
		}
		nParamPos += (int)strToken.size();
		break;
	}
	case '&':
	case '|':
		if (strSentence[1] == strSentence[0])
		{
			enumType = OPERATOR;
			strToken = strSentence.substr(0, 2);
		}
		nParamPos += (int)strToken.size();
		break;
	case '=':
		enumType = OPERATOR;
		if (strSentence.size() >= 2 && strSentence[1] == '=')
			strToken = strSentence.substr(0, 2);
		else
			strToken = strSentence.substr(0, 1);
		nParamPos += (int)strToken.size();
		break;
	case '!':
		if (strSentence.size() >= 2 && strSentence[1] == '=')
		{
			enumType = OPERATOR;
			strToken = strSentence.substr(0, 2);
		}
		nParamPos += (int)strToken.size();
		break;
	case '>':
		enumType = OPERATOR;
		if (strSentence.size() >= 2 && strSentence[1] == '=')
			strToken = strSentence.substr(0, 2);
		else
			strToken = strSentence.substr(0, 1);
		nParamPos += (int)strToken.size();
		break;
	case '<':
		enumType = OPERATOR;
		if (strSentence.size() >= 2 && (strSentence[1] == '=' || strSentence[1] == '>'))
			strToken = strSentence.substr(0, 2);
		else
			strToken = strSentence.substr(0, 1);
		nParamPos += (int)strToken.size();
		break;
	case (char)0xc3:
		if ((strSentence[1] == PRODUCT_UTF8[1])
			|| (strSentence[1] == DIVID_UTF8[1]))
		{
			enumType = OPERATOR;
			strToken = strSentence.substr(0, 2);
			nParamPos += (int)strToken.size();
		}
		break;
	case (char)0xe2:
		if ((strSentence[1] == NOT_EQUAL_UTF8[1] && strSentence[2] == NOT_EQUAL_UTF8[2])
			|| (strSentence[1] == GREATER_EQUAL_UTF8[1] && strSentence[2] == GREATER_EQUAL_UTF8[2])
			|| (strSentence[1] == LESS_EQUAL_UTF8[1] && strSentence[2] == LESS_EQUAL_UTF8[2]))
		{
			enumType = OPERATOR;
			strToken = strSentence.substr(0, 3);
			nParamPos += (int)strToken.size();
		}
		break;
	case (char)0xef:
		if ((strSentence[1] == EQUAL_UTF8[1] && strSentence[2] == EQUAL_UTF8[2])
			|| (strSentence[1] == GREATER_UTF8[1] && strSentence[2] == GREATER_UTF8[2])
			|| (strSentence[1] == LESS_UTF8[1] && strSentence[2] == LESS_UTF8[2])
			|| (strSentence[1] == PLUS_UTF8[1] && strSentence[2] == PLUS_UTF8[2])
			|| (strSentence[1] == MINUS_UTF8[1] && strSentence[2] == MINUS_UTF8[2]))
		{
			enumType = OPERATOR;
			strToken = strSentence.substr(0, 3);
			nParamPos += (int)strToken.size();
		}
		break;
	case '"':
		_treat_as_string = true;
	default:
	{
		int nPos = 1;
		bool bContinue = true;
		char _prev_char = '\0';
		while ((bContinue) && (nPos < strSentence.size()))
		{
			switch (strSentence[nPos])
			{
			case '&':
			case '|':
				if ((_treat_as_string == false || _prev_char == '"') && nPos < strSentence.size() - 1 && strSentence[nPos + 1] == strSentence[nPos])
					bContinue = false;
				else
				{
					_prev_char = strSentence[nPos];
					nPos++;
				}
				break;
			case '=':
			case '!':
			case '>':
			case '<':
			case (char)0xe2:
				if (_treat_as_string == false || _prev_char == '"')
				{
					enumType = SOURCE_VAR;
					bContinue = false;
				}
				else
				{
					_prev_char = strSentence[nPos];
					nPos++;
				}
				break;
			case '(':
			case ')':
				//LOG_E("DNA",  "!!!!!!!!!!  Invalid operator in _getToken(%s)  !!!!!!!!!!", strSentence.c_str());
				//assert(false);
				//break;
			case ' ':
				nPos++;
				break;
			default:
				_prev_char = strSentence[nPos];
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
				for (int i = 0; i < strToken.size(); i++)
				{
					if (!((strToken[i] == '.') || ((strToken[i] >= '0') && (strToken[i] <= '9'))))
					{
						bIsNumberType = false;
						break;
					}
				}
				if (bIsNumberType == true)
					enumType = NUMBER;
				else
					enumType = TARGET_VAR;
			}
			else
			{
				if (strToken.front() == strToken.back())
					enumType = STRING;
				else
				{
					LOG_E("DNA",  "!!!!!!!!!!  Invalid operator in _getToken(%s)  !!!!!!!!!!", strSentence.c_str());
					assert(false);
				}
			}
		}
		break;
	}
	}
	return enumType;
}

ConditionEval::T_TokenType ConditionEval::_getCalcToken(const String& cstrSentence, String& strToken, int& nParamPos)
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
			LOG_E("DNA",  "!!!!!!!!!!  Invalid operator in _getCalcToken(%s)  !!!!!!!!!!", strSentence.c_str());
			assert(false);
		}
		nParamPos += (int)strToken.size();
		break;
	}
	case '+':
	case '-':
	case '*':
	case '/':
		enumType = OPERATOR;
		strToken = strSentence.substr(0, 1);
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
				strToken = strSentence.substr(0, nPos);
				strToken.erase(strToken.find_last_not_of(' ') + 1);
				if (IsNumberType(strToken) == true)
				{
					enumType = SOURCE_VAR;
					bContinue = false;
				}
				else
				{
					nPos++;
				}
				break;
			case '(':
			case ')':
				//LOG_E("DNA",  "!!!!!!!!!!  Invalid operator in _getCalcToken(%s)  !!!!!!!!!!", strSentence.c_str());
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
				if (IsNumberType(strToken) == true)
					enumType = NUMBER;
				else
					enumType = TARGET_VAR;
			}
			else
			{
				if (strToken.front() == strToken.back())
					enumType = STRING;
				else
				{
					LOG_E("DNA",  "!!!!!!!!!!  Invalid operator in _getCalcToken(%s)  !!!!!!!!!!", strSentence.c_str());
					assert(false);
				}
			}
		}
		break;
	}
	}
	return enumType;
}

bool ConditionEval::Calculate(const String& szParam, double& dResult)
{
	dResult = 0.0;
	int nPos = 0;
	T_TokenType enumTokenType;
	String strToken;
	if (searchToken(szParam, nPos, enumTokenType, strToken) == true)
		return PlusMinus(szParam, nPos, enumTokenType, strToken, dResult);
	else
		return false;
}

bool ConditionEval::Brackets(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult)
{
	switch (enumTokenType)
	{
	case SOURCE_VAR:
	case NUMBER:
	{
		try
		{
			dResult = stod(strToken);
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
			if (Brackets(strSentence, nPos, enumTokenType, strToken, dResult) == true)
				dResult = -dResult;
		}
	case SENTENCE:
	{
		int nNewPos = 0;
		T_TokenType enumNewTokenType;
		String strNewToken;
		searchToken(strToken, nNewPos, enumNewTokenType, strNewToken);
		if (PlusMinus(strToken, nNewPos, enumNewTokenType, strNewToken, dResult) == false)
			return false;
		searchToken(strSentence, nPos, enumTokenType, strToken);
		return true;
	}
	case INVALID_TOKEN_TYPE:
		dResult = 1;
		return true;
	default:
		break;
	}
	return false;
}

bool ConditionEval::MulDiv(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult)
{
	if (Brackets(strSentence, nPos, enumTokenType, strToken, dResult) == false)
		return false;
	while (strToken == "*" || strToken == "/")
	{
		double Value = 0.0;
		if (strToken == "*")
		{
			searchToken(strSentence, nPos, enumTokenType, strToken);
			if (Brackets(strSentence, nPos, enumTokenType, strToken, Value) == false)
				return false;
			dResult *= Value;
		}
		else if (strToken == "/")
		{
			searchToken(strSentence, nPos, enumTokenType, strToken);
			if (Brackets(strSentence, nPos, enumTokenType, strToken, Value) == false)
				return false;
			dResult /= Value;
		}
	}
	return true;
}

bool ConditionEval::PlusMinus(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken, double& dResult)
{
	if (MulDiv(strSentence, nPos, enumTokenType, strToken, dResult) == false)
		return false;
	while (strToken == "+" || strToken == "-")
	{
		double Value = 0.0;
		if (strToken == "+")
		{
			searchToken(strSentence, nPos, enumTokenType, strToken);
			if (MulDiv(strSentence, nPos, enumTokenType, strToken, Value) == false)
				return false;
			dResult += Value;
		}
		else if (strToken == "-")
		{
			searchToken(strSentence, nPos, enumTokenType, strToken);
			if (MulDiv(strSentence, nPos, enumTokenType, strToken, Value) == false)
				return false;
			dResult -= Value;
		}
	}
	return true;
}

bool ConditionEval::searchToken(const String& strSentence, int& nPos, T_TokenType& enumTokenType, String& strToken)
{
	bool bRet = false;

	strToken = "";
	enumTokenType = INVALID_TOKEN_TYPE;
	if ((enumTokenType = _getCalcToken(strSentence.substr(nPos), strToken, nPos)) != INVALID_TOKEN_TYPE)
	{
		switch (enumTokenType)
		{
		case SOURCE_VAR:
		case TARGET_VAR:
		{
			String strOperand = strToken;
			RetrieveData(strToken, strOperand, strToken);
			strToken = strOperand;
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
			//	//LOG_E("DNA",  "!!!!!!!!!!  Invalid operator in searchToken(%s)  !!!!!!!!!!", strSentence.c_str());
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
			//LOG_E("DNA",  "!!!!!!!!!!  Invalid operator in searchToken(%s)  !!!!!!!!!!", strSentence.c_str());
			//assert(false);
			//return false;
			bRet = false;
			break;
		case NUMBER:
			bRet = true;
			break;
		case OPERATOR:
			break;
		default:
			LOG_E("DNA", "!!!!!!!!!!  Invalid operator in searchToken(%s)  !!!!!!!!!!", strSentence.c_str());
			assert(false);
			return false;
		}
	};
	return bRet;
}

bool ConditionEval::Eval(const String& param, bool fill_default_model_name)
{
	//bool bRet = false;
	bool bRet = true;
	String strToken = "";
	T_TokenType enumType = INVALID_TOKEN_TYPE;
	T_TokenType enumLastType = INVALID_TOKEN_TYPE;
	int nPos = 0;
	String strOperand = "";
	T_Operator enumOperator = INVALID_OPERATOR;
	while ((enumType = _getToken(param.substr(nPos), strToken, nPos)) != INVALID_TOKEN_TYPE)
	{
		enumLastType = enumType;
		switch (enumType)
		{
		case SOURCE_VAR:
			double dOperand;
			if (Calculate(strToken, dOperand) == true)
			{
				strOperand = std::to_string(dOperand);
			}
			else
			{
				//strOperand = strToken;
				strOperand = "";
				if (fill_default_model_name)
					RetrieveData(strToken, strOperand, strToken);
				else
					RetrieveData(strToken, strOperand);
				//if (strOperand != "")
				//	bRet = true;
				//else
				//{
				//	LOG_E("DNA", "!!!!!!!!!!  Invalid operator in C_Eval(%s)  !!!!!!!!!!", szParam);
				//	assert(false);
				//	return false;
				//}
			}
			break;
		case TARGET_VAR:
			CompareSourceTarget(strOperand, enumOperator, strToken, bRet);
			break;
		case SENTENCE:
			strToken = strToken.substr(1, strToken.size() - 2);
			switch (enumOperator)
			{
			case INVALID_OPERATOR:
			case OR:
				bRet = Eval(strToken, fill_default_model_name);
				break;
			case AND:
				if (bRet == true)
					bRet &= Eval(strToken, fill_default_model_name);
				break;
			default:
				//LOG_E("DNA", "!!!!!!!!!!  Invalid operator in C_Eval(%s)  !!!!!!!!!!", szParam);
				//assert(false);
				CompareSourceTarget(strOperand, enumOperator, strToken, bRet);
				break;
			}
			break;
		case STRING:
			//assert(strOperand != "");
			strToken = strToken.substr(1, strToken.size() - 2);
			if (strOperand.size() >=2 && (strOperand.front() == '\"' || strOperand.front() == '\'') && strOperand.front() == strOperand.back())
				strOperand = strOperand.substr(1, strOperand.size() - 2);
			bRet &= Compare(strOperand, enumOperator, strToken);
			break;
		case NUMBER:
			if (strOperand == "")
			{
				if (enumOperator == INVALID_OPERATOR)
				{
					strOperand = "1.0";
					enumOperator = EQUAL;
				}
				else
				{
					strOperand = "0.0";
				}
			}
			try
			{
				bRet &= Compare(stod(strOperand), enumOperator, stod(strToken));
			}
			catch (...)
			{
				bRet = false;
			}
			break;
		case OPERATOR:
			if (strToken == "==" || strToken == "=" || strToken == EQUAL_UTF8)
				enumOperator = EQUAL;
			else if (strToken == "!=" || strToken == "<>" || strToken == NOT_EQUAL_UTF8)
				enumOperator = NOT_EQUAL;
			else if (strToken == ">" || strToken == GREATER_UTF8)
				enumOperator = GREATER;
			else if (strToken == ">=" || strToken == GREATER_EQUAL_UTF8)
				enumOperator = GREATER_EQUAL;
			else if (strToken == "<" || strToken == LESS_UTF8)
				enumOperator = LESS;
			else if (strToken == "<=" || strToken == LESS_EQUAL_UTF8)
				enumOperator = LESS_EQUAL;
			else if (strToken == "&&")
			{
				if (bRet == false)
					return false;
				else
				{
					enumOperator = AND;
					strOperand = "";
				}
			}
			else if (strToken == "||")
			{
				if (bRet == true)
					return true;
				else
				{
					bRet = true;
					enumOperator = OR;
					strOperand = "";
				}
			}
			else
			{
				LOG_E("DNA", "!!!!!!!!!!  Invalid operator in C_Eval(%s)  !!!!!!!!!!", param.c_str());
				assert(false);
				return false;
			}
			break;
		default:
			LOG_E("DNA", "!!!!!!!!!!  Invalid operator in C_Eval(%s)  !!!!!!!!!!", param.c_str());
			assert(false);
			return false;
		}
	};
	return bRet && (enumLastType != OPERATOR);
}

template<typename T>
bool ConditionEval::Compare(const T& srcSource, T_Operator enumOperator, const T& srcTarget)
{
	bool bRet = false;
	switch (enumOperator)
	{
	case EQUAL:
		bRet = (srcSource == srcTarget);
		break;
	case NOT_EQUAL:
		bRet = (srcSource != srcTarget);
		break;
	case GREATER:
		bRet = (srcSource > srcTarget);
		break;
	case GREATER_EQUAL:
		bRet = (srcSource >= srcTarget);
		break;
	case LESS:
		bRet = (srcSource < srcTarget);
		break;
	case LESS_EQUAL:
		bRet = (srcSource <= srcTarget);
		break;
	case INVALID_OPERATOR:
		LOG_E("DNA", "!!!!!!!!!! ConditionEval::Compare() Invalid operator  !!!!!!!!!!");
		assert(false);
		return false;
	default:
		LOG_W("DNA", "Warning ConditionEval::Compare() Unsupported operator  !!!!!!!!!!");
		break;
	}
	return bRet;
}

bool ConditionEval::findOperator(const String& cstrSentence)
{
	T_TokenType enumType = INVALID_TOKEN_TYPE;

	if (cstrSentence.size() == 0)
		return enumType == OPERATOR;
	for (int i = 0; i < cstrSentence.size(); i++)
	{
		switch (cstrSentence[i])
		{
		case '&':
		case '|':
		case '=':
		case '!':
		case '>':
		case '<':
			enumType = OPERATOR;
			break;
		default:
			break;
		}
	}
	if (enumType != OPERATOR)
	{
		const String EXT_OP[] = { NOT_EQUAL_UTF8, EQUAL_UTF8, GREATER_UTF8, LESS_UTF8, GREATER_EQUAL_UTF8, LESS_EQUAL_UTF8, PLUS_UTF8, MINUS_UTF8, PRODUCT_UTF8, DIVID_UTF8 };
		for (auto elem : EXT_OP)
		{
			if (cstrSentence.find(elem) != String::npos)
			{
				enumType = OPERATOR;
				break;
			}
		}
	}
	return enumType == OPERATOR;
}

void ConditionEval::RetrieveData(const String& src, String& target, const String& default_value)
{
	String _model_name = "";
	if (src.size() > 2 && src[0] == ':' && src[1] == ':')
		_model_name = src.substr(2);
	else if (src.size() > 4 && (src[0] == '"' || src[0] == '\'') && src.front()== src.back() && src[1] == ':' && src[2] == ':')
		_model_name = src.substr(3, src.size() - 4);
	if (!_model_name.empty())
	{
		if (_model_name.find('[') == String::npos)
		{
			owner_->Read<String>(_model_name, target);
			if (target == "")
				target = default_value;
		}
		else
		{
			size_t _start_pos = _model_name.find('[');
			size_t _end_pos = _model_name.find(']', _start_pos);
			if (_end_pos != String::npos)
			{
				if (_end_pos - _start_pos == 1)
				{
					String _model_name = "";
					Array<String> _operand;
					owner_->Read(_model_name.substr(0, _start_pos), _operand);
					if (_model_name.substr(_end_pos + 1, sizeof(".size") - 1) == ".size" || _model_name.substr(_end_pos + 1, sizeof(".length") - 1) == ".length")
					{
						target = std::to_string(_operand.size());
					}
				}
				else
				{
					Array<String> _operand;
					owner_->Read(_model_name.substr(0, _start_pos), _operand);
					int _index = stoi(_model_name.substr(_start_pos + 1, _end_pos - _start_pos - 1));
					if (_index < _operand.size())
						target = _operand[_index];
				}
			}
		}
	}
	else
	{
		if (default_value.empty())
			target = src;
		else
			target = default_value;
	}
}

bool ConditionEval::IsNumberType(const String& strToken)
{
	bool bIsNumberType = true;
	for (int i = 0; i < strToken.size(); i++)
	{
		if (!((strToken[i] == '.') || ((strToken[i] >= '0') && (strToken[i] <= '9'))))
		{
			bIsNumberType = false;
			break;
		}
	}
	return bIsNumberType;
}

void ConditionEval::CompareSourceTarget(const String& strOperand, T_Operator enumOperator, const String& strToken, bool& bRet)
{
	double dTargetOperand;
	if (Calculate(strToken, dTargetOperand) == true)
	{
		assert(strOperand != "");
		bRet &= Compare(stod(strOperand), enumOperator, dTargetOperand);
	}
	else if (strToken.back() == ')' && strToken.find('(') != String::npos)
	{
		bool _value = false;
		size_t _pos = strToken.find('(');
		String _name = strToken.substr(0, _pos);
		Array<Pair<String, String>> _params;
		_params.push_back(make_pair("param1", strToken.substr(_pos + 1, strToken.size() - _pos - 2)));
		owner_->WriteParams(_name, _params);
		owner_->owner()->do_event(_name);

		String _ret_name = String("return.") + _name;
		owner_->Read(_ret_name, _value);
		bRet &= _value;
	}
	else
	{
		String strTargetOperand = strToken;
		RetrieveData(strToken, strTargetOperand, strToken);
		if (strTargetOperand == "true" || strTargetOperand == "false")
		{
			if (strOperand == "")
				bRet &= (strTargetOperand == "false");
			else if (strOperand == "true" || strOperand == "false")
				bRet &= Compare(strOperand, enumOperator, strTargetOperand);
			else
				bRet &= Compare(stod(strOperand), enumOperator, (double)(strTargetOperand == "true"));
		}
		else
			bRet &= Compare(strOperand, enumOperator, strTargetOperand);
	}
}