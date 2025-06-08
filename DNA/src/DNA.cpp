#include "internal/DNA.h"
#include "internal/IModel.h"
#include "internal/utils/serializer.h"
#include "internal/utils/cell_utils.h"
#include "compile_time.h"
#include "ConditionEval.hpp"
#include "ActionEval.hpp"
#include <regex>
#include <locale>
#include <float.h>

#ifndef VERSION_MAJOR
#define VERSION_MAJOR 0
#endif

#ifndef VERSION_MINOR
#define VERSION_MINOR 0
#endif

#ifndef VERSION_BUILD
#define VERSION_BUILD 1
#endif

#ifndef VERSION_REVISION
#define VERSION_REVISION ""
#endif

using json = nlohmann::json;

#undef TAG
#define TAG "DNA"

USING_BIO_NAMESPACE
#ifdef __cplusplus
#ifdef STATIC_API
extern "C" PUBLIC_API IBiomolecule * DNA_CreateInstance(IBiomolecule * owner)
{
	return new DNA(owner);
}
#else
extern "C" PUBLIC_API IBiomolecule * CreateInstance(IBiomolecule * owner)
{
	return new DNA(owner);
}
#endif
#else
#endif

enum type_id_table_
{
	type_bool,
	type_int32_t,
	type_uint32_t,
	type_longlong_t,
	type_ulonglong_t,
	type_double,
	type_string,

	type_bool_array,
	type_int32_t_array,
	type_uint32_t_array,
	type_longlong_t_array,
	type_ulonglong_t_array,
	type_double_array,
	type_string_array,

	type_json,

	MAX_TYPE_COUNT,
};

bool DNA::discard_no_such_model_warning_ = false;
int DNA::MAX_REGEXPR_SIZE = 2048;

DNA::DNA(IBiomolecule* owner)
	: IBiomolecule(owner)
	, cond_eval_(Obj<ConditionEval>(new ConditionEval(this)))
	, action_eval_(Obj<ActionEval>(new ActionEval(this)))
{
	BuildTypeHastable();
	{
		DynaArray _stored_type;
		DynaArray _buf;
		model()->Read("Bio.Cell.Model.DiscardNoSuchModelWarning", _stored_type, _buf);
	#ifdef __linux__
		if (_stored_type == "l")
			_stored_type = "x";
		else if (_stored_type == "m")
			_stored_type = "y";
	#endif
		try {
			Deserialize(_stored_type.data(), _buf, discard_no_such_model_warning_);
		}
		catch (...)
		{
			Write("Bio.Cell.Model.DiscardNoSuchModelWarning", false);
		}
	}
	String _default_value = "";
	Read("Bio.Cell.Model.DefaultReturnModelname", _default_value);
	if (_default_value == "")
		Write("Bio.Cell.Model.DefaultReturnModelname", false);
}

DNA::~DNA()
{
}

bool DNA::init(const char* name, bool isFile)
{
	IBiomolecule::init("DNA");
	bool _retval = state_machine_.Load(name, this, isFile);
	SaveVersion();
	//Read("Bio.Cell.Model.DiscardNoSuchModelWarning", discard_no_such_model_warning_);
	return _retval;
};

void DNA::add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src)
{
	assert(false);
}

void DNA::add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src)
{
	assert(false);
}

void DNA::do_event(const DynaArray& msg_name)
{
	if (msg_name == "Bio.Chromosome.GetActiveStates")
	{
		Array<String> _state_list;
		get_state_list(_state_list);
		Write("Bio.Chromosome.GetActiveStates.Result.state_list", _state_list);
#ifdef _DEBUG
		String _states;
		Read("Bio.Chromosome.GetActiveStates.Result.state_list", _states);
		LOG_D(TAG, "Bio.Chromosome.GetActiveStates.Result.state_list=%s", _states.c_str());
#endif
	}
	else if (msg_name == "Bio.Chromosome.SetActiveStates")
	{
		Array<String> _state_list;
		Read("Bio.Chromosome.SetActiveStates.state_list", _state_list);
#ifdef _DEBUG
		String _states;
		Read("Bio.Chromosome.SetActiveStates.state_list", _states);
		LOG_D(TAG, "Bio.Chromosome.SetActiveStates.state_list=%s", _states.c_str());
#endif
		set_state_list(_state_list);
	}
	else if (msg_name == "Bio.Chromosome.StringFormat")
	{
		typedef char* PChar;
		String _format, _parameters, _result;
		Read(msg_name.str() + ".format", _format);
		Read(msg_name.str() + ".params", _parameters);
		ReplaceModelNameWithValue(_format);
		ReplaceModelNameWithValue(_parameters);
		Array<String> _format_list = split(_format, "%");
		if (_parameters.size() >= 2 && _parameters.front() == '[' && _parameters.back() == ']')
		{
			_parameters = _parameters.substr(1, _parameters.size() - 2);
		}
		Array<String> _parameter_list = split(_parameters, ",");
		int i = 0, j = 0;
		if (_format_list.size() > _parameter_list.size())
		{
			_result = _format_list[0];
			j = 1;
		}
		for (; i < _parameter_list.size() && j < _format_list.size(); i++, j++)
		{
			String _sub_result(4096,'\0');
			if (_format_list[j].find('d') != String::npos || _format_list[j].find('i') != String::npos)
			{
				//int _size = sprintf((char*)_sub_result.data(), ("%"+_format_list[j]).c_str(), std::stoi(_parameter_list[i]));
				//_sub_result.resize(_size, 0);
				sprintf((char*)_sub_result.data(), ("%" + _format_list[j]).c_str(), std::stoi(_parameter_list[i]));
				//_sub_result.shrink_to_fit();
			}
			else if (_format_list[j].find('f') != String::npos || _format_list[j].find('e') != String::npos || _format_list[j].find('E') != String::npos || _format_list[j].find('g') != String::npos || _format_list[j].find('G') != String::npos) {
				//int _size = sprintf((char*)_sub_result.data(), ("%"+_format_list[j]).c_str(), std::stod(_parameter_list[i]));
				//_sub_result.resize(_size, 0);
				sprintf((char*)_sub_result.data(), ("%" + _format_list[j]).c_str(), std::stod(_parameter_list[i]));
				//_sub_result.shrink_to_fit();
			}
			else if (_format_list[j].find('u') != String::npos || _format_list[j].find('x') != String::npos || _format_list[j].find('X') != String::npos || _format_list[j].find('o') != String::npos) {
				//int _size = sprintf((char*)_sub_result.data(), ("%" + _format_list[j]).c_str(), std::stoul(_parameter_list[i]));
				//_sub_result.resize(_size, 0);
				sprintf((char*)_sub_result.data(), ("%" + _format_list[j]).c_str(), std::stoul(_parameter_list[i]));
				//_sub_result.shrink_to_fit();
			}
			else {
				//int _size = sprintf((char*)_sub_result.data(), ("%" + _format_list[j]).c_str(), _parameter_list[i].c_str());
				//_sub_result.resize(_size, 0);
				sprintf((char*)_sub_result.data(), ("%" + _format_list[j]).c_str(), _parameter_list[i].c_str());
				//_sub_result.shrink_to_fit();
			}
			_result = _result.append(_sub_result.c_str());
		}
		String _target_model;
		Read(msg_name.str() + ".target_model_name", _target_model);
		if (_target_model != "")
			Write(_target_model, _result);
	}
	else if (msg_name == "Bio.Chromosome.Substring")
	{
		String _source_string, _target_model;
		unsigned long long _offset = 0, _count = 0;
		Read(msg_name.str() + ".source", _source_string);
		Read(msg_name.str() + ".offset", _offset);
		Read(msg_name.str() + ".count", _count);
		Read(msg_name.str() + ".target_model_name", _target_model);
		if (_target_model != "")
		{
			if (_count == 0)
				Write(_target_model, _source_string.substr(_offset));
			else
				Write(_target_model, _source_string.substr(_offset, _count));
		}
	}
	else if (msg_name == "Bio.Chromosome.ReplaceString")
	{
		String _source_string, _source_model, _target_model;
		String _to_be_replaced, _replace_with;
		bool _no_preprocessing = false;
		Read(msg_name.str() + ".source", _source_string);
		Read(msg_name.str() + ".source_model_name", _source_model);
		Read(msg_name.str() + ".to_be_replaced", _to_be_replaced);
		Read(msg_name.str() + ".replace_with", _replace_with);
		Read(msg_name.str() + ".target_model_name", _target_model);
		Read(msg_name.str() + ".no_preprocessing", _no_preprocessing);
		if (_target_model != "")
		{
			if (_source_string == "" && _source_model != "")
			{
				Read(_source_model, _source_string);
				if (!_no_preprocessing)
					ReplaceModelNameWithValue(_source_string);
			}
			Write(_target_model, std::regex_replace(_source_string, std::regex(_to_be_replaced), _replace_with));
		}
	}
	else if (msg_name == "Bio.Chromosome.SplitString")
	{
		String _source_string, _source_model, _target_model;
		String _delimiter;
		Array<String> _output;
		Read(msg_name.str() + ".source", _source_string);
		Read(msg_name.str() + ".source_model_name", _source_model);
		Read(msg_name.str() + ".delimiter", _delimiter);
		Read(msg_name.str() + ".target_model_name", _target_model);
		if (_target_model != "")
		{
			if (_source_string == "" && _source_model != "")
			{
				Read(_source_model, _source_string);
			}
			_output = split(_source_string, _delimiter.c_str());
			Write(_target_model, _output);
		}
	}
}

void DNA::on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload)
{
	try
	{
		Write("Bio.Cell.Current.ShortNamespace", owner()->name().str());
		Write("Bio.Cell.Current.FullNamespace", owner()->full_name().str());
		Write("Bio.Cell.Current.Event", msg_name.str());
		Write("Bio.Cell.Current.Action", (String)"");
		Write("Bio.Cell.Current.ActionType", (String)"");
		Write("Bio.Cell.Current.Condition", (String)"");
		//if (payload.size() > 1 || payload.size() == 1 && payload.data()[0] != '\0')
		//{	// it has been written in mRNA::unpack() called by Chromosome
		//	Write(String("encode.") + msg_name.str(), payload.str());
		//}
		LOG_T(TAG, "<%s> fire event %s: begin", owner()->full_name().data(), msg_name.data());
		state_machine_.EventFired(msg_name.data());
		LOG_T(TAG, "<%s> fire event %s: end", owner()->full_name().data(), msg_name.data());

		const int MAX_LIST_SIZE = 1;
		const char* _active_states[MAX_LIST_SIZE] = { nullptr };
		state_machine_.GetActiveStates(_active_states, MAX_LIST_SIZE);
		if (_active_states[0] == nullptr)
			Write("Bio.Cell.Current.State", (String)"");
		else
			Write("Bio.Cell.Current.State", (String)_active_states[0]);
	}
	catch (const std::exception& e)
	{
		std::cout << "Error! DNA::on_event(" << msg_name.data() << ") exception: " << e.what() << std::endl;
		assert(false);
	}
}

