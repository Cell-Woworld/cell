#include "internal/mRNA.h"
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>
#include "internal/utils/cell_utils.h"
#include <regex>

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

#define TAG "mRNA"

using namespace google::protobuf;

USING_BIO_NAMESPACE
#ifdef __cplusplus
#ifdef STATIC_API
extern "C" PUBLIC_API RNA* mRNA_CreateInstance(IBiomolecule* owner)
{
	return new mRNA(owner);
}
#else
extern "C" PUBLIC_API RNA* CreateInstance(IBiomolecule* owner)
{
	return new mRNA(owner);
}
#endif
#else
#endif

mRNA::mRNA(IBiomolecule* owner)
	:RNA(owner, "mRNA", this)
{
	SaveVersion();
}

mRNA::~mRNA()
{
}

void mRNA::SaveVersion()
{
	WriteValue("Bio.Cell.version.mRNA", GetVersion());
	WriteValue("Bio.Cell.version.major.mRNA", VERSION_MAJOR);
	WriteValue("Bio.Cell.version.minor.mRNA", VERSION_MINOR);
	WriteValue("Bio.Cell.version.build.mRNA", VERSION_BUILD);
	WriteValue("Bio.Cell.version.revision.mRNA", (String)VERSION_REVISION);
	LOG_D(TAG, "version: %s", ReadValue<String>("Bio.Cell.version.mRNA").c_str());
	const time_t TIME_DIFFERENCE = -8 * 60 * 60;
	WriteValue("Bio.Cell.version.time.mRNA", (time_t)__TIME_UNIX__ + TIME_DIFFERENCE);
}

void mRNA::bind(IBiomolecule* src, void* desc_pool)
{
	//LOG_D(TAG, "mRNA<0x%X>::bind(%s)", (unsigned int)(unsigned long long)this, src->full_name().data());
	const DescriptorPool& _pool = *(DescriptorPool*)desc_pool;
	databases_.push_back(new DescriptorPoolDatabase(_pool));
	databases_src_.push_back(src);
	merged_database_.reset(new MergedDescriptorDatabase(databases_));
	desc_pool_.reset(new DescriptorPool(merged_database_.get()));
}

void mRNA::unbind(IBiomolecule* src)
{
	//LOG_D(TAG, "mRNA<0x%X>::unbind(%s)", (unsigned int)(unsigned long long)this, src->full_name().data());
	for (int i = 0; i < databases_src_.size();)
	{
		if (databases_src_[i] == src)
		{
			std::swap(databases_src_[i], databases_src_.back());
			databases_src_.pop_back();
			std::swap(databases_[i], databases_.back());
			delete databases_.back();
			databases_.pop_back();
		}
		else
		{
			i++;
		}
	}
	merged_database_.reset(new MergedDescriptorDatabase(databases_));
	desc_pool_.reset(new DescriptorPool(merged_database_.get()));
}

bool mRNA::fill_model_by_field_name(const String& name, const String& content)
{
	const Descriptor* _desc = desc_pool_->FindMessageTypeByName(name);
	if (_desc == nullptr)
		return false;
	//String _content = content;
	//std::replace(_content.begin(), _content.end(), ';', ',');
	//Array<String> _field_values = split(_content, ",");
	Array<String> _field_values = split(content, ",");
	for (int i = 0; i < _desc->field_count(); i++)
	{
		const FieldDescriptor* _field_desc = _desc->field(i);
		const String _model_name = _desc->field(i)->full_name();
		if (i < _field_values.size())
			WriteValue(_model_name, _field_values[i]);
		else
			WriteValue(_model_name, "");
	}
	return true;
}

