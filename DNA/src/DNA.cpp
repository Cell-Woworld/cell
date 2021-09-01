#include "internal/DNA.h"
#include "internal/IModel.h"
#include "internal/utils/serializer.h"
#include "internal/utils/cell_utils.h"
#include "compile_time.h"
#include "ConditionEval.hpp"
#include "ActionEval.hpp"
#include <regex>
#include <locale>

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

enum ENUM_TAG
{
	RAISE = 0,
	SEND,
	SCRIPT,
	ASSIGN,
	LOG,
	INVOKE,
	UNINVOKE,
	FINAL_STATE,
};

enum type_id_table_
{
	type_bool,
	type_int32_t,
	type_uint32_t,
	type_int64_t,
	type_uint64_t,
	type_double,
	type_string,

	type_bool_array,
	type_int32_t_array,
	type_uint32_t_array,
	type_int64_t_array,
	type_uint64_t_array,
	type_double_array,
	type_string_array,
};

bool DNA::discard_no_such_model_warning_ = false;

DNA::DNA(IBiomolecule* owner)
	: IBiomolecule(owner)
	, cond_eval_(Obj<ConditionEval>(new ConditionEval(this)))
	, action_eval_(Obj<ActionEval>(new ActionEval(this)))
{
	BuildTypeHastable();
	String _default_value = "";
	Read("Bio.Cell.Model.DefaultReturnModelname", _default_value);
	if (_default_value == "")
		Write("Bio.Cell.Model.DefaultReturnModelname", false);
}

DNA::~DNA()
{
}