void DNA::on_action(int type, const char* name, const Array<Pair<String, String>>& params)
{
	assert(name != nullptr);
	String _name = name;
	ReplaceModelNameWithValue(_name);
	const char* _action_name = _name.c_str();
	Write("Bio.Cell.Current.Action", String(name));
	Write("Bio.Cell.Current.ActionType", type);
	//Read("Bio.Cell.Model.DiscardNoSuchModelWarning", discard_no_such_model_warning_);
	BioSys::IBiomolecule* _src = nullptr;
	switch (type)
	{
	case RAISE:
		_src = this;
	case SEND:
		resolved_model_cache_.clear();
		for (auto _elem : params)
		{
			if (_elem.second.size() > 2 && _elem.second[0] == ':' && _elem.second[1] == ':' && _elem.second.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.", 2) == String::npos)
			{
				Array<String> _value_list;
				Read(_elem.second.substr(2), _value_list);
				String _target_model_name = _name + "." + _elem.first;
				if (_value_list.size() > 0)
				{
					Clone(_target_model_name, _elem.second.substr(2));
				}
				else
				{
					String _value;
					Read(_elem.second.substr(2), _value);
					ReplaceModelNameWithValue(_value);

					bool _default_model_name = false;
					Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
					if (_default_model_name && _value == "")
					{
						//Clone(_target_model_name, _elem.second.substr(2), _elem.second);
						Write(_target_model_name, _elem.second);
					}
					else
					{
						//Clone(_target_model_name, _elem.second.substr(2));
						Write(_target_model_name, _value);
					}
					//_target_model_name = "::" + _target_model_name;
					//ReplaceModelNameWithValue(_target_model_name);
				}
			}
			else
			{
				ReplaceModelNameWithValue(_elem.second);
				if (_elem.second.size() >= 2)
				{
					// remove double quote only for single quote being database's keyword 
					//if (_elem.second.size() > 2 && (_elem.second.front() == '\"' || _elem.second.front() == '\'') && _elem.second.front() == _elem.second.back())
					if (_elem.second.size() > 2 && _elem.second.front() == '\"' && _elem.second.front() == _elem.second.back())
					{
						Write(_name + "." + _elem.first, _elem.second.substr(1, _elem.second.size() - 2));
					}
					else if ((_elem.second.front() != '[' && _elem.second.back() != ']')
						&& (_elem.second.front() != '{' && _elem.second.back() != '}')
						&& action_eval_->findOperator(_elem.second, true) == true)
					{
						//action_eval_->Eval((_name + "." + _elem.first + "=" + _elem.second).c_str());
						action_eval_->Eval(_elem.second, _name + "." + _elem.first);
					}
					else
					{
						Write(_name + "." + _elem.first, _elem.second);
					}
				}
				else
				{
					Write(_name + "." + _elem.first, _elem.second);
				}
			}
		}
		owner()->add_event(_action_name, "", _src);
		Remove((String)_action_name + ".*");
		break;
	case SCRIPT:
		resolved_model_cache_.clear();
		if (action_eval_->findOperator(_action_name) == true)
		{
			action_eval_->Eval(_action_name);
		}
		else
		{
			bool _default_model_name = false;
			Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
			Write("Bio.Cell.Model.DefaultReturnModelname", true);
			WriteParams(_action_name, params);
			owner()->do_event(_action_name);
			//Remove(_action_name + (String)".*");			// Don't do it. Let Chromosome do.
			RemoveParams(_action_name, params);
			Write("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
		}
		break;
	case ASSIGN:
		resolved_model_cache_.clear();
		for (auto _elem : params)
		{
			ReplaceModelNameWithValue(_elem.first);
			if ((_elem.second.size() > 2 && _elem.second[0] == ':' && _elem.second[1] == ':'
				&& _elem.second.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.*", 2) == String::npos)
				|| (_elem.second.size() > 4 && (_elem.second.front() == '\"' || _elem.second.front() == '\'') && _elem.second.front() == _elem.second.back() && _elem.second[1] == ':' && _elem.second[2] == ':')
				&& _elem.second.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.*", 3) == String::npos)
			{
				if (_elem.second[0] != ':')
					_elem.second = _elem.second.substr(1, sizeof(_elem.second) - 2);
				size_t _pos = _elem.first.find('[');
				bool _default_model_name = false;
				Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
				if (_pos == String::npos)
				{
					if (_elem.second.find(".*") == String::npos)
					{
						String _value = "";
						if (_default_model_name)
							_value = _elem.second;
						Read(_elem.second.substr(2), _value);
						ReplaceModelNameWithValue(_value);
						const String WHITE_SPACES(" \t\f\v\n\r");
						if (_value.size() >= 2 && (_value[_value.find_first_not_of(WHITE_SPACES)] == '{' && _value[_value.find_last_not_of(WHITE_SPACES)] == '}' || _value[_value.find_first_not_of(WHITE_SPACES)] == '[' && _value[_value.find_last_not_of(WHITE_SPACES)] == ']'))
						{
							json _object;
							try
							{
								_object = json::parse(_value);
								Write(_elem.first, _object);
							}
							catch (const std::exception& e)
							{
								Write(_elem.first, _value);
							}
						}
						else
						{
							Write(_elem.first, _value);
						}
					}
					else
					{
						if (_default_model_name)
							Clone(_elem.first, _elem.second.substr(2), _elem.second);
						else
							Clone(_elem.first, _elem.second.substr(2));
					}
				}
				else
				{
					if (_pos != _elem.first.size() - 1 && _elem.first[_pos + 1] == ']')
					{
						if (_elem.first.find("[*]") == String::npos)
						{
							if (_default_model_name)
								Pushback(_elem.first.substr(0, _pos), _elem.second.substr(2), _elem.second);
							else
								Pushback(_elem.first.substr(0, _pos), _elem.second.substr(2));
						}
						else
						{
							Clone(_elem.first, _elem.second.substr(2));
						}
					}
					else
					{
						if (_elem.second.find(".*") == String::npos)
						{
							String _value = "";
							if (_default_model_name)
								_value = _elem.second;
							Read(_elem.second.substr(2), _value);
							ReplaceModelNameWithValue(_value);
							deepAssign(_elem.first, _value, _pos);
						}
						else
						{
							if (_default_model_name)
								Clone(_elem.first, _elem.second.substr(2), _elem.second);
							else
								Clone(_elem.first, _elem.second.substr(2));
						}
					}
				}
			}
			//else if (_elem.second.find("[*]") != String::npos && _elem.second.size() > 2 && _elem.second[0] == ':' && _elem.second[1] == ':' && _elem.second.substr(2).find("::")==String::npos)
			else if (_elem.second.find("[*]") != String::npos && _elem.second.size() > 2 && _elem.second[0] == ':' && _elem.second[1] == ':')
			{
				ReplaceModelNameWithValue(_elem.second);		// ex. clone xxx[::xxx[xx]][*] to xxx.xxx.*
				Clone(_elem.first, _elem.second.substr(2));
			}
			else
			{
				ReplaceModelNameWithValue(_elem.second);
				if (_elem.second.size() >= 2
					&& (_elem.second.front() != '\"' && _elem.second.front() != '\'' || _elem.second.front() != _elem.second.back())
					&& (_elem.second.front() != '[' && _elem.second.back() != ']')
					&& (_elem.second.front() != '{' && _elem.second.back() != '}')
					)
				{
					const std::regex PURE_NUMBER("^([+-](?=\\.?\\d))?(\\d+)?(\\.\\d+)?$");
					if (_elem.second.size() > MAX_REGEXPR_SIZE || !std::regex_match(_elem.second, PURE_NUMBER))
					{
						//action_eval_->Eval(((String)"Bio.Chromosome.DNA.Temp.Index=" + _elem.second).c_str
						action_eval_->Eval(_elem.second, "Bio.Chromosome.DNA.Temp.Index");
						Read("Bio.Chromosome.DNA.Temp.Index", _elem.second);
					}
				}
				try
				{
					size_t _left_bracket_pos = _elem.first.find('[');
					//bool _default_model_name = false;
					//Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
					if (_left_bracket_pos == String::npos)
					{
						size_t _left_brace_pos = _elem.first.find('{');
						if (_left_brace_pos == _elem.first.size() - 2 && _elem.first.back() == '}')
						{
							String _key = _elem.first.substr(0, _left_brace_pos);
							json _root;
							Read(_key, _root);
							json _object;
							String _escaped_value = "";
							if (!_elem.second.empty())
							{
								try
								{
									_object = json::parse(_elem.second);
								}
								catch (const std::exception& e)
								{
									LOG_D(TAG, "Error when parsing %s as JSON, message=%s", _elem.second.c_str(), e.what());
									EscapeJSON(_elem.second, _escaped_value);
								}
							}
							if (_escaped_value != "")
							{
								try
								{
									_object = json::parse(_escaped_value);
									LOG_I(TAG, "JSON string escaped");
								}
								catch (const std::exception& e)
								{
									LOG_E(TAG, "Escaped!! Error when parsing %s as JSON, message=%s", _escaped_value.c_str(), e.what());
								}
							}
							if (!_root.is_null())
							{	// JSON format
								for (auto it : _object.items())
								{
									_root[it.key()] = it.value();
								}
							}
							Write(_key, _root);
						}
						else
						{
							if (_elem.second == "")
							{
								//Write<String>(_elem.first, "");
								Remove(_elem.first);
							}
							else if (_elem.second.size() >= 4
								&& (_elem.second.front() == '"' || _elem.second.front() == '\'')
								&& _elem.second.front() == _elem.second.back()
								&& _elem.second[1] == '{' && _elem.second[_elem.second.size() - 2] == '}')
							{
								String _escaped_value = EscapeJSONString(_elem.second.substr(1, _elem.second.size() - 2), true);
								Write(_elem.first, _escaped_value);
							}
							else
							{
								const std::regex PURE_NUMBER("^([+-](?=\\.?\\d))?(\\d+)?(\\.\\d+)?$");
								if (_elem.second.size() <= MAX_REGEXPR_SIZE && std::regex_match(_elem.second, PURE_NUMBER))
								{
									Write(_elem.first, _elem.second);
								}
								else
								{
									//action_eval_->Eval((_elem.first + "=" + _elem.second).c_str());
									if (_elem.second.find("[*]") == String::npos && _elem.second.find(".*") == String::npos)
									{
										const String& _value = _elem.second;
										const String WHITE_SPACES(" \t\f\v\n\r");
										if (_value.size() >= 2 && (_value[_value.find_first_not_of(WHITE_SPACES)] == '{' && _value[_value.find_last_not_of(WHITE_SPACES)] == '}' || _value[_value.find_first_not_of(WHITE_SPACES)] == '[' && _value[_value.find_last_not_of(WHITE_SPACES)] == ']'))
										{
											json _object;
											try
											{
												_object = json::parse(_elem.second);
												Write(_elem.first, _object);
											}
											catch (const std::exception& e)
											{
												action_eval_->Eval(_elem.second, _elem.first);
											}
										}
										else
										{
											action_eval_->Eval(_elem.second, _elem.first);
										}
									}
									else
									{
										Clone(_elem.first, _elem.second.substr(2));
									}
								}
							}
						}
					}
					else //if (_elem.second != "")
					{
						if (_elem.second == "")
						{
							String _target_name = _elem.first.substr(0, _left_bracket_pos);
							ReplaceModelNameWithValue(_target_name);
							if (_target_name.find("[") != String::npos)
							{
								deepAssign(_elem.first, _elem.second, _left_bracket_pos);
							}
							else
							{
								if (_left_bracket_pos != _elem.first.size() - 1)
								{
									size_t _right_bracket_pos = FindRightBracket(_elem.first, _left_bracket_pos, std::make_pair('[', ']'));
									String _index_str = _elem.first.substr(_left_bracket_pos + 1, _right_bracket_pos - _left_bracket_pos - 1);
									ReplaceModelNameWithValue(_index_str);
									if (_target_name[0] == '@')
									{
										_target_name = _target_name.substr(1);
										Array<String> _value_list;
										Read(_target_name, _value_list, false);
										try
										{
											if (_value_list.size() > 0)
											{
												int _index = std::stoi(_index_str);
												if (_index < 0)
												{
													_index = _index + (int)_value_list.size() >= 0 ? _index + (int)_value_list.size() : 0;
												}
												if (_index >= 0 && _index < _value_list.size())
												{
													for (int i = _index; i < _value_list.size() - 1; i++)
														std::swap(_value_list[i], _value_list[i + 1]);
													_value_list.pop_back();
													Write(_target_name, _value_list);
													if (for_each_cache_.count(_target_name))
														for_each_cache_.erase(_target_name);
												}
											}
										}
										catch (const std::exception& e)
										{
											LOG_W(TAG, "invalid array index %s, exception=%s", _index_str.c_str(), e.what());
										}
									}
									else
									{
										try
										{
											json _object = json::array();
											Read(_target_name, _object, false);
											if (_object.is_array() && _object.size() > 0)
											{
												int _index = std::stoi(_index_str);
												if (_index < 0)
												{
													_index = _index + (int)_object.size() >= 0 ? _index + (int)_object.size() : 0;
												}
												if (_index >= 0 && _index < _object.size())
												{
													for (int i = _index; i < _object.size() - 1; i++)
														std::swap(_object[i], _object[i + 1]);
													_object.erase(_object.size() - 1);
													Write(_target_name, _object);
													if (for_each_cache_.count(_target_name))
														for_each_cache_.erase(_target_name);
												}
												return;
											}
											else if (!_object.empty())
											{
												deepAssign(_elem.first, _elem.second, _left_bracket_pos);
												if (for_each_cache_.count(_target_name))
													for_each_cache_.erase(_target_name);
												return;
											}
										}
										catch (const std::exception& e)
										{
											LOG_W(TAG, "invalid JSON array index %s, exception=%s", _index_str.c_str(), e.what());
										}
										Array<String> _value_list;
										Read(_target_name, _value_list, false);
										try
										{
											if (_value_list.size() > 0)
											{
												int _index = std::stoi(_index_str);
												if (_index < 0)
												{
													_index = _index + (int)_value_list.size() >= 0 ? _index + (int)_value_list.size() : 0;
												}
												if (_index >= 0 && _index < _value_list.size())
												{
													for (int i = _index; i < _value_list.size() - 1; i++)
														std::swap(_value_list[i], _value_list[i + 1]);
													_value_list.pop_back();
													Write(_target_name, _value_list);
													if (for_each_cache_.count(_target_name))
														for_each_cache_.erase(_target_name);
												}
											}
										}
										catch (const std::exception& e)
										{
											LOG_W(TAG, "invalid array index %s, exception=%s", _index_str.c_str(), e.what());
										}
									}
								}
								else
								{
									deepAssign(_elem.first, _elem.second, _left_bracket_pos);
									if (for_each_cache_.count(_target_name))
										for_each_cache_.erase(_target_name);
								}
							}
						}
						else
						{
							if (_left_bracket_pos != _elem.first.size() - 1 && _elem.first[_left_bracket_pos + 1] == ']' &&  _elem.second.front() != '{' && _elem.second.back() != '}')
							{
								if (_elem.second.find("[*]") == String::npos)
								{
									String _array_name = _elem.first.substr(0, _left_bracket_pos);
									if (_array_name.find("[") != String::npos)
										deepAssign(_elem.first, _elem.second, _left_bracket_pos);
									else
										PushbackValue(_array_name, (_elem.second.front() == '\'' || _elem.second.front() == '"') && _elem.second.front() == _elem.second.back() ? _elem.second.substr(1,_elem.second.size()-2) : _elem.second);
									//action_eval_->Eval(_elem.first.substr(0, _left_bracket_pos) + "+=" + _elem.second);
								}
								else
								{
									Clone(_elem.first, _elem.second.substr(2));
								}
							}
							else
							{
								deepAssign(_elem.first, _elem.second, _left_bracket_pos);
							}
						}
					}
				}
				catch (...)
				{
					LOG_E(TAG, "Error when assign model %s with value %s", _elem.first.c_str(), _elem.second.c_str());
					return;
				}
				//Write(_elem.first, _elem.second);
			}
		}
		break;
	case LOG:
	{
		const int LEVEL_INFO = 3;
		Write("Logger.log.level", LEVEL_INFO);
		resolved_model_cache_.clear();
		bool _is_recursive = true;
		for (auto _elem : params)
		{
			String _value = _elem.second;
			if (_elem.first == "label" && _elem.second.size() > 0 && _elem.second[0] == '@')
				_is_recursive = false;
			ReplaceModelNameWithValue(_value, 0, String::npos, "", _is_recursive);
			Write("Logger.log." + _elem.first, _value);
		}

		String _label = "", _expr = "";
		Read("Logger.log.label", _label);
		Read("Logger.log.expr", _expr);
		LOG_I(_label.c_str(), "%s", _expr.c_str());

		break;
	}
	case FOREACH:
	{
		resolved_model_cache_.clear();
		bool _is_valid_json = true;
		json _array = json::array();
		json _name_array = json::array();
		String _model_name;
		int _index = -1;
		String _array_model_name = "";
		String _index_model_name = "";
		String _item_value_name = "";
		String _item_key_name = "";
		for (auto _elem : params)
		{
			if (_elem.first == "array")
			{
				_array_model_name = _elem.second;
				_model_name = _elem.second;
				if (for_each_cache_.count(_elem.second) > 0)
				{
					_array = for_each_cache_[_elem.second];
					if (_array.is_object())
					{
						_name_array = _array["key"];
						_array = _array["value"];
					}
				}
				else
				{
					//Read(_elem.second, _array);
					String _model_value = String("::") + _elem.second;
					ReplaceModelNameWithValue(_model_value);
					try
					{
						_array = json::parse(_model_value);
					}
					catch (const json::exception)
					{
						if (_model_value.empty())
							continue;
						const String& _value = _model_value;
						const String WHITE_SPACES(" \t\f\v\n\r");
						if (_value.size() >= 2 && (_value[_value.find_first_not_of(WHITE_SPACES)] == '{' && _value[_value.find_last_not_of(WHITE_SPACES)] == '}' || _value[_value.find_first_not_of(WHITE_SPACES)] == '[' && _value[_value.find_last_not_of(WHITE_SPACES)] == ']') && _value.find("\\\"") != String::npos)
						{
							_model_value = std::regex_replace(_model_value, std::regex(R"(\\\")"), "\"");
							try
							{
								_array = json::parse(_model_value);
							}
							catch (const json::exception)
							{
								Array<String> _value_list;
								Read(_elem.second, _value_list);
								if (_value_list.size() > 0)
								{
									_is_valid_json = false;
									for (auto& item : _value_list)
									{
										_array.push_back(item);
									}
									//for_each_cache_[_elem.second] = _array;
								}
								else
								{
									for (auto& elem : _model_value)
									{
										_array.push_back(String(1, elem));
									}
									for_each_cache_[_elem.second] = _array;
									Write(_array_model_name + ".cache", _array.size());
								}
								continue;
							}
						}
						else
						{
							Array<String> _value_list;
							Read(_elem.second, _value_list);
							if (_value_list.size() > 0)
							{
								_is_valid_json = false;
								for (auto& item : _value_list)
								{
									_array.push_back(item);
								}
								//for_each_cache_[_elem.second] = _array;
							}
							else
							{
								for (auto& elem : _model_value)
								{
									_array.push_back(String(1, elem));
								}
								for_each_cache_[_elem.second] = _array;
								Write(_array_model_name + ".cache", _array.size());
							}
							continue;
						}
					}
					if (_array.is_array())
					{
						for_each_cache_[_elem.second] = _array;
						Write(_array_model_name + ".cache", _array.size());
					}
					else if (_array.is_object())
					{
						json _value_array = json::array();
						for (auto itr = _array.begin(); itr != _array.end(); itr++)
						{
							_name_array.push_back(itr.key());
							_value_array.push_back(itr.value());
						}
						_value_array.swap(_array);
						for_each_cache_[_elem.second] = json::object();
						for_each_cache_[_elem.second]["value"] = _array;
						for_each_cache_[_elem.second]["key"] = _name_array;
						Write(_array_model_name + ".cache", _array.size());
					}
				}
			}
			else if (_elem.first == "index")
			{
				if (_elem.second[0] == '-')
				{
					_index_model_name = _elem.second.substr(1);
					Read(_index_model_name, _index);
					if (_index < 0)
					{
						_index = (int)_array.size() - 1;
						Write(_index_model_name, _index);
					}
				}
				else
				{
					_index_model_name = _elem.second;
					Read(_index_model_name, _index);
					if (_index < 0)
					{
						_index = 0;
						Write(_index_model_name, _index);
					}
				}
			}
			else if (_elem.first == "item")
			{
				if (_name_array.size() > _index)
				{
					Array<String> _map_item = split(_elem.second, ",");
					if (_map_item.size() > 1)
					{
						_item_key_name = _map_item[0];
						_item_value_name = _map_item[1];
					}
					else
					{
						_item_value_name = _elem.second;
					}
				}
				else
				{
					_item_value_name = _elem.second;
				}
			}
		}
		if (_index >= 0 && _array.size() > _index)
		{
			switch (_array[_index].type())
			{
			case json::value_t::number_integer:
				Write(_item_value_name, _array[_index].get<int>());
				break;
			case json::value_t::number_unsigned:
				Write(_item_value_name, _array[_index].get<unsigned int>());
				break;
			case json::value_t::number_float:
				Write(_item_value_name, _array[_index].get<double>());
				break;
			case json::value_t::string:
			{
				String _value = _array[_index].get<String>();
				if (_value.empty() || _value.front() == '\"' && _value.back() == '\"')
					Write(_item_value_name, _value);
				else
				{
					if (_model_name != "")
					{
						if (_is_valid_json)
						{
							Write(_item_value_name, _value);
						}
						else
						{
							size_t _break_pos = _model_name.find_last_of('.');
							Write("Bio.Chromosome.DecomposeMessage.message_name", _model_name.substr(0, _break_pos));
							Write("Bio.Chromosome.DecomposeMessage.field_name", _model_name.substr(_break_pos + 1));
							Write("Bio.Chromosome.DecomposeMessage.payload", _value);
							owner()->do_event("Bio.Chromosome.DecomposeMessage");
							json _json_payload;
							Read("@json." + _model_name, _json_payload);
							if (!_json_payload.empty())
								Write(_item_value_name, _json_payload);
							else
								Write(_item_value_name, _value);
						}
					}
				}
				break;
			}
			case json::value_t::boolean:
				Write(_item_value_name, _array[_index].get<bool>());
				break;
			case json::value_t::array:
			case json::value_t::object:
				Write(_item_value_name, _array[_index]);
				break;
			default:
				break;
			}
			if (_name_array.size() > _index)
			{
				Write(_item_key_name, _name_array[_index].get<String>());
			}

		}
		else
		{
			for_each_cache_.erase(_array_model_name);
			Write(_index_model_name, -1);
		}
		break;
	}
	case NEXT:
	{
		size_t _array_size = 0;
		int _index = -1;
		String _array_model_name = "";
		String _index_model_name = "";
		for (auto _elem : params)
		{
			if (_elem.first == "array")
			{
				_array_model_name = _elem.second;
				Read(_array_model_name + ".cache", _array_size);
				if (for_each_cache_.count(_elem.second) == 0 || _array_size != for_each_cache_[_elem.second].size())
				{
					//Read(_elem.second, _array);
					String _model_value = String("::") + _elem.second;
					ReplaceModelNameWithValue(_model_value);
					try
					{
						json _array = json::parse(_model_value);
						_array_size = _array.size();
						if (for_each_cache_.count(_elem.second) > 0)
						{
							if (_array.is_array())
							{
								for_each_cache_[_elem.second] = _array;
							}
							else if (_array.is_object())
							{
								json _value_array = json::array();
								json _name_array = json::array();
								for (auto itr = _array.begin(); itr != _array.end(); itr++)
								{
									_name_array.push_back(itr.key());
									_value_array.push_back(itr.value());
								}
								_value_array.swap(_array);
								for_each_cache_[_elem.second] = json::object();
								for_each_cache_[_elem.second]["value"] = _array;
								for_each_cache_[_elem.second]["key"] = _name_array;
							}
						}
					}
					catch (const json::exception)
					{
						Array<String> _value_list;
						Read(_elem.second, _value_list);
						_array_size = _value_list.size();
					}
					Write(_array_model_name + ".cache", _array_size);
				}
			}
			else if (_elem.first == "index")
			{
				if (_elem.second[0] == '-')
				{
					_index_model_name = _elem.second.substr(1);
					Read(_index_model_name, _index);
					_index--;
				}
				else
				{
					_index_model_name = _elem.second;
					Read(_index_model_name, _index);
					_index++;
				}
				break;
			}
		}
		if (_index >= 0 && _array_size > _index)
			Write(_index_model_name, _index);
		else
		{
			for_each_cache_.erase(_array_model_name);
			Write(_index_model_name, -1);
		}
		break;
	}
	case INVOKE:
		resolved_model_cache_.clear();
		Remove("Bio.Chromosome.New.*");
		for (auto _elem : params)
		{
			//if (_elem.second.size() > 2 && _elem.second[0] == ':' && _elem.second[1] == ':' && _elem.second.find('[') == String::npos)
			//	Clone("Bio.Chromosome.New." + _elem.first, _elem.second.substr(2));
			//else
			//	Write("Bio.Chromosome.New." + _elem.first, _elem.second);
			ReplaceModelNameWithValue(_elem.second);
			Write("Bio.Chromosome.New." + _elem.first, _elem.second);
		}
		owner()->do_event("Bio.Chromosome.New");
		Remove("Bio.Chromosome.New.*");
		break;
	case UNINVOKE:
		resolved_model_cache_.clear();
		for (auto _elem : params)
		{
			//if (_elem.second.size() > 2 && _elem.second[0] == ':' && _elem.second[1] == ':' && _elem.second.find('[') == String::npos)
			//	Clone("Bio.Chromosome.Remove." + _elem.first, _elem.second.substr(2));
			//else
			//	Write("Bio.Chromosome.Remove." + _elem.first, _elem.second);
			ReplaceModelNameWithValue(_elem.second);
			Write("Bio.Chromosome.Remove." + _elem.first, _elem.second);
		}
		owner()->do_event("Bio.Chromosome.Remove");
		Remove("Bio.Chromosome.Remove.*");
		break;
	case FINAL_STATE:
		owner()->add_event("Bio.Chromosome.Final", "", nullptr);
		break;
	}
}

bool DNA::on_condition(const char* name, const Array<Pair<String, String>>& params)
{
	assert(name != nullptr);
	resolved_model_cache_.clear();
	//Read("Bio.Cell.Model.DiscardNoSuchModelWarning", discard_no_such_model_warning_);
	bool _value = false;
	String _name = name;
	if (cond_eval_->findOperator(name) == true)
	{
		ReplaceModelNameWithValue(_name);
		if (_name.size() >= 2 && (_name.back() == '&' || _name[_name.size() - 2] == '&'))
			_value = false;
		else
			_value = cond_eval_->Eval(_name);
	}
	else if (strlen(name) > 2 && name[0] == ':' && name[1] == ':')
	{
		ReplaceModelNameWithValue(_name);
		try
		{
			_value = (!_name.empty() && (_name == "true" || _name.size() == 1 && stoi(_name) == 1));
		}
		catch (const std::exception& e)
		{
			LOG_E(TAG, "exception in on_condition(%s), value size=%zd, value=%s: %s", name, _name.size(), _name.c_str(), e.what());
		}
	}
	else
	{
		size_t _pos = _name.find('(');
		String _cond_name = _name.substr(0, _pos);
		Array<Pair<String, String>> _cond_params;
		_cond_params.push_back(make_pair("param1", _name.substr(_pos + 1, _name.size() - _pos - 2)));
		WriteParams(_cond_name, _cond_params);
		owner()->do_event(_cond_name);

		String _ret_name = String("return.") + _cond_name;
		Read(_ret_name, _value);
	}
	if (_value)
		Write("Bio.Cell.Current.Condition", String(name));
	return _value;
}

bool DNA::get_content(const char* name, String& content)
{
	content = "";
	Read(name, content);
	return content != "";
}

void DNA::WriteParams(const String& name, const Array<Pair<String, String>>& params)
{
	for (auto _elem : params)
	{	// _elem.first == "params_in_JSON"
		//ReplaceModelNameWithValue(_elem.second);		// Don't do this here, it may be binary data
		if (_elem.second.size() <= 2 || _elem.second[0] != '{' || _elem.second.back() != '}')
		{
			LOG_E(TAG, "Parameters \"%s\" must start with '{' and end with '}'\n", _elem.second.c_str());
			continue;
		}

		json _root;
		String _escaped_value;
		try
		{
			_root = json::parse(_elem.second);
		}
		catch (...)
		{
			_escaped_value = _elem.second;
			ReplaceModelNameWithValue(_escaped_value);
		}
		if (_escaped_value != "")
		{
			try
			{
				_root = json::parse(_escaped_value);
				LOG_I(TAG, "JSON string escaped");
			}
			catch (const std::exception)
			{
				LOG_E(TAG, "Error when parsing parameters %s of %s", _elem.second.c_str(), name.c_str());
				assert(false);
				return;
			}
		}
		assert(_root.is_object());
		for (auto _obj : _root.items())
		{
			String _key = String(name) + "." + _obj.key();
			if (_obj.value().is_string())
			{
				String _value = _obj.value();
				if (_value.size() > 2 && _value[0] == ':' && _value[1] == ':' && _value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.", 2) == String::npos)
					Clone(_key, _value.substr(2));
				else
				{
					ReplaceModelNameWithValue(_value);
					Write(_key, _value);
				}
			}
			else if (_obj.value().is_boolean())
			{
				Write(_key, _obj.value().get<bool>());
			}
			else if (_obj.value().is_number_integer())
			{
				Write(_key, _obj.value().get<int>());
			}
			else if (_obj.value().is_number_unsigned())
			{
				Write(_key, _obj.value().get<unsigned int>());
			}
			else if (_obj.value().is_number_float())
			{
				Write(_key, _obj.value().get<double>());
			}
			else if (_obj.value().is_array())
			{
				if (_obj.value().size() == 0)
					Write(_key, "[]");
				else
				{
					if (_obj.value()[0].is_string())
					{
						Array<String> _value = _obj.value().get<Array<String>>();
						for (auto& elem : _value)
						{
							ReplaceModelNameWithValue(elem);
						}
						Write(_key, _value);
					}
					else if (_obj.value()[0].is_boolean())
					{
						Array<bool> _value1 = _obj.value().get<Array<bool>>();
						Array<String> _value;
						std::transform(_value1.cbegin(), _value1.cend(), std::back_inserter(_value), [](const bool& elem)->String {
							return elem == true ? "true" : "false";
							});
						Write(_key, _value);
					}
					else if(_obj.value()[0].is_number_integer())
					{
						Array<int> _value1 = _obj.value().get<Array<int>>();
						Array<String> _value;
						std::transform(_value1.cbegin(), _value1.cend(), std::back_inserter(_value), [](const int& elem)->String {
							return std::to_string(elem);
							});
						Write(_key, _value);
					}
					else if (_obj.value()[0].is_number_unsigned())
					{
						Array<unsigned int> _value1 = _obj.value().get<Array<unsigned int>>();
						Array<String> _value;
						std::transform(_value1.cbegin(), _value1.cend(), std::back_inserter(_value), [](const unsigned int& elem)->String {
							return std::to_string(elem);
							});
						Write(_key, _value);
					}
					else if (_obj.value()[0].is_number_float())
					{
						Array<double> _value1 = _obj.value().get<Array<double>>();
						Array<String> _value;
						std::transform(_value1.cbegin(), _value1.cend(), std::back_inserter(_value), [](const double& elem)->String {
							return std::to_string(elem);
							});
						Write(_key, _value);
					}
					else
					{
						Array<String> _value;
						for (auto elem : _obj.value())
						{
							String _elem = elem.dump();
							ReplaceModelNameWithValue(_elem);
							_value.emplace_back(_elem);
						}
						Write(_key, _value);
					}
				}
			}
			else if (_obj.value().is_object())
			{
				Map<String, String> _value = _obj.value().get<Map<String, String>>();
				for (auto& elem : _value)
				{
					//ReplaceModelNameWithValue(elem.first);
					ReplaceModelNameWithValue(elem.second);
				}
				Write(_key, _value);
			}
		}
	}
}

void DNA::RemoveParams(const String& name, const Array<Pair<String, String>>& params)
{
	for (auto _elem : params)
	{	// _elem.first == "params_in_JSON"
		if (_elem.second.size() <= 2 || _elem.second[0] != '{' || _elem.second.back() != '}')
		{
			LOG_E(TAG, "Parameters \"%s\" must start with '{' and end with '}'\n", _elem.second.c_str());
			continue;
		}
		//ReplaceModelNameWithValue(_elem.second);		// Don't do this here. It may be binary data.
		json _root;
		String _escaped_value;
		try
		{
			_root = json::parse(_elem.second);
		}
		catch (...)
		{
			_escaped_value = _elem.second;
			ReplaceModelNameWithValue(_escaped_value);
		}
		if (_escaped_value != "")
		{
			try
			{
				_root = json::parse(_escaped_value);
				LOG_I(TAG, "JSON string escaped");
			}
			catch (const std::exception)
			{
				LOG_E(TAG, "Error when parsing parameters %s of %s", _elem.second.c_str(), name.c_str());
				assert(false);
				return;
			}
		}
		assert(_root.is_object());
		for (auto _obj : _root.items())
		{
			String _key = String(name) + "." + _obj.key();
			model()->Remove(_key);
		}
	}
}

template<typename T>
const char* DNA::GetType(const T& value)
{
	return typeid(value).name();
}

void DNA::Write(const String& name, const nlohmann::json& value)
{
	model()->Write(name, GetType(value), value);
}

template<typename T>
void DNA::Write(const String& name, const T& value)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	out(value);
	model()->Write(name, GetType(value), (DynaArray)data);
}

void DNA::Read(const String& name, bool& value, bool auto_conversion)
{
	DynaArray _stored_type;
	DynaArray _buf;
	model()->Read(name, _stored_type, _buf, auto_conversion);
#ifdef __linux__
	if (_stored_type == "l")
		_stored_type = "x";
	else if (_stored_type == "m")
		_stored_type = "y";
#endif
	try {
		Deserialize(_stored_type.data(), _buf, value);
	}
	catch (...)
	{
		Read("Bio.Cell.Model.DiscardNoSuchModelWarning", discard_no_such_model_warning_);
		if (!discard_no_such_model_warning_)
		{
			if (name.substr(0, strlen("Bio.Cell.")) == "Bio.Cell.")
			{
				LOG_D(TAG, "No such key in Model, key name: %s", name.c_str());
			}
			else
			{
				LOG_W(TAG, "No such key in Model, key name: %s", name.c_str());
			}
		}
	}
}

void DNA::Read(const String& name, nlohmann::json& value, bool auto_conversion)
{
	DynaArray _stored_type;
	model()->Read(name, _stored_type, value, auto_conversion);
	if (!auto_conversion && _stored_type != GetType(value))
	{
		value = json();
	}
}

template<typename T>
void DNA::Read(const String& name, T& value, bool auto_conversion)
{
	DynaArray _stored_type;
	DynaArray _buf;
	model()->Read(name, _stored_type, _buf);
	if (!auto_conversion && _stored_type != GetType(value))
		return;
#ifdef __linux__
	if (_stored_type == "l")
		_stored_type = "x";
	else if (_stored_type == "m")
		_stored_type = "y";
#endif
	try {
		Deserialize(_stored_type.data(), _buf, value);
	}
	catch (...)
	{
		Read("Bio.Cell.Model.DiscardNoSuchModelWarning", discard_no_such_model_warning_);
		if (!discard_no_such_model_warning_)
		{
			if (name.substr(0, strlen("Bio.Cell.")) == "Bio.Cell.")
			{
				LOG_D(TAG, "No such key in Model, key name: %s", name.c_str());
			}
			else
			{
				LOG_W(TAG, "No such key in Model, key name: %s", name.c_str());
			}
		}
	}
}


void DNA::Clone(const String& target_name, const String& src_name, const String& default_value)
{
	DynaArray _stored_type;
	DynaArray _buf;
	if (src_name.back() == '*' || src_name.substr(src_name.size() - 3) == "[*]" || target_name.substr(target_name.size() - 3) == "[*]")
		model()->Clone(target_name, src_name);
	else
	{
		model()->Read(src_name, _stored_type, _buf);
		if (_stored_type != "")
			model()->Clone(target_name, src_name);
		else
			Write(target_name, default_value);
	}
}

void DNA::Remove(const String& name)
{
	model()->Remove(name);
}

void DNA::Pushback(const String& target_name, const String& src_name, const String& default_value)
{
	String _value = default_value;
	Read(src_name, _value);
	PushbackValue(target_name, _value);
}

void DNA::PushbackValue(const String& target_name, const String& value)
{
	const String WHITE_SPACES(" \t\f\v\n\r");
	if (value.size() >= 2 && (value[value.find_first_not_of(WHITE_SPACES)] == '{' && value[value.find_last_not_of(WHITE_SPACES)] == '}' || value[value.find_first_not_of(WHITE_SPACES)] == '[' && value[value.find_last_not_of(WHITE_SPACES)] == ']'))
	{
		String _target_name = target_name;
		if (target_name[0] == '@')
		{
			String _target_name = target_name.substr(1);
		}
		json _root = json::array();
		Read(_target_name, _root);
		json _item = json::parse(value);
		_root.push_back(_item);
		Write(_target_name, _root);
	}
	else
	{
		const String CTRL_CODE("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f");
		if (target_name[0] == '@' || value.find_first_of(CTRL_CODE) != String::npos)
		{
			Array<String> _list = Array<String>();
			if (target_name[0] == '@')
			{
				String _target_name = target_name.substr(1);
				Read(_target_name, _list);
				_list.push_back(value);
				Write(_target_name, _list);
			}
			else
			{
				Read(target_name, _list);
				_list.push_back(value);
				Write(target_name, _list);
			}
		}
		else
		{
			json _root;
			Read(target_name, _root);
			if (_root.is_array())
			{
				switch (_root.size() > 0 ? _root.back().type() : json::value_t::string)
				{
				case json::value_t::number_integer:
				case json::value_t::number_unsigned:
					_root.push_back(stoi(value));
					break;
				case json::value_t::number_float:
					_root.push_back(stod(value));
					break;
				case json::value_t::string:
				case json::value_t::array:
				case json::value_t::object:
					_root.push_back(value);
					break;
				case json::value_t::boolean:
					_root.push_back(value=="true");
					break;
				default:
					break;
				}
				Write(target_name, _root);
			}
			else
			{
				Array<String> _list = Array<String>();
				Read(target_name, _list);
				_list.push_back(value);
				Write(target_name, _list);
			}
		}
	}
}

template <typename T2>
void DNA::Deserialize(bool, const DynaArray& data, T2& value)
{
	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		ByteArray _data(data.size());
		memcpy(_data.data(), data.data(), data.size());
		zpp::serializer::memory_input_archive in(_data);

		int8_t _value;
		in(_value);
		TypeConversion(_value == 1, value);
	}
}

template <typename T>
void DNA::Deserialize(const char* type, const DynaArray& data, T& value)
{
	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		size_t _type_hash = hash_((String)type);
		//if (type == nullptr || type[0] == '\0')
		//	TypeConversion(String(""), value);
		//else if (type_hash_table_[_type_hash] == type_bool)
		if (type_hash_table_[_type_hash] == type_bool)
			Deserialize(bool(), data, value);
		else
		{
			if (_type_hash == hash_((String)GetType(value)))
			{
				ByteArray _data(data.size());
				memcpy(_data.data(), data.data(), data.size());
				zpp::serializer::memory_input_archive in(_data);
				in(value);
			}
			else
			{
				typedef long long longlong;
				typedef unsigned long long ulonglong;
				switch (type_hash_table_[_type_hash])
				{
				case type_int32_t:
					Deserialize(int32_t(), data, value);
					break;
				case type_uint32_t:
					Deserialize(uint32_t(), data, value);
					break;
				case type_longlong_t:
					Deserialize(longlong(), data, value);
					break;
				case type_ulonglong_t:
					Deserialize(ulonglong(), data, value);
					break;
				case type_double:
					Deserialize(double(), data, value);
					break;
				case type_string:
					Deserialize(String(), data, value);
					break;

				case type_bool_array:
					Deserialize(Array<bool>(), data, value);
					break;
				case type_int32_t_array:
					Deserialize(Array<int32_t>(), data, value);
					break;
				case type_uint32_t_array:
					Deserialize(Array<uint32_t>(), data, value);
					break;
				case type_longlong_t_array:
					Deserialize(Array<longlong>(), data, value);
					break;
				case type_ulonglong_t_array:
					Deserialize(Array<ulonglong>(), data, value);
					break;
				case type_double_array:
					Deserialize(Array<double>(), data, value);
					break;
				case type_string_array:
					Deserialize(Array<String>(), data, value);
					break;
				case type_json:
					Deserialize(nlohmann::json(), data, value);
					break;
				default:
					std::cout << "Unsupported type" << std::endl;
					break;
				}
			}
		}
	}
}

template <typename T1, typename T2>
void DNA::Deserialize(T1, const DynaArray& data, T2& value)
{
	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		ByteArray _data(data.size());
		memcpy(_data.data(), data.data(), data.size());
		zpp::serializer::memory_input_archive in(_data);

		T1 _value;
		in(_value);
		TypeConversion(_value, value);
	}
}

template <typename T2>
void DNA::Deserialize(Array<bool>, const DynaArray& data, T2& value)
{
	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		ByteArray _data(data.size());
		memcpy(_data.data(), data.data(), data.size());
		zpp::serializer::memory_input_archive in(_data);
		Array<int8_t> _value_in_storage;
		in(_value_in_storage);

		Array<bool> _value;
		std::transform(_value_in_storage.cbegin(), _value_in_storage.cend(), std::back_inserter(_value), [](const int8_t& elem)->bool {
			return elem == 1;
			});
		TypeConversion(_value, value);
	}
}