bool mRNA::pack(const DynaArray& root_path, const DynaArray& name, DynaArray& payload)
{
	// translate via patterns
	if (desc_pool_ == nullptr)
		return false;
	const Descriptor* _desc = nullptr;
	try
	{
		_desc = desc_pool_->FindMessageTypeByName(name.str());
		if (_desc == nullptr)
		{
			return false;
		}
	}
	catch (...)
	{
		if (payload.size() > 0)
		{
			LOG_W(TAG, "mRNA::pack() doesn't find %s", name.data());
		}
		return false;
	}
	DynamicMessageFactory _message_factory;
	const Message* _message_proto = _message_factory.GetPrototype(_desc);
	Obj<Message> _message = Obj<Message>(_message_proto->New());

	Set<String> _bin_field_set;
	if (packFromPacket(root_path.str(), _message, _bin_field_set) == true)
	{
		payload = _message->SerializeAsString();
		return true;
	}

	Remove(name.str() + ".*", true);

	Array<String> _columns_src;
	bool _pack_from_columns_src = packFromColumnsSource(name.str(), _message, _columns_src);

	const Reflection* _reflection = _message->GetReflection();
	for (int i = 0; i < _desc->field_count(); i++)
	{
		const FieldDescriptor* _field_desc = _desc->field(i);
		String _model_name = "";
		if (_pack_from_columns_src == true)
		{
			if (_columns_src[i] == "")
				continue;
			else if (_columns_src[i][0] == ':' && _columns_src[i][1] == ':')
				_model_name = _columns_src[i].substr(2);
			else
			{
				_model_name = _desc->field(i)->full_name();
				WriteValue(_model_name, _columns_src[i]);
			}
		}
		else
		{
			_model_name = _desc->field(i)->full_name();
		}
		if (_bin_field_set.size() > 0 && _bin_field_set.find(_model_name) == _bin_field_set.end())
			continue;

		switch (_field_desc->type())
		{
		case FieldDescriptor::TYPE_BYTES:
		case FieldDescriptor::TYPE_STRING:
			if (_field_desc->is_repeated())
			{
				Array<String> _content = ReadValue<Array<String>>(_model_name);
				for (auto _elem : _content)
				{
					_reflection->AddString(_message.get(), _field_desc, _elem);
				}
			}
			else
			{
				String _content = ReadValue<String>(_model_name);
				_reflection->SetString(_message.get(), _field_desc, _content);
			}
			break;
		case FieldDescriptor::TYPE_BOOL:
			if (_field_desc->is_repeated())
			{
				Array<bool> _content = ReadValue<Array<bool>>(_model_name);
				for (auto _elem : _content)
				{
					_reflection->AddBool(_message.get(), _field_desc, _elem);
				}
			}
			else
			{
				bool _content = ReadValue<bool>(_model_name);
				_reflection->SetBool(_message.get(), _field_desc, _content);
			}
			break;
		case FieldDescriptor::TYPE_INT32:
			if (_field_desc->is_repeated())
			{
				Array<int32_t> _content = ReadValue<Array<int32_t>>(_model_name);
				for (auto _elem : _content)
				{
					_reflection->AddInt32(_message.get(), _field_desc, _elem);
				}
			}
			else
			{
				int32_t _content = ReadValue<int32_t>(_model_name);
				_reflection->SetInt32(_message.get(), _field_desc, _content);

			}
			break;
		case FieldDescriptor::TYPE_UINT32:
			if (_field_desc->is_repeated())
			{
				Array<uint32_t> _content = ReadValue<Array<uint32_t>>(_model_name);
				for (auto _elem : _content)
				{
					_reflection->AddUInt32(_message.get(), _field_desc, _elem);
				}
			}
			else
			{
				uint32_t _content = ReadValue<uint32_t>(_model_name);
				_reflection->SetUInt32(_message.get(), _field_desc, _content);
			}
			break;
		case FieldDescriptor::TYPE_INT64:
			if (_field_desc->is_repeated())
			{
				Array<int64_t> _content = ReadValue<Array<int64_t>>(_model_name);
				for (auto _elem : _content)
				{
					_reflection->AddInt64(_message.get(), _field_desc, _elem);
				}
			}
			else
			{
				int64_t _content = ReadValue<int64_t>(_model_name);
				_reflection->SetInt64(_message.get(), _field_desc, _content);
			}
			break;
		case FieldDescriptor::TYPE_UINT64:
			if (_field_desc->is_repeated())
			{
				Array<uint64_t> _content = ReadValue<Array<uint64_t>>(_model_name);
				for (auto _elem : _content)
				{
					_reflection->AddUInt64(_message.get(), _field_desc, _elem);
				}
			}
			else
			{
				uint64_t _content = ReadValue<uint64_t>(_model_name);
				_reflection->SetUInt64(_message.get(), _field_desc, _content);
			}
			break;
		case FieldDescriptor::TYPE_DOUBLE:
			if (_field_desc->is_repeated())
			{
				Array<double> _content = ReadValue<Array<double>>(_model_name);
				for (auto _elem : _content)
				{
					_reflection->AddDouble(_message.get(), _field_desc, _elem);
				}
			}
			else
			{
				double _content = ReadValue<double>(_model_name);
				_reflection->SetDouble(_message.get(), _field_desc, _content);
			}
			break;
		case FieldDescriptor::TYPE_ENUM:
			if (_field_desc->is_repeated())
			{
				Array<String> _content = ReadValue<Array<String>>(_model_name);
				for (auto _elem : _content)
				{
					if (is_number(_elem))
					{
						_reflection->AddEnumValue(_message.get(), _field_desc, (int)stoi(_elem));
					}
					else
					{
						const google::protobuf::EnumValueDescriptor* _enum_value_descriptor = _field_desc->enum_type()->FindValueByName(_elem);
						if (_enum_value_descriptor != nullptr)
							_reflection->AddEnum(_message.get(), _field_desc, _enum_value_descriptor);
						else
							_reflection->AddEnumValue(_message.get(), _field_desc, 0);
					}
				}
			}
			else
			{
				String _content = ReadValue<String>(_model_name);
				if (is_number(_content))
				{
					_reflection->SetEnumValue(_message.get(), _field_desc, stoi(_content));
				}
				else
				{
					const google::protobuf::EnumValueDescriptor* _enum_value_descriptor = _field_desc->enum_type()->FindValueByName(_content);
					if (_enum_value_descriptor != nullptr)
						_reflection->SetEnum(_message.get(), _field_desc, _enum_value_descriptor);
					else
						_reflection->SetEnumValue(_message.get(), _field_desc, 0);
				}
			}
			break;
		case FieldDescriptor::TYPE_MESSAGE:
		{
			if (_field_desc->is_repeated())
			{
				//try to parse as JSON first
				String _value = ReadValue<String>(_model_name);
				if (_value.size() >= 4 && _value.front() == '[' && _value.back() == ']' && _value[1] == '{' && _value[_value.size() - 2] == '}')
				{
					// It must be JSON format
					//std::replace(_value.begin(), _value.end(), ';', ',');
					Array<String> _content = SplitByString(_value.substr(1,_value.size()-2), "},");
					for (int i = 0; i < _content.size(); i++)
					{
						String _elem = (i == _content.size() - 1) ? _content[i] : _content[i] + "}";
						Message* _sub_message = _reflection->AddMessage(_message.get(), _field_desc, &_message_factory);
						if (util::JsonStringToMessage(_elem, _sub_message) != util::Status::OK)
						{
							LOG_E(TAG, "mRNA::pack(%s) failed. Invalid JSON content of sub_message %s", _message->GetDescriptor()->full_name().c_str(), _sub_message->GetDescriptor()->full_name().c_str());
							_reflection->RemoveLast(_message.get(), _field_desc);
						}
					}
				}
				else
				{
					Array<String> _content = ReadValue<Array<String>>(_model_name);
					String sub_msg_name = _field_desc->message_type()->full_name();
					for (const auto& _elem : _content)
					{
						Message* _sub_message = _reflection->AddMessage(_message.get(), _field_desc, &_message_factory);
						if (_sub_message->ParseFromString(_elem) == false)
						{
							if (_elem.front() == '[' && _elem.back() == ']')
							{
								DynaArray _payload = "";
								if (fill_model_by_field_name(sub_msg_name, _elem.substr(1, _elem.size() - 2)) == false
									|| pack(root_path, sub_msg_name, _payload) == false
									|| _sub_message->ParseFromArray(_payload.data(), (int)_payload.size()) == false)
								{
									_reflection->RemoveLast(_message.get(), _field_desc);
								}
							}
							else
							{
								_reflection->RemoveLast(_message.get(), _field_desc);
							}
						}
					}
				}
			}
			else
			{
				String _content = ReadValue<String>(_model_name);
				Message* _sub_message = _reflection->MutableMessage(_message.get(), _field_desc, &_message_factory);
				// Serialized
				if (_content != "" && _sub_message->ParseFromString(_content) == true)
					break;
				// JSON format
				if (util::JsonStringToMessage(_content, _sub_message) == util::Status::OK)
					break;

				String sub_msg_name = _field_desc->message_type()->full_name();
				if (_content != "" && fill_model_by_field_name(sub_msg_name, _content) == true)
				{
					DynaArray _payload = "";
					if (pack(root_path, sub_msg_name, _payload) == true)
					{
						_sub_message->ParseFromArray(_payload.data(), (int)_payload.size());
					}
				}
			}
			break;
		}
		default:
			break;
		}
	}
	payload = _message->SerializeAsString();
	return true;
}