void DNA::init(const char* name)
{
	IBiomolecule::init("DNA");
	state_machine_.Load(name, this);
	SaveVersion();
	Read("Bio.Cell.Model.DiscardNoSuchModelWarning", discard_no_such_model_warning_);
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
		if (payload.size() > 1 || payload.size() == 1 && payload.data()[0] != '\0')
			Write(String("encode.") + msg_name.str(), payload.str());
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
	Write("Bio.Cell.Current.Action", String(name));
	Write("Bio.Cell.Current.ActionType", type);
	BioSys::IBiomolecule* _src = nullptr;
	switch (type)
	{
	case RAISE:
		_src = this;
	case SEND:
		for (auto _elem : params)
		{
			if (_elem.second.size() > 2 && _elem.second[0] == ':' && _elem.second[1] == ':' && _elem.second.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.", 2) == String::npos)
			{
				String _value;
				Read(_elem.second.substr(2), _value);
				ReplaceModelNameWithValue(_value);

				bool _default_model_name = false;
				Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
				String _target_model_name = String(name) + "." + _elem.first;
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
			else
			{
				ReplaceModelNameWithValue(_elem.second);
				if (_elem.second.size() >= 2)
				{
					// remove double quote only for single quote being database's keyword 
					//if (_elem.second.size() > 2 && (_elem.second.front() == '\"' || _elem.second.front() == '\'') && _elem.second.front() == _elem.second.back())
					if (_elem.second.size() > 2 && _elem.second.front() == '\"' && _elem.second.front() == _elem.second.back())
					{
						Write(String(name) + "." + _elem.first, _elem.second.substr(1, _elem.second.size() - 2));
					}
					else if ((_elem.second.front() != '[' && _elem.second.back() != ']')
						&& (_elem.second.front() != '{' && _elem.second.back() != '}')
						&& action_eval_->findOperator(_elem.second, true) == true)
					{
						//action_eval_->Eval((String(name) + "." + _elem.first + "=" + _elem.second).c_str());
						action_eval_->Eval(_elem.second, String(name) + "." + _elem.first);
					}
					else
					{
						Write(String(name) + "." + _elem.first, _elem.second);
					}
				}
				else
				{
					Write(String(name) + "." + _elem.first, _elem.second);
				}
			}
		}
		owner()->add_event(name, "", _src);
		Remove((String)name + ".*");
		break;
	case SCRIPT:
		if (action_eval_->findOperator(name) == true)
		{
			action_eval_->Eval(name);
		}
		else
		{
			WriteParams(name, params);
			owner()->do_event(name);
			//Remove(name + (String)".*");			// Don't do it. Let Chromosome do.
			RemoveParams(name, params);
		}
		break;
	case ASSIGN:
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
						Write(_elem.first, _value);
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
						if (_default_model_name)
							Pushback(_elem.first.substr(0, _pos), _elem.second.substr(2), _elem.second);
						else
							Pushback(_elem.first.substr(0, _pos), _elem.second.substr(2));
					}
					else
					{
						String _value = "";
						if (_default_model_name)
							_value = _elem.second;
						Read(_elem.second.substr(2), _value);
						deepAssign(_elem.first, _value, _pos);
					}
				}
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
					if (!std::regex_match(_elem.second, PURE_NUMBER))
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
							String _serialized_str = "";
							String _key = _elem.first.substr(0, _left_brace_pos);
							Read(_key, _serialized_str);
							json _root;
							json _object;
							String _escaped_value = "";
							if (!_serialized_str.empty())
							{
								try
								{
									_root = json::parse(_serialized_str);
								}
								catch (const std::exception& e)
								{
									LOG_D(TAG, "Error when parsing %s as JSON, message=%s", _serialized_str.c_str(), e.what());
									EscapeJSON(_serialized_str, _escaped_value);
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
							_escaped_value = "";
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
							Write(_key, _root.dump());
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
								if (std::regex_match(_elem.second, PURE_NUMBER))
								{
									Write(_elem.first, _elem.second);
								}
								else
								{
									//action_eval_->Eval((_elem.first + "=" + _elem.second).c_str());
									action_eval_->Eval(_elem.second, _elem.first);
								}
							}
						}
					}
					else //if (_elem.second != "")
					{
						if (_elem.second == "")
						{
							deepAssign(_elem.first, _elem.second, _left_bracket_pos);
						}
						else
						{
							if (_left_bracket_pos != _elem.first.size() - 1 && _elem.first[_left_bracket_pos + 1] == ']' &&  _elem.second.front() != '{' && _elem.second.back() != '}')
							{
								action_eval_->Eval(_elem.first.substr(0, _left_bracket_pos) + "+=" + _elem.second);
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
		for (auto _elem : params)
		{
			String _value = _elem.second;
			ReplaceModelNameWithValue(_value);
			Write("Logger.log." + _elem.first, _value);
		}

		String _label = "", _expr = "";
		Read("Logger.log.label", _label);
		Read("Logger.log.expr", _expr);
		LOG_I(_label.c_str(), "%s", _expr.c_str());

		break;
	}
	case INVOKE:
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
			_value = (!_name.empty() && (stoi(_name) == 1 || _name == "true"));
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
		if (_elem.second.size() <= 2 || _elem.second[0] != '{' || _elem.second.back() != '}')
		{
			LOG_E(TAG, "Parameters \"%s\" must start with '{' and end with '}'\n", _elem.second.c_str());
			continue;
		}

		json _root;
		try
		{
			_root = json::parse(_elem.second);
		}
		catch (...)
		{
			LOG_E(TAG, "Error when parsing parameters %s of %s", _elem.second.c_str(), name.c_str());
			assert(false);
			return;
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
				Array<String> _value = _obj.value().get<Array<String>>();
				Write(_key, _value);
			}
			else if (_obj.value().is_object())
			{
				Map<String, String> _value = _obj.value().get<Map<String, String>>();
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

		json _root;
		try
		{
			_root = json::parse(_elem.second);
		}
		catch (...)
		{
			LOG_E(TAG, "Error when parsing parameters %s of %s", _elem.second.c_str(), name.c_str());
			assert(false);
			return;
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

template<typename T>
void DNA::Write(const String& name, const T& value)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	out(value);
	model()->Write(name, GetType(value), data);
}

void DNA::Read(const String& name, bool& value)
{
	DynaArray _stored_type;
	DynaArray _buf;
	model()->Read(name, _stored_type, _buf);
	try {
		Deserialize(_stored_type.data(), _buf, value);
	}
	catch (...)
	{
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

template<typename T>
void DNA::Read(const String& name, T& value)
{
	DynaArray _stored_type;
	DynaArray _buf;
	model()->Read(name, _stored_type, _buf);
	try {
		Deserialize(_stored_type.data(), _buf, value);
	}
	catch (...)
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


void DNA::Clone(const String& target_name, const String& src_name, const String& default_value)
{
	DynaArray _stored_type;
	DynaArray _buf;
	if (src_name.back() == '*')
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
	Array<String> _list = Array<String>();
	Read(target_name, _list);
	_list.push_back(_value);
	Write(target_name, _list);
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
				switch (type_hash_table_[_type_hash])
				{
				case type_int32_t:
					Deserialize(int32_t(), data, value);
					break;
				case type_uint32_t:
					Deserialize(uint32_t(), data, value);
					break;
				case type_int64_t:
					Deserialize(int64_t(), data, value);
					break;
				case type_uint64_t:
					Deserialize(uint64_t(), data, value);
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
				case type_int64_t_array:
					Deserialize(Array<int64_t>(), data, value);
					break;
				case type_uint64_t_array:
					Deserialize(Array<uint64_t>(), data, value);
					break;
				case type_double_array:
					Deserialize(Array<double>(), data, value);
					break;
				case type_string_array:
					Deserialize(Array<String>(), data, value);
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
		switch (type_hash_table_[_type_hash])
		{
		case type_int32_t:
			Deserialize(int32_t(), data, value);
			break;
		case type_uint32_t:
			Deserialize(uint32_t(), data, value);
			break;
		case type_int64_t:
			Deserialize(int64_t(), data, value);
			break;
		case type_uint64_t:
			Deserialize(uint64_t(), data, value);
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
		case type_int64_t_array:
			Deserialize(Array<int64_t>(), data, value);
			break;
		case type_uint64_t_array:
			Deserialize(Array<uint64_t>(), data, value);
			break;
		case type_double_array:
			Deserialize(Array<double>(), data, value);
			break;
		case type_string_array:
			Deserialize(Array<String>(), data, value);
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
template<typename T>
void DNA::TypeConversion(const String& src, T& dest) {
	if (is_number(src))
		dest = (T)stod(src);
	else
		dest = (T)0.0;
};
void DNA::TypeConversion(const String& src, String& dest) { dest = src; };
template<typename T>
void DNA::TypeConversion(const T& src, Array<String>& dest) { dest.push_back(std::to_string(src)); };
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
		dest = split(src.substr(1, src.size() - 2), ",");
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
	type_hash_table_ =
	{
		{ hash_(typeid(bool).name()), type_bool },
		{ hash_(typeid(int32_t).name()), type_int32_t },
		{ hash_(typeid(uint32_t).name()), type_uint32_t},
		{ hash_(typeid(int64_t).name()), type_int64_t },
		{ hash_(typeid(uint64_t).name()), type_uint64_t },
		{ hash_(typeid(double).name()), type_double },
		{ hash_(typeid(String).name()), type_string },

		{ hash_(typeid(Array<bool>).name()), type_bool_array },
		{ hash_(typeid(Array<int32_t>).name()), type_int32_t_array },
		{ hash_(typeid(Array<uint32_t>).name()), type_uint32_t_array },
		{ hash_(typeid(Array<int64_t>).name()), type_int64_t_array },
		{ hash_(typeid(Array<uint64_t>).name()), type_uint64_t_array },
		{ hash_(typeid(Array<double>).name()), type_double_array },
		{ hash_(typeid(Array<String>).name()), type_string_array },
	};
}

void DNA::SaveVersion()
{
	Write(String("Bio.Cell.version.") + name().data(), (String)get_version());
	Write(String("Bio.Cell.version.major.") + name().data(), VERSION_MAJOR);
	Write(String("Bio.Cell.version.minor.") + name().data(), VERSION_MINOR);
	Write(String("Bio.Cell.version.build.") + name().data(), VERSION_BUILD);
	Write(String("Bio.Cell.version.revision.") + name().data(), (String)VERSION_REVISION);
	const time_t TIME_DIFFERENCE = -8 * 60 * 60;
	Write(String("Bio.Cell.version.time.") + name().data(), (time_t)__TIME_UNIX__ + TIME_DIFFERENCE);
}

const char* DNA::get_version()
{
	static String _version = std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_BUILD) + "." + VERSION_REVISION;
	return _version.c_str();
}

bool DNA::findAndReplaceAll(String& data, const String& toSearch, const String& replaceStr)
{
	bool _ret = false;
	// Get the first occurrence
	size_t pos = data.find(toSearch);
	String _replace_str = "";
	if (replaceStr.size() >= 2 && replaceStr.front() == '\"' && replaceStr.back() == '\"')
		_replace_str = replaceStr.substr(1, replaceStr.size() - 2);
	else
		_replace_str = replaceStr;
	// Repeat till end is reached
	while (pos != std::string::npos)
	{
		// Replace this occurrence of Sub String
		unsigned char _next_char = data[pos + toSearch.size()];
		if (!std::isalpha(_next_char) && !std::isdigit(_next_char) && _next_char != '_' && _next_char != '.' && _next_char != '['
			|| toSearch.substr(0,3) == "::(" && toSearch.back() == ')')
		{
			data.replace(pos, toSearch.size(), _replace_str);
			_ret = true;
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

void DNA::ReplaceModelNameWithValue(String& target, const String& resolved_model_name)
{
	if (target.find('\0') != String::npos)		// ignore binary data
		return;
	Stack<Pair<size_t, size_t>> _active_range;
	Set<Pair<size_t, size_t>> _ignored_range_list;
	Set<String> _ignored_token_list;
	Map<String, String> _resolved_model_cache;
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
				bool _is_ternary_op = std::regex_match(_token, TERNARY_OPERATOR);
				if (_token.find('\0') == String::npos && (_model_pos == String::npos || _model_pos > 1 && _token[_model_pos-1]=='`' || _is_ternary_op == true))
				{
					const std::regex EVALUABLE_EXPR("^[\\w.+\\-*\\/&~^?|',:<>!={}\\[\\]\\(\\) ]*$");
					//const std::regex STRING_COMPARISON("^[\"'][^?]+[\"'] *[=><]{1,2} *[\"'][^?]+[\"'](&&|\|\|)*\w*$");
					String _model_value;
					if (_token.empty()) {
						_model_value = "";
					}
					else if (_token.front() == '{' && _token.back() == '}'
						|| _token.front() == '[' && _token.back() == ']')
					{	// escaping JSON
						_model_value = EscapeJSONString(_token, true);
					}
					else if (_is_ternary_op)
					{
						size_t _question_pos = _token.find('?');
						size_t _splitter = _token.find(':', _question_pos + 1);
						size_t _next_quot = _token.find_first_not_of(' ', _splitter + 1);
						while (_next_quot != String::npos && _splitter != String::npos
							&& _token[_next_quot] != '\'' && _token[_next_quot] != '"')
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
						const std::regex TERNARY_OPERATOR_OF_STRING("^[^?]*? *\\? *[\"'][^?]*[\"'] *: *[\"'][^?]*[\"'] *$");
						if (std::regex_match(_token, TERNARY_OPERATOR_OF_STRING))
						{
							if (_result)
							{
								size_t _start_pos = _token.find_first_of("\"'", _question_pos + 1);
								size_t _end_pos = _token.find_last_of("\"'", _splitter - 1);
								_model_value = _token.substr(_start_pos + 1, _end_pos - _start_pos - 1);
							}
							else
							{
								size_t _start_pos = _token.find_first_of("\"'", _splitter + 1);
								size_t _end_pos = _token.find_last_of("\"'");
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
					else if (std::regex_match(_token, EVALUABLE_EXPR))
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
							if (!_root.is_null())
							{	// JSON format
								if (_root.is_array() || _root.is_object())
								{
									_model_value = std::to_string(_root.size());
								}
								else
								{
									_model_value = "0";
								}
								_found_in_model = true;
							}
							else
							{
								_model_value = "0";
								_found_in_model = true;
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
							&& (_value_list.front().size() == 0 || _value_list.front().front() != '{'
								|| _value_list.back().size() == 0 || _value_list.back().back() != '}'))
						{
							//int _array_index = std::stoi(_index_str);
							int _array_index = 0;
							Read("Bio.Chromosome.DNA.Temp.Index", _array_index);
							if (_value_list.size() > _array_index)
							{
								_found_in_model = true;

								if (target.size() > _index_end_pos + 1 && target[_index_end_pos + 1] == '.')
								{	// try protobuf
									size_t _break_pos = _model_name.find_last_of('.');
									Write("Bio.Chromosome.DecomposeMessage.message_name", _model_name.substr(0, _break_pos));
									Write("Bio.Chromosome.DecomposeMessage.field_name", _model_name.substr(_break_pos + 1));
									Write("Bio.Chromosome.DecomposeMessage.payload", _value_list[_array_index]);
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
									_model_value = _value_list[_array_index];
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
								if (_resolved_model_cache.count(_model_name) > 0)
									_value = _resolved_model_cache[_model_name];
								else
								{
									Write("Bio.Cell.Model.DefaultReturnModelname", true);
									ReplaceModelNameWithValue(_value, _model_name);
									Write("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
									_resolved_model_cache[_model_name] = _value;
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
							ReplaceModelNameWithValue(_path_str);
							ReplaceModelNameWithValue(_last_str);
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
										else
										{
											_model_value = "0";
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
									}
								}
								if (!_root.is_null())
								{	// JSON format
									String _orig_path_str = _path_str;
									while (true)
									{
										json::json_pointer _target_ptr(_path_str);
										try
										{
											if (_root[_target_ptr].is_null())
												_model_value = "";
											else if (_path_str == _orig_path_str)
												_model_value = _root[_target_ptr].dump();
											else
											{
												json _sub_tree = json::parse(_root[_target_ptr].get<String>());
												String _new_path = _orig_path_str.substr(_path_str.size());
												json::json_pointer _subtree_target_ptr(_new_path);
												_model_value = _sub_tree[_subtree_target_ptr].dump();
											}
											_found_in_model = true;
											break;
										}
										catch (json::exception)
										{
											size_t _pos = _path_str.find_last_of('/');
											if (_pos != String::npos && _pos != 0)
												_path_str = _path_str.substr(0, _pos);
											else
												break;
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
							if (_resolved_model_cache.count(_model_name) > 0)
								_model_value = _resolved_model_cache[_model_name];
							else
							{
								ReplaceModelNameWithValue(_model_value, _model_name);		// for nested model names
								_resolved_model_cache[_model_name] = _model_value;
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
				//_start_pos += _model_value.size();		// if replaced, search from the start for nested model names
			}
			else
			{
				_start_pos = _end_pos;
			}
		}
		if (_start_pos >= _range_end)
			_active_range.pop();
	}
}

void DNA::deepAssign(const String& map_key, const String& map_value, size_t left_bracket_pos)
{
	size_t _right_bracket_pos = map_key.find(']', left_bracket_pos);
	if (_right_bracket_pos != String::npos)
	{
		String _serialized_str = "";
		String _key = map_key.substr(0, left_bracket_pos);
		Read(_key, _serialized_str);
		{	// deep assign to JSON path before converting without touching anything not known
			bool _default_model_name = false;
			Read("Bio.Cell.Model.DefaultReturnModelname", _default_model_name);
			Write("Bio.Cell.Model.DefaultReturnModelname", true);
			ReplaceModelNameWithValue(_serialized_str);
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

		json _root;
		String _escaped_value = "";
		if (!_serialized_str.empty())
		{
			try
			{
				_root = json::parse(_serialized_str);
			}
			catch (const std::exception& e)
			{
				LOG_D(TAG, "Error when parsing %s as JSON, exception: %s", _serialized_str.c_str(), e.what());
				EscapeJSON(_serialized_str, _escaped_value);
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
				else if (map_value.front() == '{' && map_value.back() == '}'
					|| map_value.front() == '[' && map_value.back() == ']')
				{
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
					RemoveElement(_root, _path_str);
					if (map_value.find("::") == String::npos)
					{	// "JSON" or 'JSON'
						if ((map_value.front() == '\"' || map_value.front() == '\'') && map_value.front() == map_value.back())
							_root[_new_value_ptr] = map_value.substr(1, map_value.size() - 2);
						else
							_root[_new_value_ptr] = map_value;
					}
				}
			}
			else
			{
				if (map_value.front() == '{' && map_value.back() == '}'
					|| map_value.front() == '[' && map_value.back() == ']')
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
							if (_root[_new_value_ptr].is_null())
								_root[_new_value_ptr] = json::array();
							if (_new_value.is_array())
							{
								for (const auto& elem : _new_value)
									_root[_new_value_ptr].push_back(elem);
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
					if (map_value.front() == '\"' && map_value.back() == '\"')
						_root[_new_value_ptr].push_back(map_value.substr(1, map_value.size() - 2));
					else
						_root[_new_value_ptr].push_back(map_value);
				}
			}
			Write(_key, _root.dump());
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
				path_str.replace(path_str.find(_index_str), _index_str.size(), _new_index_str);
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
			if (_index >= (*obj)->size())
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

void DNA::RemoveElement(json& root, const String& path_str, bool forever)
{
	json* _obj = nullptr;
	String _key;
	bool _found = FindAndUnserialize(root, path_str, &_obj, _key);
	if (_obj->is_array())
	{
		if (forever)
			_obj->erase(stoi(_key));
		else
			(*_obj)[stoi(_key)] = json();
	}
	else if (_obj->is_object())
		_obj->erase(_key);
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