template <typename T2>
void DNA::Deserialize(json, const DynaArray& data, T2& value)
{
	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		String _value = data.str();
		TypeConversion(_value, value);
	}
}

void DNA::Deserialize(const char* type, const DynaArray& data, bool& value)
{
	value = false;
	size_t _type_hash = hash_(type);
	if (type_hash_table_[_type_hash] == type_bool)
	{
		ByteArray _data(data.size());
		memcpy(_data.data(), data.data(), data.size());
		zpp::serializer::memory_input_archive in(_data);

		int8_t _value;
		in(_value);
		value = (_value == 1);
	}
	else
	{
		typedef long long longlong;
		typedef unsigned long long ulonglong;
		switch (type_hash_table_[_type_hash])
		{
		case type_int32_t:
			Deserialize(int32_t(), data, value);
			break;
		case type_uint32_t:
			Deserialize(uint32_t(), data, value);
			break;
		case type_longlong_t:
			Deserialize(longlong(), data, value);
			break;
		case type_ulonglong_t:
			Deserialize(ulonglong(), data, value);
			break;
		case type_double:
			Deserialize(double(), data, value);
			break;
		case type_string:
			Deserialize(String(), data, value);
			break;

		case type_bool_array:
			Deserialize(Array<bool>(), data, value);
			break;
		case type_int32_t_array:
			Deserialize(Array<int32_t>(), data, value);
			break;
		case type_uint32_t_array:
			Deserialize(Array<uint32_t>(), data, value);
			break;
		case type_longlong_t_array:
			Deserialize(Array<longlong>(), data, value);
			break;
		case type_ulonglong_t_array:
			Deserialize(Array<ulonglong>(), data, value);
			break;
		case type_double_array:
			Deserialize(Array<double>(), data, value);
			break;
		case type_string_array:
			Deserialize(Array<String>(), data, value);
			break;
		case type_json:
			Deserialize(nlohmann::json(), data, value);
			break;
		}
	}
}