#define unpack_field(type, func1, func2) { \
	if (_field_desc->is_repeated()) { \
		int _count = _reflection->FieldSize(*_message, _field_desc); \
		Array<type> _content;	\
		for (int i = 0; i < _count; i++) \
		{ \
			_content.push_back(func1(*_message, _field_desc, i)); \
		}; \
		WriteValue(_model_name, _content, true); \
	} else { \
		WriteValue(_model_name, func2(*_message, _field_desc), true); \
	} \
}\

bool mRNA::unpack(const DynaArray& name, const DynaArray& payload, const DynaArray& field_name)
{
	if (payload.size() == 1 && payload.data()[0] == '\0')
		return true;

	// translate via patterns
	if (desc_pool_ == nullptr)
		return true;
	const Descriptor* _desc = desc_pool_->FindMessageTypeByName(name.str());
	if (_desc == nullptr)
	{
		if (payload.size() > 0)
		{
			LOG_W(TAG, "mRNA::unpack() doesn't find %s. Fail to unpack()!", name.data());
		}
		return true;		// it must be "true"
	}

	Remove(name.str() + ".*", true);		// to remove fields of external messages

	//if (payload == "")					// Don't do this! We should unpack payload with size=0 to models. ex. if return, boolean field with value "false" will be "" or "false"
	//	return true;

	DynamicMessageFactory _message_factory;
	const Message* _message_proto = _message_factory.GetPrototype(_desc);
	Obj<Message> _message = Obj<Message>(_message_proto->New());
	if (_message->ParseFromArray(payload.data(), (int)payload.size()) == false)
		return false;

	const Reflection* _reflection = _message->GetReflection();
	String _path = "";
	if (field_name != "")
	{
		_path = field_name.str();
	}
	else
	{
		_path = name.str();
	}
	for (int i = 0; i < _desc->field_count(); i++)
	{
		const FieldDescriptor* _field_desc = _desc->field(i);
		String _model_name = _path + "." + _desc->field(i)->name();

		switch (_field_desc->type())
		{
		case FieldDescriptor::TYPE_BYTES:
			unpack_field(String, _reflection->GetRepeatedString, _reflection->GetString);
			break;
		case FieldDescriptor::TYPE_STRING:
			if (_field_desc->is_repeated())
			{
				int _count = _reflection->FieldSize(*_message, _field_desc); 
				Array<String> _content;
				for (int i = 0; i < _count; i++) 
				{
					String _scratch;
					const String& _string_content = _reflection->GetRepeatedStringReference(*_message, _field_desc, i, &_scratch);
					size_t first_char_pos = _string_content.find_first_not_of(" \r\n\t");
					size_t last_char_pos = _string_content.find_last_not_of(" \r\n\t");
					if (_string_content.size() < 2
						|| _string_content[first_char_pos] == '[' && _string_content[last_char_pos] == ']'
						|| _string_content[first_char_pos] == '{' && _string_content[last_char_pos] == '}')
						_content.push_back(_string_content);
					else
					{
						if (_string_content.front() == '"' && _string_content.back() == '"')
							_content.push_back(String("\"") + std::regex_replace(_string_content.substr(1, _string_content.size() - 2), std::regex("(?!.*\\\")^.*\""), u8"би") + "\"");
						else
							_content.push_back(std::regex_replace(_string_content, std::regex("(?!.*\\\")^.*\""), u8"би"));
					}
				}; 
				WriteValue(_model_name, _content, true); 
			}
			else
			{
				String _scratch;
				const String& _string_content = _reflection->GetStringReference(*_message, _field_desc, &_scratch);
				size_t first_char_pos = _string_content.find_first_not_of(" \r\n\t");
				size_t last_char_pos = _string_content.find_last_not_of(" \r\n\t");
				if (_string_content.size() < 2
					|| _string_content[first_char_pos] == '[' && _string_content[last_char_pos] == ']'
					|| _string_content[first_char_pos] == '{' && _string_content[last_char_pos] == '}')
					WriteValue(_model_name, _string_content, true);
				else {
					if (_string_content.front() == '"' && _string_content.back() == '"')
						WriteValue(_model_name, String("\"") + std::regex_replace(_string_content.substr(1, _string_content.size() - 2), std::regex("(?!.*\\\")^.*\""), u8"би") + "\"", true);
					else
						WriteValue(_model_name, std::regex_replace(_string_content, std::regex("(?!.*\\\")^.*\""), u8"би"), true);
				}
			}
			break;
		case FieldDescriptor::TYPE_BOOL:
			unpack_field(bool, _reflection->GetRepeatedBool, _reflection->GetBool);
			break;
		case FieldDescriptor::TYPE_INT32:
			unpack_field(int32_t, _reflection->GetRepeatedInt32, _reflection->GetInt32);
			break;
		case FieldDescriptor::TYPE_UINT32:
			unpack_field(uint32_t, _reflection->GetRepeatedUInt32, _reflection->GetUInt32);
			break;
		case FieldDescriptor::TYPE_INT64:
			unpack_field(int64_t, _reflection->GetRepeatedInt64, _reflection->GetInt64);
			break;
		case FieldDescriptor::TYPE_UINT64:
			unpack_field(uint64_t, _reflection->GetRepeatedUInt64, _reflection->GetUInt64);
			break;
		case FieldDescriptor::TYPE_DOUBLE:
			unpack_field(double, _reflection->GetRepeatedDouble, _reflection->GetDouble);
			break;
		case FieldDescriptor::TYPE_ENUM:
		{
			//unpack_field(int, _reflection->GetRepeatedEnumValue, _reflection->GetEnumValue);
			if (_field_desc->is_repeated()) {
				int _count = _reflection->FieldSize(*_message, _field_desc);
				Array<String> _content;
				Array<int> _value;
				for (int i = 0; i < _count; i++)
				{
					_content.push_back(_reflection->GetRepeatedEnum(*_message, _field_desc, i)->name());
					_value.push_back(_reflection->GetRepeatedEnum(*_message, _field_desc, i)->number());
				}
				WriteValue(_model_name, _content, true);
				WriteValue(_model_name+".value", _value, true);
			}
			else {
				WriteValue(_model_name, _reflection->GetEnum(*_message, _field_desc)->name(), true);
				WriteValue(_model_name+".value", _reflection->GetEnum(*_message, _field_desc)->number(), true);
			}

		}
			break;
		case FieldDescriptor::TYPE_MESSAGE:
			if (_field_desc->is_repeated())
			{
				int _count = _reflection->FieldSize(*_message, _field_desc);
				if (_field_desc->is_map())
				{
					::Map<String, String> _content;
					String sub_msg_name = _field_desc->message_type()->full_name();
					for (int i = 0; i < _count; i++)
					{
						unpack(sub_msg_name, _reflection->GetRepeatedMessage(*_message, _field_desc, i).SerializeAsString());
						_content.insert(std::make_pair(ReadValue<String>(sub_msg_name+".key"), ReadValue<String>(sub_msg_name + ".value")));
					}
					WriteValue(_model_name, _content, true);
				}
				else
				{
					Array<String> _content;
					for (int i = 0; i < _count; i++)
						_content.push_back(_reflection->GetRepeatedMessage(*_message, _field_desc, i).SerializeAsString());
					WriteValue(_model_name, _content, true);
				}
			}
			else
			{
				WriteValue(_model_name, _reflection->GetMessage(*_message, _field_desc).SerializeAsString());
			}
			break;
		default:
			break;
		}
	}
	return true;
}