template<typename T1, typename T2>
void DNA::TypeConversion(const T1& src, T2& dest) {
	dest = (T2)src;
}

void DNA::TypeConversion(const String& src, bool& dest) {
	if (src == "true")
		dest = true;
	else
	{
		if (is_number(src))
			dest = (bool)stod(src);
		else
			dest = false;
	}
};

template<typename T>
void DNA::TypeConversion(const T& src, String& dest) { dest = std::to_string(src); };
void DNA::TypeConversion(const double& src, String& dest)
{
	dest = to_string_with_precision(src, DBL_DIG);
};
template<typename T>
void DNA::TypeConversion(const String& src, T& dest) {
	if (is_number(src))
		dest = (T)stod(src);
	else
		dest = (T)0.0;
};
void DNA::TypeConversion(const bool& src, String& dest) { dest = (src == true ? "true" : "false"); };
void DNA::TypeConversion(const String& src, String& dest) { dest = src; };
template<typename T>
void DNA::TypeConversion(const T& src, Array<String>& dest) { dest.push_back(std::to_string(src)); };
void DNA::TypeConversion(const double& src, Array<String>& dest)
{ 
	dest.push_back(to_string_with_precision(src, DBL_DIG));
};
template<typename T1, typename T2>
void DNA::TypeConversion(const T1& src, Array<T2>& dest) { dest.push_back((T2)src); };
template<typename T>
void DNA::TypeConversion(const String& src, Array<T>& dest) {
	Array<String> _string_array;
	if (src.size() > 2 && src[0] == '[' && src.back() == ']')
	{
		_string_array = split(src.substr(1, src.size() - 2), ",");
	}
	TypeConversion(_string_array, dest);
};