bool mRNA::packFromPacket(const String& root_path, Obj<Message>& message, Set<String>& bin_field_set)
{
	const Descriptor* _desc = message->GetDescriptor();
	String _model_name = _desc->full_name() + "." + TAG_PACKET;
	String _content = ReadValue<String>(_model_name);
	if (_content == "")
		return false;
	if  (message->ParseFromString(_content) == true)
	{
		Remove(_model_name);
		return true;
	}
	// JSON format
	RetrieveBinaryData(_content, _desc->full_name(), bin_field_set);
	auto _status = util::JsonStringToMessage(_content, message.get());
	if (_status == util::Status::OK)
	{
		Remove(_model_name);
		return (bin_field_set.size() == 0);
	}
	else
	{
		LOG_W(TAG, "unresolved packet \"%s\" with payload: %s, error code=%d, message=%s", _desc->full_name().c_str(), _content.c_str(), _status.code(), _status.message().data());
		return false;
	}
}

bool mRNA::packFromColumnsSource(const String& name, Obj<Message>& message, Array<String>& columns_source)
{
	const Descriptor* _desc = message->GetDescriptor();
	String _model_name = _desc->full_name() + "." + TAG_COLUMNS_SOURCE_MODEL_LIST;
	columns_source = ReadValue<Array<String>>(_model_name);
	if (columns_source.size() > 0 && columns_source.size() != _desc->field_count())
		LOG_W(TAG, "Unmatched size between Columns(size=%d) and Fields(size=%d)", (int)columns_source.size(), _desc->field_count());
	return (columns_source.size() >= _desc->field_count());
}

void mRNA::RetrieveBinaryData(String& target, const String& name, Set<String>& bin_field_set)
{
	bool _is_changed = false;
	using json = nlohmann::json;
	json _root;
	String _escaped_value = "";
	try
	{
		_root = json::parse(target);
	}
	catch (const std::exception& e)
	{
		LOG_D(TAG, "Error when parsing %s as JSON, message=%s", target.c_str(), e.what());
		EscapeJSON(target, _escaped_value);
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
	Array<String> _bin_field_list;
	for (auto& obj : _root.items())
	{
		String _key = String(name) + "." + obj.key();
		if (obj.value().is_string())
		{
			String _value = obj.value();
			const std::regex EVALUABLE_EXPR("^(::\\(::)[\\w.]+\\)$");
			if (std::regex_match(_value, EVALUABLE_EXPR))
			{
				Clone(_key, _value.substr(sizeof("::(::") - 1, _value.size()-sizeof("::(::)") + 1));
				bin_field_set.insert(_key);
				_bin_field_list.push_back(obj.key());
			}
		}
	}
	for (auto key : _bin_field_list)
		_root.erase(key);
	if (bin_field_set.size() > 0)
		target = _root.dump();
}

void mRNA::assign(const DynaArray& name)
{
	String _message_name = ReadValue<String>(name.str() + ".model_name");
	String _field_name = ReadValue<String>(name.str() + ".field_name");
	String _model_name = _message_name + "." + _field_name;
	Array<String> _path = ReadValue<Array<String>>(name.str() + ".path");
	String _value = ReadValue<String>(name.str() + ".value");
	// translate via patterns
	if (desc_pool_ == nullptr)
		return;
	const Descriptor* _desc = desc_pool_->FindMessageTypeByName(_message_name);
	if (_desc == nullptr)
	{
		LOG_E(TAG, "mRNA::assign(%s) failed. Doesn't find message %s.", _model_name.c_str(), _message_name.c_str());
		return;
	}
	if (_path.size() == 0)
	{
		LOG_E(TAG, "mRNA::assign(%s) failed. Invalid path.", _model_name.c_str());
		return;
	}
	const FieldDescriptor* _field_desc = _desc->FindFieldByName(_field_name);
	if (_field_desc->type() != FieldDescriptor::TYPE_MESSAGE)
	{
		LOG_E(TAG, "mRNA::assign(%s) failed. Invalid field. It must be in message format.", _model_name.c_str());
		return;
	}

	String _submsg_name = _field_desc->message_type()->full_name();
	const Descriptor* _submsg_desc = desc_pool_->FindMessageTypeByName(_submsg_name);
	DynamicMessageFactory _message_factory;
	const Message* _submsg_proto = _message_factory.GetPrototype(_submsg_desc);
	Obj<Message> _submsg = Obj<Message>(_submsg_proto->New());

	const String& _path_way = _path[0];
	if (is_number(_path_way))
	{
		if (!_field_desc->is_repeated())
		{
			LOG_E(TAG, "mRNA::assign(%s) failed. Unmatched path without repeated modifier(%s in %s)", _model_name.c_str(), _path_way.c_str(), ReadValue<String>(name.str() + ".path").c_str());
			return;
		}
		int _index = stoi(_path_way);
		Array<String> _content = ReadValue<Array<String>>(_model_name);
		if (_index >= _content.size())
		{
			LOG_E(TAG, "mRNA::assign(%s) failed. Index(%d) overflow. Array size is %d in path %s", _model_name.c_str(), _index, (int)_content.size(), ReadValue<String>(name.str() + ".path").c_str());
			return;
		}
		if (_submsg->ParseFromString(_content[_index]) == false)
		{
			LOG_E(TAG, "mRNA::assign(%s) failed. Invalid content of message %s in path %s", _model_name.c_str(), _submsg_name.c_str(), ReadValue<String>(name.str() + ".path").c_str());
			return;
		}
		assignValue(_submsg.get(), _message_factory, _field_desc, _path, _value);
		_content[_index] = _submsg->SerializeAsString();
		WriteValue(_model_name, _content);
	}
	else
	{
		if (_field_desc->is_repeated())
		{
			LOG_E(TAG, "mRNA::assign(%s) failed. Unmatched path with repeated modifier(%s in %s)", _model_name.c_str(), _path_way.c_str(), ReadValue<String>(name.str() + ".path").c_str());
			return;
		}
		String _content = ReadValue<String>(_model_name);
		if (_submsg->ParseFromString(_content) == false)
		{
			LOG_E(TAG, "mRNA::assign(%s) failed. Invalid content of message %s in path %s", _model_name.c_str(), _submsg_name.c_str(), ReadValue<String>(name.str() + ".path").c_str());
			return;
		}
		assignValue(_submsg.get(), _message_factory, _field_desc, _path, _value);
		WriteValue(_model_name, _submsg->SerializeAsString());
	}
}

void mRNA::assignValue(Message* message, DynamicMessageFactory& message_factory, const FieldDescriptor* field_desc, const Array<String>& path, const String& value)
{
	const Reflection* _reflection = message->GetReflection();
	Message* _current_message = message;
	const FieldDescriptor* _current_field_desc = field_desc;
	for (int i = 1; i < path.size(); i++)
	{
		if (is_number(path[i]))
		{
			int _index = stoi(path[i]);
			int _count = _reflection->FieldSize(*_current_message, _current_field_desc);
			if (_index >= _count)
			{
				LOG_E(TAG, "mRNA::assign(%s) failed. Index(%d) overflow. Array size is %d.", message->GetDescriptor()->full_name().c_str(), _index, _count);
				return;
			}
			if (i < path.size() - 1)	// it must be a message
			{
				_current_message = _reflection->MutableRepeatedMessage(_current_message, _current_field_desc, _index);
			}
		}
		else
		{
			_current_field_desc = _current_message->GetDescriptor()->FindFieldByName(path[i]);
			if (i < path.size() - 1 && _current_field_desc->type() == FieldDescriptor::TYPE_MESSAGE)
			{
				if (!_current_field_desc->is_repeated())
					_current_message = _reflection->MutableMessage(_current_message, _current_field_desc);
			}
		}
	}
	switch (_current_field_desc->type())
	{
	case FieldDescriptor::TYPE_BYTES:
	case FieldDescriptor::TYPE_STRING:
		if (_current_field_desc->is_repeated())
		{
			_reflection->AddString(_current_message, _current_field_desc, value);
		}
		else
		{
			_reflection->SetString(_current_message, _current_field_desc, value);
		}
		break;
	case FieldDescriptor::TYPE_BOOL:
		if (_current_field_desc->is_repeated())
		{
			_reflection->AddBool(_current_message, _current_field_desc, value == "true" ? true : false);
		}
		else
		{
			_reflection->SetBool(_current_message, _current_field_desc, value == "true" ? true : false);
		}
		break;
	case FieldDescriptor::TYPE_INT32:
		if (_current_field_desc->is_repeated())
		{
			_reflection->AddInt32(_current_message, _current_field_desc, std::stoi(value));
		}
		else
		{
			_reflection->SetInt32(_current_message, _current_field_desc, std::stoi(value));
		}
		break;
	case FieldDescriptor::TYPE_UINT32:
		if (_current_field_desc->is_repeated())
		{
			_reflection->AddUInt32(_current_message, _current_field_desc, std::stoi(value));
		}
		else
		{
			_reflection->SetUInt32(_current_message, _current_field_desc, std::stoi(value));
		}
		break;
	case FieldDescriptor::TYPE_INT64:
		if (_current_field_desc->is_repeated())
		{
			_reflection->AddInt64(_current_message, _current_field_desc, std::stoll(value));
		}
		else
		{
			_reflection->SetInt64(_current_message, _current_field_desc, std::stoll(value));
		}
		break;
	case FieldDescriptor::TYPE_UINT64:
		if (_current_field_desc->is_repeated())
		{
			_reflection->AddUInt64(_current_message, _current_field_desc, std::stoll(value));
		}
		else
		{
			_reflection->SetUInt64(_current_message, _current_field_desc, std::stoll(value));
		}
		break;
	case FieldDescriptor::TYPE_DOUBLE:
		if (_current_field_desc->is_repeated())
		{
			_reflection->AddDouble(_current_message, _current_field_desc, std::stod(value));
		}
		else
		{
			_reflection->SetDouble(_current_message, _current_field_desc, std::stod(value));
		}
		break;
	case FieldDescriptor::TYPE_ENUM:
		if (_current_field_desc->is_repeated())
		{
			if (is_number(value))
			{
				_reflection->AddEnumValue(_current_message, _current_field_desc, (int)stoi(value));
			}
			else
			{
				const google::protobuf::EnumValueDescriptor* _enum_value_descriptor = _current_field_desc->enum_type()->FindValueByName(value);
				if (_enum_value_descriptor != nullptr)
					_reflection->AddEnum(_current_message, _current_field_desc, _enum_value_descriptor);
				else
					_reflection->AddEnumValue(_current_message, _current_field_desc, 0);
			}
		}
		else
		{
			if (is_number(value))
			{
				_reflection->SetEnumValue(_current_message, _current_field_desc, stoi(value));
			}
			else
			{
				const google::protobuf::EnumValueDescriptor* _enum_value_descriptor = _current_field_desc->enum_type()->FindValueByName(value);
				if (_enum_value_descriptor != nullptr)
					_reflection->SetEnum(_current_message, _current_field_desc, _enum_value_descriptor);
				else
					_reflection->SetEnumValue(_current_message, _current_field_desc, 0);
			}
		}
		break;
	case FieldDescriptor::TYPE_MESSAGE:
	{
		if (_current_field_desc->is_repeated())
		{
			Message* _new_message = _reflection->AddMessage(_current_message, _current_field_desc, &message_factory);
			// It must be JSON format
			if (util::JsonStringToMessage(value, _new_message) != util::Status::OK)
			{
				LOG_E(TAG, "mRNA::assign(%s) failed. Invalid JSON content of message %s", message->GetDescriptor()->full_name().c_str(), _new_message->GetDescriptor()->full_name().c_str());
				return;
			}
		}
		else
		{
			Message* _new_message = _reflection->MutableMessage(message, _current_field_desc, &message_factory);
			// It must be JSON format
			if (util::JsonStringToMessage(value, _new_message) != util::Status::OK)
			{
				LOG_E(TAG, "mRNA::assign(%s) failed. Invalid JSON content of _current_message %s", message->GetDescriptor()->full_name().c_str(), _new_message->GetDescriptor()->full_name().c_str());
				return;
			}
		}
	}
	break;
	default:
		break;
	}
}

bool mRNA::decompose(const DynaArray& name, const DynaArray& payload, const DynaArray& field_name)
{
	// translate via patterns
	if (desc_pool_ == nullptr)
		return false;
	const Descriptor* _desc = desc_pool_->FindMessageTypeByName(name.str());
	if (_desc == nullptr)
	{
		LOG_W(TAG, "mRNA::decompose() schema of message \"%s\" not found", name.data());
		return false;
	}

	const FieldDescriptor* _field_desc = _desc->FindFieldByName(field_name.str());
	if (_field_desc->type() != FieldDescriptor::TYPE_MESSAGE)
		return false;
	else
		return unpack(_field_desc->message_type()->full_name(), payload, _field_desc->full_name());
}

Array<String>  mRNA::SplitByString(const String& input, const String& separator)
{
	Array<String> output;
	size_t begin = 0;
	size_t end = input.find(separator, begin);
	while (end != String::npos)
	{
		output.push_back(input.substr(begin, end - begin));
		begin = end + separator.size();
		end = input.find(separator, begin);
	}
	output.push_back(input.substr(begin, end));
	return output;
}