template<typename T1, typename T2>
void DNA::TypeConversion(const Array<T1>& src, T2& dest) {
	dest = (src.size() > 0 ? (T2)src.front() : dest);
};

template<typename T>
void DNA::TypeConversion(const Array<T>& src, String& dest) {
	dest = "[";
	for (auto _elem : src)
	{
		dest += std::to_string(_elem) + ",";
	}
	if (dest.size() > 1)
		dest.back() = ']';
	else
		dest.push_back(']');
};

template<typename T>
void DNA::TypeConversion(const Array<String>& src, T& dest) {
	if (src.size() > 1)
		TypeConversion(src[0], dest);
	else
		dest = (T)0.0;
};

void DNA::TypeConversion(const Array<String>& src, String& dest) {
	dest = "[";
	for (auto _elem : src)
	{
		dest += _elem + ",";
	}
	if (dest.size() > 1)
		dest.back() = ']';
	else
		dest.push_back(']');
};

void DNA::TypeConversion(const String& src, Array<String>& dest)
{
	if (src.size() > 2 && src[0] == '[' && src.back() == ']')
	{
		String _src = std::regex_replace(src, std::regex("\",\""), ",");
		if (_src.size() > 3 && _src[1] == '\"')
			_src.erase(1, 1);
		if (_src.size() > 3 && _src[_src.size() - 2] == '\"')
			_src.erase(_src.size() - 2, 1);
		dest = split(_src.substr(1, _src.size() - 2), ",");
	}
};

void DNA::TypeConversion(const Array<bool>& src, String& dest) {
	dest = "[";
	for (auto _elem : src)
	{
		dest += String(_elem == true ? "true" : "false") + ",";
	}
	if (dest.size() > 1)
		dest.back() = ']';
	else
		dest.push_back(']');
};

// Array to Array
template<typename T1, typename T2>
void DNA::TypeConversion(const Array<T1>& src, Array<T2>& dest) {
	std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const T1& elem)->T2 {
		return (T2)elem;
		});
};

template<typename T>
void DNA::TypeConversion(const Array<T>& src, Array<String>& dest) {
	std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const T& elem)->String {
		return std::to_string(elem);
		});
};

template<typename T>
void DNA::TypeConversion(const Array<String>& src, Array<T>& dest) {
	std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [this](const String& elem)->T {
		T _retval;
		this->TypeConversion(elem, _retval);
		return _retval;
		});
};

void DNA::TypeConversion(const Array<String>& src, Array<String>& dest) {
	dest = src;
};

void DNA::BuildTypeHastable()
{
	const char* _type_id_table_name[] = { 
		typeid(const bool).name(), 
		typeid(int32_t).name(),
		typeid(uint32_t).name(),
		typeid(long long).name(),
		typeid(unsigned long long).name(),
		typeid(double).name(),
		typeid(String).name(),
		typeid(Array<bool>).name(),
		typeid(Array<int32_t>).name(),
		typeid(Array<uint32_t>).name(),
		typeid(Array<long long>).name(),
		typeid(Array<unsigned long long>).name(),
		typeid(Array<double>).name(),
		typeid(Array<String>).name(),
		typeid(json).name()
	};
	for (int i = 0; i < MAX_TYPE_COUNT; i++)
	{
		type_hash_table_[hash_(_type_id_table_name[i])] = i;
	}
}

void DNA::SaveVersion()
{
	Write(String("Bio.Cell.version.") + name().data(), (String)get_version());
	Write(String("Bio.Cell.version.major.") + name().data(), VERSION_MAJOR);
	Write(String("Bio.Cell.version.minor.") + name().data(), VERSION_MINOR);
	Write(String("Bio.Cell.version.build.") + name().data(), VERSION_BUILD);
	Write(String("Bio.Cell.version.revision.") + name().data(), (String)VERSION_REVISION);
	const long long TIME_DIFFERENCE = -8 * 60 * 60;
	Write(String("Bio.Cell.version.time.") + name().data(), (long long)__TIME_UNIX__ + TIME_DIFFERENCE);
}

const char* DNA::get_version()
{
	static String _version = std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_BUILD) + "." + VERSION_REVISION;
	return _version.c_str();
}

int DNA::findAndReplaceAll(String& data, const String& toSearch, const String& replaceStr, size_t target_start, size_t target_end)
{
	int _ret = 0;
	// Get the first occurrence
	size_t pos = data.find(toSearch, target_start);
	String _replace_str = "";
	if (replaceStr.size() >= 2 && replaceStr.front() == '\"' && replaceStr.back() == '\"')
		_replace_str = replaceStr.substr(1, replaceStr.size() - 2);
	else
		_replace_str = replaceStr;
	// Repeat till end is reached
	while (pos < target_end)
	{
		// Replace this occurrence of Sub String
		unsigned char _next_char = data[pos + toSearch.size()];
		if (!std::isalpha(_next_char) && !std::isdigit(_next_char) && _next_char != '_' && _next_char != '.' && _next_char != '['
			|| toSearch.substr(0, 3) == "::(" && toSearch.back() == ')'
			|| toSearch.back() == ']')				// for "::a1.b1.c1[x1][y1]::a2.b2.c2[x2][y2]" without space as delimeters
		{
			data.replace(pos, toSearch.size(), _replace_str);
			_ret++;
			// Get the next occurrence from the current position
			pos = data.find(toSearch, pos + _replace_str.size());
		}
		else
		{
			//if (_next_char == '.')
			//	return false;
			//else
				pos = data.find(toSearch, pos + toSearch.size());
		}
	}
	return _ret;
}

//void DNA::ReplaceModelNameWithValue(String& target, const String& resolved_model_name)
void DNA::ReplaceModelNameWithValue(String& target, size_t target_start, size_t target_end, const String& resolved_model_name, bool recursive)
{
	if (target.find('\0') != String::npos)		// ignore binary data
		return;
	Stack<Pair<size_t, size_t>> _active_range;
	Set<Pair<size_t, size_t>> _ignored_range_list;
	Set<String> _ignored_token_list;
	_active_range.push(std::make_pair(0, String::npos));
	while (!_active_range.empty())
	{
		size_t _start_pos = _active_range.top().first;
		size_t _end_pos = _active_range.top().second;
		size_t _range_end = _end_pos;
		while ((_start_pos = target.find("::", _start_pos)) < _range_end)
		{
			if (_start_pos > 0 && target[_start_pos - 1] == '`')
			{	// ignoring all "::" between ` `
				_start_pos = target.find('`', _start_pos);
				continue;
			}
			size_t _next_start = _start_pos;
			while (target.size() > _next_start + 2 && target[_next_start + 2] == ':')
				_next_start++;
			if (_next_start - _start_pos > 0)
			{
				if (_next_start - _start_pos > 1 && _ignored_range_list.find(std::make_pair(_start_pos, _range_end)) == _ignored_range_list.end())
				{
					_active_range.push(std::make_pair(_start_pos, _range_end));
					_ignored_range_list.insert(std::make_pair(_start_pos, _range_end));
				}
				_start_pos = _next_start;
			}
			if (target[_start_pos + 2] == '(')
			{
				size_t _token_end_pos = FindRightBracket(target, _start_pos + 1, std::make_pair('(', ')'));
				String _tobe_replaced = target.substr(_start_pos, _token_end_pos - _start_pos + 1);
				String _token = target.substr(_start_pos + 3, _token_end_pos - _start_pos - 3);
				size_t _model_pos = _token.find("::");
				if (_model_pos != String::npos && _ignored_range_list.find(std::make_pair(_start_pos + 3, _token_end_pos)) == _ignored_range_list.end())
				{
					if (_ignored_token_list.find(_token) == _ignored_token_list.end())
					{
						_active_range.push(std::make_pair(_start_pos + 3, _token_end_pos));
						_ignored_range_list.insert(std::make_pair(_start_pos + 3, _token_end_pos));
						_ignored_token_list.insert(_token);
						break;
					}
					else
					{
						_start_pos = _token_end_pos;
						continue;
					}
				}
				//ReplaceModelNameWithValue(_token);
				//action_eval_->Eval(((String)"Bio.Chromosome.DNA.Temp.Value=" + _token).c_str());
				//const std::regex NOT_BINARY_EXPR("^[^\t\n\x0B\f\r]*$");
				//const std::regex NOT_BINARY_EXPR("^[^\x0]*$");
				//if (_token.find("::") == String::npos && std::regex_match(_token, NOT_BINARY_EXPR))
				const std::regex TERNARY_OPERATOR("^[^?]*? *\\? *[^?]+ *: *[^?]+ *$");
				bool _is_ternary_op = _token.size()<= MAX_REGEXPR_SIZE && std::regex_match(_token, TERNARY_OPERATOR);
				if (_token.find('\0') == String::npos && (_model_pos == String::npos || _model_pos > 1 && _token[_model_pos - 1] == '`' || _is_ternary_op == true))
				{
					const std::regex EVALUABLE_EXPR("^[\\w.+\\-*\\/%&~^?|',:<>!={}\\[\\]\\(\\) ]*$");
					//const std::regex STRING_COMPARISON("^[\"'][^?]+[\"'] *[=><]{1,2} *[\"'][^?]+[\"'](&&|\|\|)*\w*$");
					String _model_value;
					if (_token.empty()) {
						_model_value = "";
					}
					else if (_token.size() >= 2 && (_token.front() == '{' && _token.back() == '}'
						|| _token.front() == '[' && _token.back() == ']'))
					{	// escaping JSON
						_model_value = EscapeJSONString(_token, true);
					}
					else if (_is_ternary_op)
					{
						size_t _question_pos = _token.find('?');
						size_t _splitter = _token.find(':', _question_pos + 1);
						size_t _next_quot = _token.find_first_not_of(' ', _splitter + 1);
						while (_next_quot != String::npos && _splitter != String::npos
							&& _token[_next_quot] != '\'')
							//&& _token[_next_quot] != '\'' && _token[_next_quot] != '"')		// for JSON, ignore \"
						{
							_splitter = _token.find(':', _next_quot + 1);
							_next_quot = _token.find_first_not_of(' ', _splitter + 1);
						}
						String _eval_token = _token.substr(0, _question_pos);
						bool _result = false;
						//if (std::regex_match(_eval_token, STRING_COMPARISON))
						//if (std::regex_match(_eval_token, EVALUABLE_EXPR))
						//{
							//action_eval_->Eval(_eval_token, "Bio.Chromosome.DNA.Temp.Value");
							//Read("Bio.Chromosome.DNA.Temp.Value", _result);
						//}
						//else
						{
							_result = cond_eval_->Eval(_eval_token, false);
						}
						const std::regex TERNARY_OPERATOR_OF_STRING("^[^?]*? *\\? *['][^?]*['] *: *['][^?]*['] *$");
						//const std::regex TERNARY_OPERATOR_OF_STRING("^[^?]*? *\\? *[\"'][^?]*[\"'] *: *[\"'][^?]*[\"'] *$");			// for JSON, ignore \"
						if (_token.size() <= MAX_REGEXPR_SIZE && std::regex_match(_token, TERNARY_OPERATOR_OF_STRING))
						{
							if (_result)
							{
								//size_t _start_pos = _token.find_first_of("\"'", _question_pos + 1);		// for JSON, ignore \"
								//size_t _end_pos = _token.find_last_of("\"'", _splitter - 1);				// for JSON, ignore \"
								size_t _start_pos = _token.find_first_of("'", _question_pos + 1);
								size_t _end_pos = _token.find_last_of("'", _splitter - 1);
								_model_value = _token.substr(_start_pos + 1, _end_pos - _start_pos - 1);
							}
							else
							{
								//size_t _start_pos = _token.find_first_of("\"'", _splitter + 1);			// for JSON, ignore \"
								//size_t _end_pos = _token.find_last_of("\"'");								// for JSON, ignore \"
								size_t _start_pos = _token.find_first_of("'", _splitter + 1);
								size_t _end_pos = _token.find_last_of("'");
								_model_value = _token.substr(_start_pos + 1, _end_pos - _start_pos - 1);
							}
						}
						else
						{
							//action_eval_->Eval(std::to_string(_result) + _token.substr(_question_pos), "Bio.Chromosome.DNA.Temp.Value");
							action_eval_->Eval(_token, "Bio.Chromosome.DNA.Temp.Value");
							Read("Bio.Chromosome.DNA.Temp.Value", _model_value);
						}
					}
					else if (_token.size() <= MAX_REGEXPR_SIZE && std::regex_match(_token, EVALUABLE_EXPR))
					{
						const std::regex CONDITION_EXPR("(?!.*\\?)^.*(!=|==|>|<|<=|>=|&&|\\|\\|).*$");
						if (std::regex_match(_token, CONDITION_EXPR))
							_model_value = cond_eval_->Eval(_token) ? "true" : "false";
						else
						{
							action_eval_->Eval(_token, "Bio.Chromosome.DNA.Temp.Value");
							Read("Bio.Chromosome.DNA.Temp.Value", _model_value);
						}
					}
					else
					{
						_model_value = cond_eval_->Eval(_token) ? "true" : "false";
					}
					if (findAndReplaceAll(target, _tobe_replaced, _model_value) == false)
					{
						LOG_E(TAG, "Unsupported arithmetic operation!! expression: %s", _token.c_str());
						break;
					}
				}
				else
				{
					_start_pos = _token_end_pos + 1;
				}
				continue;
			}
			_end_pos = target.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.", _start_pos + 2);
			String _model_value;
			bool _found_in_model = false;
			if (_end_pos == String::npos)
			{
				_end_pos = target.size();
				String _model_name = target.substr(_start_pos + 2, _end_pos - _start_pos - 2);
				if (!_model_name.empty())
				{
					bool _default_model_name = false;
					Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
					if (_default_model_name)
						_model_value = String("::") + _model_name;
					else
						_model_value = "";
					Read(_model_name, _model_value);
					if (_model_value != String("::") + _model_name)
					{
						_found_in_model = true;
					}
				}
			}
			else if (target[_end_pos] == '[')
			{
				size_t _index_end_pos = FindRightBracket(target, _end_pos, std::make_pair('[', ']'));
				if (_index_end_pos == String::npos)
					_index_end_pos = target.size();
				if (_index_end_pos - _end_pos == 1)
				{	// getting size of array, JSON array or JSON object
					String _model_name;
					if (target.substr(_index_end_pos + 1, sizeof(".size") - 1) == ".size")
					{
						_model_name = target.substr(_start_pos + 2, _end_pos - _start_pos - 2);
						_index_end_pos = _end_pos + sizeof(".size");
					}
					else if (target.substr(_index_end_pos + 1, sizeof(".length") - 1) == ".length")
					{
						_model_name = target.substr(_start_pos + 2, _end_pos - _start_pos - 2);
						_index_end_pos = _end_pos + sizeof(".length");
					}
					if (!_model_name.empty())
					{
						bool _default_model_name = false;
						Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
						if (_default_model_name)
							_model_value = String("::") + _model_name;
						else
							_model_value = "";
						Array<String> _value_list;
						Read(_model_name, _value_list);
						if (_value_list.size() > 0
							&& (_value_list.front().size() == 0 || _value_list.front().front() != '{'
								|| _value_list.back().size() == 0 || _value_list.back().back() != '}'))
						{
							_model_value = std::to_string(_value_list.size());
							_found_in_model = true;
						}
						else
						{
							/*
							String _value;
							Read(_model_name, _value);
							json _root;
							String _escaped_value = "";
							if (!_value.empty())
							{
								size_t _first_quote = _value.find('\"');
								if (_first_quote != String::npos && _first_quote > 1 && _value[_first_quote - 1] == '\\')
								{
									_value = std::regex_replace(_value, std::regex(R"(\\\")"), "\"");
								}
								try
								{
									_root = json::parse(_value);
								}
								catch (const std::exception& e)
								{
									LOG_D(TAG, "Error when parsing \"%s\" as JSON, message=%s", _value.c_str(), e.what());
									EscapeJSON(_value, _escaped_value);
								}
							}
							if (_escaped_value != "")
							{
								try
								{
									_root = json::parse(_escaped_value);
									LOG_I(TAG, "JSON string escaped");
								}
								catch (const std::exception& e)
								{
									LOG_E(TAG, "Escaped!! Error when parsing %s as JSON, message=%s", _escaped_value.c_str(), e.what());
								}
							}
							*/
							json _root;
							Read(_model_name, _root);
							if (!_root.is_null())
							{	// JSON format
								if (_root.is_array() || _root.is_object())
								{
									_model_value = std::to_string(_root.size());
								}
								else
								{
									_model_value = "0";		// ex. ::AAA[::BBB][].length when it is empty
								}
								_found_in_model = true;
							}
							else
							{
								String _value, _escaped_value;
								Read(_model_name, _value);
								try
								{
									_root = json::parse(_value);
								}
								catch (const std::exception& e)
								{
									LOG_D(TAG, "Error when parsing %s as JSON, message=%s", _value.c_str(), e.what());
									EscapeJSON(_value, _escaped_value);
								}
								if (_escaped_value != "")
								{
									try
									{
										_root = json::parse(_escaped_value);
										LOG_I(TAG, "JSON string escaped");
									}
									catch (const std::exception& e)
									{
										LOG_E(TAG, "Escaped!! Error when parsing %s as JSON, message=%s", _escaped_value.c_str(), e.what());
									}
								}
								if (!_root.is_null())
								{	// JSON format
									if (_root.is_array() || _root.is_object())
									{
										_model_value = std::to_string(_root.size());
									}
									else if (_root.is_string())
									{
										String _value = _root.get<String>();
										_model_value = std::to_string(_value.size());
									}
									else if (_root.is_number_float())
									{
										//_model_value = std::to_string(std::to_string(_root.get<double>()).size());
										_model_value = std::to_string(_value.size());
									}
									else if (_root.is_number_integer())
									{
										_model_value = std::to_string(std::to_string(_root.get<int>()).size());
									}
									else if (_root.is_number_unsigned())
									{
										_model_value = std::to_string(std::to_string(_root.get<unsigned int>()).size());
									}
									else
									{
										_model_value = "0";		// ex. ::AAA[::BBB][].length when it is empty
									}
									_found_in_model = true;
								}
								else if (_escaped_value == "")
								{
									_model_value = std::to_string(_value.size());		// ex. ::AAA[].length when AAA is a string
									_found_in_model = true;
								}
							}
						}
					}
				}
				else
				{
					String _index_str = target.substr(_end_pos + 1, _index_end_pos - _end_pos - 1);
					if (_index_str.find("::") != String::npos && _ignored_range_list.find(std::make_pair(_end_pos + 1, _index_end_pos)) == _ignored_range_list.end())
					{
						if (_ignored_token_list.find(_index_str) == _ignored_token_list.end())
						{
							_active_range.push(std::make_pair(_end_pos + 1, _index_end_pos));
							_ignored_range_list.insert(std::make_pair(_end_pos + 1, _index_end_pos));
							_ignored_token_list.insert(_index_str);
							break;
						}
						else
						{
							_start_pos = _index_end_pos;
							continue;
						}
					}
					//ReplaceModelNameWithValue(_index_str);
					//action_eval_->Eval(((String)"Bio.Chromosome.DNA.Temp.Index=" + _index_str).c_str());
					action_eval_->Eval(_index_str, "Bio.Chromosome.DNA.Temp.Index");
					String _model_name = target.substr(_start_pos + 2, _end_pos - _start_pos - 2);
					if (!_model_name.empty())
					{
						bool _default_model_name = false;
						Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
						if (_default_model_name)
							_model_value = String("::") + _model_name;
						else
							_model_value = "";
						Array<String> _value_list;
						Read(_model_name, _value_list);
						if (_value_list.size() > 0
							&& (_value_list.front().size() == 0 || (_value_list.front().front() != '{' && _value_list.front().front() != '[')
								|| _value_list.back().size() == 0 || (_value_list.back().back() != '}' && _value_list.back().back() != ']')))
						{
							//int _index = std::stoi(_index_str);
							int _index = 0;
							Read("Bio.Chromosome.DNA.Temp.Index", _index);
							if (_index < 0)
							{
								_index = _index + (int)_value_list.size() >= 0 ? _index + (int)_value_list.size() : 0;
							}
							if (_value_list.size() > _index)
							{
								_found_in_model = true;

								if (target.size() > _index_end_pos + 1 && target[_index_end_pos + 1] == '.')
								{	// try protobuf
									size_t _break_pos = _model_name.find_last_of('.');
									Write("Bio.Chromosome.DecomposeMessage.message_name", _model_name.substr(0, _break_pos));
									Write("Bio.Chromosome.DecomposeMessage.field_name", _model_name.substr(_break_pos + 1));
									Write("Bio.Chromosome.DecomposeMessage.payload", _value_list[_index]);
									owner()->do_event("Bio.Chromosome.DecomposeMessage");
									//owner()->on_event("", _model_name, _value_list[_array_index]);
									size_t _field_end_pos = target.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.", _index_end_pos + 1);
									if (_field_end_pos == String::npos)
										_field_end_pos = target.size() - 1;
									else
										_field_end_pos--;
									_model_name += target.substr(_index_end_pos + 1, _field_end_pos - _index_end_pos);
									Read(_model_name, _model_value);
									_index_end_pos = _field_end_pos;
								}
								else
								{
									_model_value = _value_list[_index];
								}
							}
							//_end_pos = _index_end_pos + 1;
						}
						else
						{
							String _value;
							Read(_model_name, _value);

							if (resolved_model_name == _model_name)
							{
								if (!resolved_model_name.empty())
									LOG_W(TAG, "Not suppored recursive replacing of model name %s", _model_name.c_str());
								_start_pos = _end_pos;
								continue;
							}
							else
							{	// modified for replacing model name with [...] in JSON
								if (resolved_model_cache_.count(_model_name) > 0)
									_value = resolved_model_cache_[_model_name];
								else
								{ 
									if (recursive)
									{
										Write("Bio.Cell.Model.DefaultReturnModelname", true);
										ReplaceModelNameWithValue(_value, 0, String::npos, _model_name);
										Write("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
									}
									resolved_model_cache_[_model_name] = _value;
								}
							}
							//_model_value = _root[_index_str];


							/*
								(sample 1, assign) ::JustDoIt.Project.item[items][1][items][4][content] ::JustDoIt.SourceCodeDescMap["::JustDoIt.GetProjectsByCondition.Result.sourceCode"]
								(sample 2, cond) ::YJInfo.Task.MemberCollection[::YJInfo.LayoutTemplate.layout[layout][0][items][MODIFYTASK_MEMBER][items][::index][id]] = "true"
								(sample 3, cond) ::index<::JustDoIt.LayoutTemplate.layout[layout][PRODUCER_PROJECTLIST_ROOT][items][PRODUCER_PROJECTITEM_ROOT][items][::group_index][items][].length - 1
							*/

							//size_t _last_right_bracket = target.rfind(']');
							//String _path_str = target.substr(_end_pos, _last_right_bracket - _end_pos + 1);
							//String _last_str = target.substr(_last_right_bracket + 1);
							size_t _path_end_pos = _index_end_pos;
							if (_path_end_pos < target.size() - 1)
								_path_end_pos++;
							else
								_path_end_pos = String::npos;
							//while ((_path_end_pos = target.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_\"", _path_end_pos)) != String::npos)
							while ((_path_end_pos = target.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", _path_end_pos)) != String::npos)
							{
								if (target[_path_end_pos] == '[')
								{
									_path_end_pos = FindRightBracket(target, _path_end_pos, std::make_pair('[', ']'));
									if (_path_end_pos < target.size() - 1)
										_path_end_pos++;
									else
										_path_end_pos = String::npos;
								}
								else
								{
									size_t _last_bracket_pos = String::npos;
									if (target[_path_end_pos - 1] != ']')	// for "::a1.b1.c1[x1][y1]::a2.b2.c2[x2][y2]" without space as delimeters
									{
										_last_bracket_pos = target.rfind(']', _path_end_pos - 1);
										if (_last_bracket_pos > _end_pos)
											_path_end_pos = _last_bracket_pos + 1;
									}
									break;
								}
							};
							String _path_str = target.substr(_end_pos, _path_end_pos - _end_pos);
							/*
							if (_path_str.find("::") != String::npos && _ignored_range_list.find(std::make_pair(_end_pos, _path_end_pos)) == _ignored_range_list.end())
							{
								if (_ignored_token_list.find(_path_str) == _ignored_token_list.end())
								{
									_active_range.push(std::make_pair(_end_pos, _path_end_pos));
									_ignored_range_list.insert(std::make_pair(_end_pos, _path_end_pos));
									_ignored_token_list.insert(_path_str);
									break;
								}
								else
								{
									_start_pos = _path_end_pos;
									continue;
								}
							}
							*/
							String _last_str = "";
							if (_path_end_pos < target.size())
								_last_str = target.substr(_path_end_pos);
							/*
							if (_last_str.find("::") != String::npos && _ignored_range_list.find(std::make_pair(_path_end_pos, String::npos)) == _ignored_range_list.end())
							{
								_active_range.push(std::make_pair(_path_end_pos, String::npos));
								_ignored_range_list.insert(std::make_pair(_path_end_pos, String::npos));
								break;
							}
							*/
							if (recursive)
							{
								ReplaceModelNameWithValue(_path_str);
								ReplaceModelNameWithValue(_last_str);
							}
							target = target.substr(0, _end_pos) + _path_str + _last_str;
							bool _is_get_size = false;
							bool _is_deep_path = false;
							if (std::count(_path_str.begin(), _path_str.end(), '[') > 1)
							{
								_is_deep_path = true;
								//size_t _path_end_pos = _path_str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.\"[]");
								//size_t _path_end_pos = _path_str.rfind(']');
								if (_last_str.size() >= sizeof(".size") - 1 && _last_str.substr(0, sizeof(".size") - 1) == ".size")
								{
									_is_get_size = true;
									//_path_str = _path_str.substr(0, _path_end_pos);
									_index_end_pos = _end_pos + _path_str.size() + (sizeof(".size") - 1) - 1;
								}
								else if (_last_str.size() >= sizeof(".length") - 1 && _last_str.substr(0, sizeof(".length") - 1) == ".length")
								{
									_is_get_size = true;
									//_path_str = _path_str.substr(0, _path_end_pos);
									_index_end_pos = _end_pos + _path_str.size() + (sizeof(".length") - 1) - 1;
								}
								else
								{
									//_path_str = _path_str.substr(0, _path_end_pos);
									_index_end_pos = _end_pos + _path_str.size() - 1;
									//_index_end_pos = _end_pos + _path_str.size();
								}
							}
							else
							{
								_index_end_pos = _end_pos + _path_str.size() - 1;
							}
							_path_str = std::regex_replace(_path_str, std::regex(R"(\\\")"), "");
							_path_str.erase(std::remove(_path_str.begin(), _path_str.end(), '\"'), _path_str.end());
							_path_str.erase(std::remove(_path_str.begin(), _path_str.end(), ']'), _path_str.end());
							std::replace(_path_str.begin(), _path_str.end(), '[', '/');
							ReplacePathIndex(_path_str);
							if (_path_str.back() == '/')
								_path_str.pop_back();
							if (_is_deep_path == false)
							{
								if (_path_str.size() > sizeof(".size") && _path_str.substr(_path_str.size() - sizeof(".size") + 1, sizeof(".size") - 1) == ".size")
								{
									_path_str = _path_str.substr(0, _path_str.size() - sizeof(".size") + 1);
									_is_get_size = true;
								}
								else if (_path_str.size() > sizeof(".length") && _path_str.substr(_path_str.size() - sizeof(".length") + 1, sizeof(".length") - 1) == ".length")
								{
									_path_str = _path_str.substr(0, _path_str.size() - sizeof(".length") + 1);
									_is_get_size = true;
								}
							}
							if (_is_get_size == true)
							{
								//_model_name = target.substr(_start_pos + 2, _end_pos - _start_pos - 2);
								//if (!_model_name.empty())
								{
									bool _default_model_name = false;
									Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
									if (_default_model_name)
										_model_value = String("::") + _model_name;
									else
										_model_value = "";

									json _root;
									String _escaped_value = "";
									if (!_value.empty())
									{
										size_t _first_quote = _value.find('\"');
										if (_first_quote != String::npos && _first_quote > 1 && _value[_first_quote - 1] == '\\')
										{
											_value = std::regex_replace(_value, std::regex(R"(\\\")"), "\"");
										}
										try
										{
											_root = json::parse(_value);
										}
										catch (const std::exception& e)
										{
											LOG_D(TAG, "Error when parsing %s as JSON, message=%s", _value.c_str(), e.what());
											EscapeJSON(_value, _escaped_value);
										}
									}
									if (_escaped_value != "")
									{
										try
										{
											_root = json::parse(_escaped_value);
											LOG_I(TAG, "JSON string escaped");
										}
										catch (const std::exception& e)
										{
											LOG_E(TAG, "Escaped!! Error when parsing %s as JSON, message=%s", _escaped_value.c_str(), e.what());
										}
									}
									if (!_root.is_null())
									{	// JSON format
										json::json_pointer _target_ptr(_path_str);
										if (_root[_target_ptr].is_array() || _root[_target_ptr].is_object())
										{
											_model_value = std::to_string(_root[_target_ptr].size());
										}
										else if (_root[_target_ptr].is_string())
										{
											String _value = _root[_target_ptr].get<String>();
											_model_value = std::to_string(_value.size());
										}
										else
										{
											_model_value = "0";		// ex. ::AAA[::BBB][].length when it is empty
										}
										_found_in_model = true;
									}
								}
							}
							else
							{
								json _root;
								String _escaped_value = "";
								if (!_value.empty())
								{
									size_t _first_quote = _value.find('\"');
									if (_first_quote != String::npos && _first_quote > 1 && _value[_first_quote - 1] == '\\')
									{
										_value = std::regex_replace(_value, std::regex(R"(\\\")"), "\"");
									}
									const String WHITE_SPACES(" \t\f\v\n\r");
									if (_value.size() >= 2 && (_value[_value.find_first_not_of(WHITE_SPACES)] == '{' && _value[_value.find_last_not_of(WHITE_SPACES)] == '}' || _value[_value.find_first_not_of(WHITE_SPACES)] == '[' && _value[_value.find_last_not_of(WHITE_SPACES)] == ']'))
									{
										try
										{
											_root = json::parse(_value);
										}
										catch (const std::exception& e)
										{
											LOG_D(TAG, "Error message \"%s\" when parsing %s as JSON, ", e.what(), _value.c_str());
											EscapeJSON(_value, _escaped_value);
										}
									}
								}
								if (_escaped_value != "")
								{
									try
									{
										_root = json::parse(_escaped_value);
										LOG_I(TAG, "JSON string escaped");
									}
									catch (const std::exception& e)
									{
										LOG_E(TAG, "Escaped!! Error message \"%s\" when parsing %s as JSON, ", e.what(), _escaped_value.c_str());
										throw e;
									}
								}
								if (!_root.is_null())
								{	// JSON format
									String _orig_path_str = _path_str;
									bool _continue = true;
									while (_continue)
									{
										json::json_pointer _target_ptr(_path_str);
										try
										{
											if (_target_ptr.back() == "*")
												return;
											else if (!_root.contains(_target_ptr))
											{
												Array<String> _path_array = split(_path_str, "/");
												_path_str = "";
												for (auto elem : _path_array)
												{
													const std::regex NEGATIVE_INTEGER(R"(^-\d+$)");
													_target_ptr = json::json_pointer(_path_str);
													if (std::regex_match(elem, NEGATIVE_INTEGER))
													{
														int _index = std::stoi(elem);
														if (_root[_target_ptr].is_array())
														{
															_index = _index + (int)_root[_target_ptr].size() >= 0 ? _index + (int)_root[_target_ptr].size() : 0;
															_path_str += "/" + std::to_string(_index);
														}
														else
														{
															_path_str += "/" + elem;
														}
													}
													else
													{
														_path_str += "/" + elem;
													}
												}
												_target_ptr = json::json_pointer(_path_str);
												if (!_root.contains(_target_ptr))
												{
													_model_value = "";
													_found_in_model = true;
													_continue = false;
													continue;
												}
												else
												{
													_orig_path_str = _path_str;
												}
											}
											if (_path_str == _orig_path_str)
											{
												_model_value = _root[_target_ptr].dump();
												_found_in_model = true;
												_continue = false;
											}
											else
											{
												json _subtree;
												switch (_root[_target_ptr].type())
												{
												case json::value_t::string:
												{
													String _subtree_dump = _root[_target_ptr].get<String>();
													_subtree = json::parse(_subtree_dump);
													break;
												}
												case json::value_t::array:
												case json::value_t::object:
													_subtree = _root[_target_ptr];
													break;
												default:
													_continue = false;
													break;
												}
												String _new_path = _orig_path_str.substr(_path_str.size());
												json::json_pointer _subtree_target_ptr(_new_path);
												_model_value = _subtree[_subtree_target_ptr].dump();
												_found_in_model = true;
												_continue = false;
											}
										}
										catch (const json::exception& e)
										{
											if (!_default_model_name)
												LOG_W(TAG, "no such path %s in %s, exception:%s", _path_str.c_str(), _root.dump().c_str(), e.what());
											size_t _pos = _path_str.find_last_of('/');
											if (_pos != String::npos && _pos != 0)
												_path_str = _path_str.substr(0, _pos);
											else
												_continue = false;
										}
									}
								}
							}
						}
					}
				}
				_end_pos = _index_end_pos + 1;
			}
			else if (target[_end_pos - 1] == '.')
			{
				if (_ignored_range_list.find(std::make_pair(_end_pos, _range_end)) == _ignored_range_list.end())
				{
					_active_range.push(std::make_pair(_end_pos, _range_end));
					_ignored_range_list.insert(std::make_pair(_end_pos, _range_end));
				}
			}
			else
			{
				String _model_name = target.substr(_start_pos + 2, _end_pos - _start_pos - 2);
				if (!_model_name.empty())
				{
					bool _default_model_name = false;
					Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
					if (_default_model_name)
						_model_value = String("::") + _model_name;
					else
						_model_value = "";
					Read(_model_name, _model_value);
					if (_model_value != String("::") + _model_name)
					{
						if (resolved_model_name == _model_name)
						{
							if (!resolved_model_name.empty())
								LOG_W(TAG, "Not suppored recursive replacing of model name %s", _model_name.c_str());
						}
						else
						{
							if (resolved_model_cache_.count(_model_name) > 0)
								_model_value = resolved_model_cache_[_model_name];
							else
							{
								if (recursive)
								{
									ReplaceModelNameWithValue(_model_value, 0, String::npos, _model_name);		// for nested model names
								}
								resolved_model_cache_[_model_name] = _model_value;
							}
							_found_in_model = true;
						}
					}
				}
			}

			if (_found_in_model)
			{
				String _tobe_replaced = target.substr(_start_pos, _end_pos - _start_pos);
				if (findAndReplaceAll(target, _tobe_replaced, _model_value) == false)
				{
					LOG_E(TAG, "Unsupported nested protobuf format or string concatenation without any delimeter!! model name: %s", target.substr(_start_pos, _end_pos - _start_pos).c_str());
					break;
				}

				if (!recursive)
					_start_pos += _model_value.size() - (sizeof("::") - 1);		// if replaced, search from the start for nested model names

				if (_model_value.find('\0') != String::npos)		// break if binary data
				{
					while (!_active_range.empty())
						_active_range.pop();
					break;
				}
			}
			else
			{
				_start_pos = _end_pos;
			}
		}
		if (_start_pos >= _range_end)
			_active_range.pop();
	}
	// Don't do this. It will cause unpredictable issues. You need do it by yourself at the right time.
	//std::regex_replace(target, std::regex(u8"�G�G"), "::");
}

void DNA::deepAssign(const String& map_key, const String& map_value, size_t left_bracket_pos)
{
	size_t _right_bracket_pos = map_key.find(']', left_bracket_pos);
	if (_right_bracket_pos != String::npos)
	{
		String _key = map_key.substr(0, left_bracket_pos);
		json _root;
		Read(_key, _root);
		{	// deep assign to JSON path before converting without touching anything not known
			bool _default_model_name = false;
			Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
			Write("Bio.Cell.Model.DefaultReturnModelname", true);
			resolved_model_cache_.clear();
			//ReplaceModelNameWithValue(_serialized_str);
			Write("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
		}
		String _path_str = map_key.substr(left_bracket_pos);
		bool _is_array = false;
		bool _is_append = false;
		if (_path_str[_path_str.size() - 2] == '[' && _path_str.back() == ']')
			_is_array = true;
		else if (_path_str[_path_str.size() - 2] == '{' && _path_str.back() == '}')
		{
			_is_append = true;
			_path_str.pop_back();
			_path_str.pop_back();
		}

		_path_str.erase(std::remove(_path_str.begin(), _path_str.end(), '\"'), _path_str.end());
		_path_str.erase(std::remove(_path_str.begin(), _path_str.end(), ']'), _path_str.end());
		std::replace(_path_str.begin(), _path_str.end(), '[', '/');
		ReplacePathIndex(_path_str);
		if (_path_str.back() == '/')
			_path_str.pop_back();

		if (!_root.is_null())
		{	// JSON format
			try
			{
				json* _obj = nullptr;
				String _key = "";
				FindAndUnserialize(_root, _path_str, &_obj, _key);
			}
			catch (const std::exception& e)
			{
				LOG_D(TAG, "Error when finding path %s, exception: %s", _path_str.c_str(), e.what());
			}

			json::json_pointer _new_value_ptr(_path_str);
			if (!_is_array)
			{
				if (map_value.empty())
				{
					LOG_D(TAG, "model with key name \"%s\" has been removed", map_key.c_str());
					RemoveElement(_root, _path_str, true);
				}
				else if (map_value.size() >= 2 && (map_value.front() == '{' && map_value.back() == '}'
					|| map_value.front() == '[' && map_value.back() == ']'))
				{
					json _removed_data = json();
					if (!_is_append)
					{
						RemoveElement(_root, _path_str);
					}
					// Don't remove if map_value = "{}" or "[]". To remove it, please use ""
					//if (map_value != "{}" && map_value != "[]")
					{
						json _new_value;
						String _escaped_value = "";
						if (!map_value.empty())
						{
							try
							{
								_new_value = json::parse(map_value);
							}
							catch (const std::exception& e)
							{
								LOG_D(TAG, "Error when parsing %s as JSON, message=%s", map_value.c_str(), e.what());
								EscapeJSON(map_value, _escaped_value);
							}
						}
						if (_escaped_value != "")
						{
							try
							{
								_new_value = json::parse(_escaped_value);
								LOG_I(TAG, "JSON string escaped");
							}
							catch (const std::exception& e)
							{
								LOG_E(TAG, "Escaped!! Error when parsing %s as JSON, message=%s", _escaped_value.c_str(), e.what());
							}
						}
						if (!_root.contains(_new_value_ptr))
							CorrectNegativeIndex(_key, _root, _path_str, _new_value_ptr);
						if (_root[_new_value_ptr].is_array())
						{
							if (_new_value.is_array())
							{
								for (const auto& elem : _new_value)
									_root[_new_value_ptr].push_back(elem);
							}
							else
							{
								_root[_new_value_ptr].push_back(_new_value);
							}
						}
						else if (_root[_new_value_ptr].is_object())
						{
							//_root[_new_value_ptr].push_back(_new_value);
							for (auto itr = _new_value.begin(); itr != _new_value.end(); ++itr)
							{
								if (_root[_new_value_ptr].contains(itr.key()))
								{
									_root[_new_value_ptr][itr.key()] = itr.value();
								}
								else
								{
									_root[_new_value_ptr].push_back(json::object_t::value_type(itr.key(), itr.value()));
								}
							}
						}
						else
						{
							_root[_new_value_ptr] = _new_value;
						}
					}
				}
				else
				{
					json _removed_data = RemoveElement(_root, _path_str);
					if (map_value.find("::") == String::npos)
					{	// "JSON" or 'JSON'
						if (!_root.contains(_new_value_ptr))
							CorrectNegativeIndex(_key, _root, _path_str, _new_value_ptr);
						String _map_value;
						if ((map_value.front() == '\"' || map_value.front() == '\'') && map_value.front() == map_value.back())
							_map_value = map_value.substr(1, map_value.size() - 2);
						else
							_map_value = map_value;
						switch (_removed_data.type())
						{
						case json::value_t::number_integer:
						case json::value_t::number_unsigned:
							_root[_new_value_ptr] = stoi(_map_value);
							break;
						case json::value_t::number_float:
							_root[_new_value_ptr] = stod(_map_value);
							break;
						case json::value_t::string:
						case json::value_t::array:
						case json::value_t::object:
							_root[_new_value_ptr] = _map_value;
							break;
						case json::value_t::boolean:
							_root[_new_value_ptr] = (_map_value == "true");
							break;
						default:
							_root[_new_value_ptr] = _map_value;
							break;
						}
					}
				}
			}
			else
			{
				if (!map_value.empty())			// ignore if map_value is empty
				{
					if (map_value.size() >= 2 && (map_value.front() == '{' && map_value.back() == '}'
						|| map_value.front() == '[' && map_value.back() == ']'))
					{
						// Don't remove if map_value = "{}" or "[]". To remove it, please use ""
						//if (map_value == "{}" || map_value == "[]")
						//{
						//	RemoveElement(_root, _path_str);
						//}
						//else
						{
							json _new_value;
							String _escaped_value = "";
							if (!map_value.empty())
							{
								try
								{
									_new_value = json::parse(map_value);
								}
								catch (const std::exception& e)
								{
									LOG_D(TAG, "Error when parsing %s as JSON, message=%s", map_value.c_str(), e.what());
									EscapeJSON(map_value, _escaped_value);
								}
							}
							if (_escaped_value != "")
							{
								try
								{
									_new_value = json::parse(_escaped_value);
									LOG_I(TAG, "JSON string escaped");
								}
								catch (const std::exception& e)
								{
									LOG_E(TAG, "Escaped!! Error when parsing %s as JSON, message=%s", _escaped_value.c_str(), e.what());
								}
							}
							int i = -1;
							try
							{
								if (!_root.contains(_new_value_ptr))
									CorrectNegativeIndex(_key, _root, _path_str, _new_value_ptr);
								if (_root[_new_value_ptr].is_null())
									_root[_new_value_ptr] = json::array();
								if (_new_value.is_array())
								{
									if (!_new_value.empty())
									{
										for (const auto& elem : _new_value)
											_root[_new_value_ptr].push_back(elem);
									}
									else
									{
										_root[_new_value_ptr].push_back(json::array());
									}
								}
								else if (_new_value.is_object())
								{
									_root[_new_value_ptr].push_back(_new_value);
								}
							}
							catch (const std::exception& e)
							{
								LOG_E(TAG, "exception (%s) was thrown when push_back(%s)", e.what(), _new_value.dump().c_str());
							}
						}
					}
					else
					{
						if (!_root.contains(_new_value_ptr))
							CorrectNegativeIndex(_key, _root, _path_str, _new_value_ptr);
						String _map_value;
						if ((map_value.front() == '\"' || map_value.front() == '\'') && map_value.front() == map_value.back())
							_map_value = map_value.substr(1, map_value.size() - 2);
						else
							_map_value = map_value;

						switch (_root[_new_value_ptr].size()>0?_root[_new_value_ptr].back().type():json::value_t::string)
						{
						case json::value_t::number_integer:
						case json::value_t::number_unsigned:
							_root[_new_value_ptr].push_back(stoi(_map_value));
							break;
						case json::value_t::number_float:
							_root[_new_value_ptr].push_back(stod(_map_value));
							break;
						case json::value_t::string:
						case json::value_t::array:
						case json::value_t::object:
							_root[_new_value_ptr].push_back(_map_value);
							break;
						case json::value_t::boolean:
							_root[_new_value_ptr].push_back(_map_value == "true");
							break;
						default:
							break;
						}
					}
				}
			}
			Write(_key, _root);
		}
		else
		{	// protobuf format
			// get type of the field from schema
			// assign the value by the field type
			size_t _pos = _key.find_last_of('.');
			if (_pos == String::npos || _pos == _key.size() - 1)
				return;
			Write("Bio.Chromosome.AssignByMessage.model_name", _key.substr(0, _pos));
			Write("Bio.Chromosome.AssignByMessage.field_name", _key.substr(_pos + 1));
			Array<String> _path = split(_path_str.substr(1, _is_array ? _path_str.size() - 1 : _path_str.size()), "/");
			Write("Bio.Chromosome.AssignByMessage.path", _path);
			Write("Bio.Chromosome.AssignByMessage.value", map_value);
			owner()->do_event("Bio.Chromosome.AssignByMessage");
		}
	}
}

void DNA::ReplacePathIndex(String& path_str)
{
	int _decimal_count = 0;
	Read("Bio.Cell.Model.NumberWithDecimalCount", _decimal_count);
	Write("Bio.Cell.Model.NumberWithDecimalCount", 0);
	// find and eval indexes
	size_t _left = path_str.find_first_not_of('/');
	size_t _right = path_str.find('/', _left);
	while (_left != String::npos)
	{
		Write("Bio.Chromosome.DNA.Temp.Index", "");
		String _index_str = path_str.substr(_left, _right - _left);
		if (_index_str != "" && cond_eval_->findOperator(_index_str) == false)
		{
			//action_eval_->Eval(((String)"Bio.Chromosome.DNA.Temp.Index=" + _index_str).c_str());
			action_eval_->Eval(_index_str, "Bio.Chromosome.DNA.Temp.Index");
			String _new_index_str;
			Read("Bio.Chromosome.DNA.Temp.Index", _new_index_str);
			if (_new_index_str != _index_str)
			{
				path_str.replace(path_str.find(_index_str), _index_str.size(), _new_index_str);
				if (_right != String::npos)
					_right += _new_index_str.size() - _index_str.size();
			}
		}
		if (_right != String::npos)
		{
			_left = _right + 1;
			_right = path_str.find('/', _left);
		}
		else
		{
			_left = _right;
		}
	}
	Write("Bio.Cell.Model.NumberWithDecimalCount", _decimal_count);
}

bool DNA::FindAndUnserialize(json& root, const String& path_str, json** obj, String& key)
{
	Array<String> _path_list = split(path_str, "/");
	*obj = &root;
	key = "";
	bool _found = false;
	for (int i = 0; i < _path_list.size(); i++)
	{
		if ((*obj)->is_string())
			*(*obj) = json::parse((*obj)->get<String>());
		if ((*obj)->is_object())
		{
			if (!(*obj)->contains(_path_list[i]))
				break;
			else if (i == _path_list.size() - 1)
			{
				key = _path_list[i];
				_found = true;
			}
			else
			{
				(*obj) = &((*obj)->at(_path_list[i]));
				key = _path_list[i];
			}
		}
		else if ((*obj)->is_array() && is_number(_path_list[i]))
		{
			int _index = stoi(_path_list[i]);
			if (_index >= (int)(*obj)->size())
				break;
			else if (i == _path_list.size() - 1)
			{
				key = _path_list[i];
				_found = true;
			}
			else
			{
				(*obj) = &((*obj)->at(_index));
				key = _path_list[i];
			}
		}
	}
	return _found;
}

nlohmann::json DNA::RemoveElement(json& root, const String& path_str, bool forever)
{
	json *_obj = nullptr, _ret_val = json();
	String _key;
	bool _found = FindAndUnserialize(root, path_str, &_obj, _key);
	if (_obj->is_array())
	{
		int _index = stoi(_key);
		if (_index < 0)
		{
			_index = _index + (int)_obj->size() >= 0 ? _index + (int)_obj->size() : 0;
		}
		if (_index >= 0)
		{
			_ret_val = (*_obj)[_index];
			if (forever)
				_obj->erase(_index);
			else
				(*_obj)[_index] = json();
		}
	}
	else if (_obj->is_object() && _key != "")
	{
		_ret_val = (*_obj)[_key];
		_obj->erase(_key);
	}
	return _ret_val;
}

size_t DNA::FindRightBracket(const String& target, size_t pos, const Pair<char, char>& bracket)
{
	size_t _ret = pos;
	int _left_bracketcount = 1;
	int _right_bracket_count = 0;
	while (_right_bracket_count < _left_bracketcount)
	{
		_ret = target.find_first_of(bracket.second, _ret + 1);
		if (_ret < target.size())
		{
			_left_bracketcount = (int)std::count(target.begin() + pos, target.begin() + _ret + 1, bracket.first);
			_right_bracket_count = (int)std::count(target.begin() + pos, target.begin() + _ret + 1, bracket.second);
		}
		else
		{
			LOG_W(TAG, "Brackets are not paired for %s", target.c_str());
			break;
		}
	}
	return _ret;
}

void DNA::get_state_list(Array<String>& state_list)
{
	const int MAX_STATE_COUNT = 1024;
	const char* _active_state_list[MAX_STATE_COUNT];
	for (int i = 0; i < MAX_STATE_COUNT; i++)
		_active_state_list[i] = nullptr;
	state_machine_.GetActiveStates(_active_state_list, MAX_STATE_COUNT);
	for (int i = 0; i < MAX_STATE_COUNT; i++)
	{
		if (_active_state_list[i] == nullptr)
			break;
		else
			state_list.push_back(_active_state_list[i]);
	}
}

void DNA::set_state_list(const Array<String>& state_list)
{
	const int MAX_STATE_COUNT = 1024;
	const char* _active_state_list[MAX_STATE_COUNT];
	for (int i = 0; i < MAX_STATE_COUNT; i++)
		_active_state_list[i] = nullptr;
	for (int i = 0; i < state_list.size(); i++)
	{
		_active_state_list[i] = (char*)state_list[i].c_str();
	}
	state_machine_.SetActiveStates(_active_state_list, (int)state_list.size());
}

template <typename T>
String DNA::to_string_with_precision(const T a_value, const int n)
{
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return out.str();
}

void DNA::CorrectNegativeIndex(const String& target, const json& root, const String& path_str, json::json_pointer& new_value_ptr)
{
	Array<String> _path_array = split(path_str, "/");
	String _path_str = "";
	for (auto elem : _path_array)
	{
		const std::regex NEGATIVE_INTEGER(R"(^-\d+$)");
		new_value_ptr = json::json_pointer(_path_str);
		if (std::regex_match(elem, NEGATIVE_INTEGER))
		{
			int _index = std::stoi(elem);
			if (root[new_value_ptr].is_array())
			{
				_index = _index + (int)root[new_value_ptr].size() >= 0 ? _index + (int)root[new_value_ptr].size() : 0;
				_path_str += "/" + std::to_string(_index);
			}
			else
			{
				_path_str += "/" + elem;
			}
		}
		else
		{
			_path_str += "/" + elem;
		}
	}
	new_value_ptr = json::json_pointer(_path_str);
	//if (!root.contains(new_value_ptr))
	//{
	//	LOG_E(TAG, "Invalid path %s of %s", _path_str.c_str(), target.c_str());
	//	return;
	//}
}