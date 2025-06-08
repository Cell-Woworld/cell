#include "Model.h"
#include "dynarray.h"
#include <iostream>
#include "internal/utils/cell_utils.h"

#define TAG "Model"

BIO_BEGIN_NAMESPACE
class Content
{
public:
	Content() : internal_use(false) {};
	Content(const ContentByString& src);
	virtual ~Content() {};
public:
	DynaArray type;
	DynaArray data;
	bool internal_use;
};

class ContentByString : public zpp::serializer::polymorphic
{
public:
	ContentByString() : internal_use(false) {};
	ContentByString(const Content& src);
	virtual ~ContentByString() {};
public:
	friend zpp::serializer::access;
	template <typename Archive, typename Self>
	static void serialize(Archive& archive, Self& self)
	{
		archive(self.type, self.data, self.internal_use);
	}

public:
	String type;
	String data;
	bool internal_use;
};

Content::Content(const ContentByString& src)
{
	type = src.type;
	data = src.data;
	internal_use = src.internal_use;
};

ContentByString::ContentByString(const Content& src)
{
	type = src.type.str();
	data = src.data.str();
	internal_use = src.internal_use;
};

class JsonContent
{
public:
	JsonContent() : internal_use(false) {};
	JsonContent(const JsonContentByString& src);
	virtual ~JsonContent() {};
public:
	nlohmann::json data;
	DynaArray type;
	bool internal_use;
};

class JsonContentByString : public zpp::serializer::polymorphic
{
public:
	JsonContentByString() : internal_use(false) {};
	JsonContentByString(const JsonContent& src);
	virtual ~JsonContentByString() {};
public:
	friend zpp::serializer::access;
	template <typename Archive, typename Self>
	static void serialize(Archive& archive, Self& self)
	{
		archive(self.type, self.data, self.internal_use);
	}

public:
	String type;
	String data;
	bool internal_use;
};

JsonContent::JsonContent(const JsonContentByString& src)
{
	type = src.type;
	try
	{
		data = nlohmann::json::parse(src.data);
	}
	catch (nlohmann::json::exception& e)
	{
		LOG_W(TAG, "parse error, exception=%s", e.what());
		data = nlohmann::json::object();
	}
	internal_use = src.internal_use;
};

JsonContentByString::JsonContentByString(const JsonContent& src)
{
	type = src.type.str();
	try
	{
		data = src.data.dump();
	}
	catch (nlohmann::json::exception& e)
	{
		LOG_W(TAG, "dump error, exception=%s", e.what());
		data = "{}";
	}
	internal_use = src.internal_use;
};
BIO_END_NAMESPACE

namespace
{
	zpp::serializer::register_types<
		zpp::serializer::make_type<BioSys::ContentByString, zpp::serializer::make_id("v1::BioSys::ContentByString")>,
		zpp::serializer::make_type<BioSys::JsonContentByString, zpp::serializer::make_id("v1::BioSys::JsonContentByString")>
	> _;
} // <anynymous namespace>

USING_BIO_NAMESPACE

#ifndef STATIC_API
extern "C"
{
	PUBLIC_API IModel* CreateInstance()
	{
		return new Model();
	}
}
#endif

bool CompareDynaArray::operator()(const DynaArray& str1, const DynaArray& str2) const
{
	return strcmp(str1.data(), str2.data()) < 0;
}

Model::Model()
{
}

Model::~Model()
{
	storage_.clear();
}

void Model::Read(const DynaArray& name, DynaArray& type, DynaArray& value, bool auto_conversion)
{
	if (storage_.find(name) != storage_.end())
	{
		type= storage_[name].type;
		value = storage_[name].data;
	}
	else if (auto_conversion && json_storage_.find(name) != json_storage_.end())
	{
		type = json_storage_[name].type;
		try
		{
			value = json_storage_[name].data.dump();
		}
		catch (const nlohmann::json::exception& e)
		{
			LOG_W(TAG, "Error when dumping %s as string, message=%s", name.data(), e.what());
			value = "";
		}
	}
}

void Model::Write(const DynaArray& name, const DynaArray& type, const ByteArray& value, bool internal_use)
{
	Write(name, type, DynaArray(value), internal_use);
}

void Model::Write(const DynaArray& name, const DynaArray& type, const DynaArray& value, bool internal_use)
{
	RemoveSpecificStorage(name, false, json_storage_);
	storage_[name].type = type;
	storage_[name].data = value;
	storage_[name].internal_use = internal_use;
}

template <typename T>
bool Model::RemoveSpecificStorage(const DynaArray& name, bool internal_only, T& storage)
{
	const String& _model_name = name.str();
	size_t _pos = _model_name.find(".*");
	bool _ret_value = false;
	if (_pos != String::npos)
	{
		String _prefix = _model_name.substr(0, _pos + 1);
		for (auto _itr = storage.lower_bound(_prefix); _itr != storage.end() && _itr->first.str().compare(0, _prefix.size(), _prefix) == 0; )
		{
			if (internal_only)
			{
				if (_itr->second.internal_use)
				{
					_itr = storage.erase(_itr);
					_ret_value = true;
				}
				else
				{
					++_itr;
				}
			}
			else
			{
				_itr = storage.erase(_itr);
				_ret_value = true;
			}
		}
	}
	else
	{
		auto _itr = storage.find(name);
		if (_itr != storage.end())
		{
			if (!internal_only)
			{
				storage.erase(_itr);
				_ret_value = true;
			}
			else
			{
				if (_itr->second.internal_use)
				{
					storage.erase(_itr);
					_ret_value = true;
				}
			}
		}
	}
	return _ret_value;
}

void Model::Remove(const DynaArray& name, bool internal_only)
{
	RemoveSpecificStorage(name, internal_only, storage_);
	RemoveSpecificStorage(name, internal_only, json_storage_);
}

void Model::Clone(const DynaArray& target_name, const DynaArray& src_name)
{
	if (CloneSpecificStorage(target_name, src_name, storage_) == false)
	{
		CloneSpecificStorage(target_name, src_name, json_storage_);
	}
}

template <typename T>
bool Model::CloneSpecificStorage(const DynaArray& target_name, const DynaArray& src_name, T& storage)
{
	bool _ret_value = false;
	const String& _src_name = src_name.str();
	size_t _src_pos = _src_name.find(".*");
	if (_src_pos != String::npos)
	{
		const String& _target_name = target_name.str();
		size_t _target_pos = String::npos;
		if ((_target_pos = _target_name.find("[*]")) == String::npos)
		{
			size_t _target_pos = _target_name.find(".*");
			String _src_prefix = _src_name.substr(0, _src_pos + 1);
			String _target_prefix = _target_pos==String::npos? _target_name:_target_name.substr(0, _target_pos + 1);
			if (_target_prefix.find(_src_prefix) != String::npos)
			{
				LOG_E(TAG, "Invalid recursive cloning from \"%s\" to \"%s\"", src_name.data(), target_name.data());
				throw std::nested_exception();
				return false;
			}
			for (auto _itr = storage.lower_bound(_src_prefix); _itr != storage.end() && _itr->first.str().compare(0, _src_prefix.size(), _src_prefix) == 0; ++_itr)
			{
				String _postfix = _itr->first.str().substr(_src_prefix.size());
				if (_postfix != TAG_PACKET && _postfix != TAG_COLUMNS_SOURCE_MODEL_LIST)
				{
					Write(_target_prefix + _postfix, _itr->second.type, _itr->second.data, false);
					_ret_value = true;
				}
				else
				{
					LOG_D(TAG, "Discard %s when copy %s* to %s*", _postfix.c_str(), _src_prefix.c_str(), _target_prefix.c_str());
				}
			}
			_ret_value = false;		// for searching in the next storage
		}
		else
		{
			String _src_prefix = _src_name.substr(0, _src_pos + 1);
			using namespace nlohmann;
			size_t _target_pos0 = _target_name.find("[");
			String _target_model_name = _target_name.substr(0, _target_pos0);
			json _root_target;
			DynaArray _type;
			Read(_target_model_name, _type, _root_target);
			if (!_root_target.is_null())
			{
				String _target_path_str = _target_name.substr(_target_pos0, _target_pos - _target_pos0);
				json* _target = nullptr;
				if (_target_path_str == "[]")
				{
					_root_target.push_back(json::object());
					_target = &_root_target.back();
				}
				else
				{
					_target_path_str.erase(std::remove(_target_path_str.begin(), _target_path_str.end(), '\"'), _target_path_str.end());
					_target_path_str.erase(std::remove(_target_path_str.begin(), _target_path_str.end(), ']'), _target_path_str.end());
					std::replace(_target_path_str.begin(), _target_path_str.end(), '[', '/');
					json::json_pointer _target_ptr(_target_path_str);
					if (_root_target[_target_ptr].is_null())
					{
						_root_target[_target_ptr] = json::object();
					}
					_target = &_root_target[_target_ptr];
				}
				if (_target != nullptr && _target->is_object())
				{
					for (auto _itr = storage.lower_bound(_src_prefix); _itr != storage.end() && _itr->first.str().compare(0, _src_prefix.size(), _src_prefix) == 0; ++_itr)
					{
						DynaArray _type, data;
						Read(_itr->first, _type, data);
						ByteArray _data(data.size());
						memcpy(_data.data(), data.data(), data.size());
						zpp::serializer::memory_input_archive in(_data);
						// support String only
						String _postfix = _itr->first.str().substr(_src_prefix.size());
						String _str_value;
						int _int_value;
						unsigned int _uint_value;
						long long _ll_value;
						unsigned long long _ull_value;
						double _double_value;
						bool _bool_value;
						if (typeid(_str_value).name() == _type.str())
						{
							in(_str_value);
							(*_target)[_postfix] = _str_value;
						}
						else if (typeid(_int_value).name() == _type.str())
						{
							in(_int_value);
							(*_target)[_postfix] = std::to_string(_int_value);
						}
						else if ( typeid(_uint_value).name() == _type.str())
						{
							in(_uint_value);
							(*_target)[_postfix] = std::to_string(_uint_value);
						}
						else if (typeid(_ll_value).name() == _type.str())
						{
							in(_ll_value);
							(*_target)[_postfix] = std::to_string(_ll_value);
						}
						else if (typeid(_ull_value).name() == _type.str())
						{
							in(_ull_value);
							(*_target)[_postfix] = std::to_string(_ull_value);
						}
						else if (typeid(_double_value).name() == _type.str())
						{
							in(_double_value);
							(*_target)[_postfix] = std::to_string(_double_value);
						}
						else if (typeid(_bool_value).name() == _type.str())
						{
							in(_bool_value);
							(*_target)[_postfix] = std::to_string(_bool_value);
						}
						else
						{
							LOG_D(TAG, "unsupported type: %s", _type.data());
						}
					}
					Write(_target_model_name, _type, _root_target);
				}
				_ret_value = true;
			}
			else
			{
				LOG_W(TAG, "Invalid data in target model: %s", _target_model_name.c_str());
			}
		}
	}
	else if ((_src_pos = _src_name.find("[*]")) != String::npos)
	{
		const String& _target_name = target_name.str();
		size_t _target_pos = String::npos;
		if ((_target_pos = _target_name.find("[*]")) == String::npos)
		{	// copy src[*] to target.* or target[]
			const String& _target_name = target_name.str();
			size_t _target_pos0 = _target_name.find("[");
			size_t _target_pos1 = _target_name.find(".*");
			size_t _target_pos2 = _target_name.find("[]");
			size_t _src_pos0 = _src_name.find("[");
			String _src_model_name = _src_name.substr(0, _src_pos0);
			String _src_path_str = _src_name.substr(_src_pos0, _src_pos - _src_pos0);
			_src_path_str.erase(std::remove(_src_path_str.begin(), _src_path_str.end(), '\"'), _src_path_str.end());
			_src_path_str.erase(std::remove(_src_path_str.begin(), _src_path_str.end(), ']'), _src_path_str.end());
			std::replace(_src_path_str.begin(), _src_path_str.end(), '[', '/');
			String _target_prefix = _target_pos1 != String::npos ? _target_name.substr(0, _target_pos1) : (_target_pos0 != String::npos ? _target_name.substr(0, _target_pos0) : _target_name);
			using namespace nlohmann;
			json _root_src;
			DynaArray type;
			Read(_src_model_name, type, _root_src);
			if (!_root_src.is_null())
			{
				json::json_pointer _src_ptr(_src_path_str);
				if (_root_src[_src_ptr].is_object())
				{
					if (_target_pos2 != String::npos)
					{
						json _root_target;
						DynaArray type;
						Read(_target_prefix, type, _root_target);
						if (_root_target.empty())
						{
							_root_target = json::array();
						}
						if (_root_target.is_array())
						{
							for (auto it : _root_src[_src_ptr].items())
							{
								_root_target.emplace_back(it.value());
							}
							ByteArray data;
							zpp::serializer::memory_output_archive out(data);
							out(_root_target.dump());
							Write(_target_prefix, typeid(String).name(), data, false);
						}
					}
					else
					{
						for (auto it : _root_src[_src_ptr].items())
						{
							ByteArray data;
							zpp::serializer::memory_output_archive out(data);
							const String name = _target_prefix + "." + it.key();
							String type;
							switch (it.value().type())
							{
							case json::value_t::number_integer:
								out(it.value().get<int>());
								type = typeid(int).name();
								break;
							case json::value_t::number_unsigned:
								out(it.value().get<unsigned int>());
								type = typeid(unsigned int).name();
								break;
							case json::value_t::number_float:
								out(it.value().get<double>());
								type = typeid(double).name();
								break;
							case json::value_t::string:
								out(it.value().get<String>());
								type = typeid(String).name();
								break;
							case json::value_t::boolean:
								out(it.value().get<bool>());
								type = typeid(bool).name();
								break;
							case json::value_t::array:
							case json::value_t::object:
								out(it.value().dump());
								type = typeid(String).name();
								break;
							default:
								break;
							}

							Write(name, type, data, false);
						}
					}
				}
				else if (_root_src[_src_ptr].is_array())
				{
					if (_target_pos2 != String::npos)
					{
						json _root_target;
						DynaArray type;
						Read(_target_prefix, type, _root_target);
						if (_root_target.empty())
						{
							_root_target = json::array();
						}
						if (_root_target.is_array())
						{
							std::move(_root_src[_src_ptr].begin(), _root_src[_src_ptr].end(), std::back_inserter(_root_target));
							Write(_target_prefix, typeid(json).name(), _root_target, false);
						}
						else if (_root_target.is_object())
						{
							String _target_path_str = "";
							if (_target_pos1 == String::npos)
							{
								_target_path_str = _target_name.substr(_target_pos0, _target_pos2 - _target_pos0);
								_target_path_str.erase(std::remove(_target_path_str.begin(), _target_path_str.end(), '\"'), _target_path_str.end());
								_target_path_str.erase(std::remove(_target_path_str.begin(), _target_path_str.end(), ']'), _target_path_str.end());
								std::replace(_target_path_str.begin(), _target_path_str.end(), '[', '/');
							}
							json::json_pointer _target_ptr(_target_path_str);
							if (_root_target[_target_ptr].is_array())
							{
								std::move(_root_src[_src_ptr].begin(), _root_src[_src_ptr].end(), std::back_inserter(_root_target[_target_ptr]));
								Write(_target_prefix, typeid(json).name(), _root_target, false);
							}
						}
					}
					else
					{
						for (auto it : _root_src[_src_ptr].items())
						{
							ByteArray data;
							zpp::serializer::memory_output_archive out(data);
							const String name = _target_prefix + "." + it.key();
							String type;
							switch (it.value().type())
							{
							case json::value_t::number_integer:
								out(it.value().get<int>());
								type = typeid(int).name();
								break;
							case json::value_t::number_unsigned:
								out(it.value().get<unsigned int>());
								type = typeid(unsigned int).name();
								break;
							case json::value_t::number_float:
								out(it.value().get<double>());
								type = typeid(double).name();
								break;
							case json::value_t::string:
								out(it.value().get<String>());
								type = typeid(String).name();
								break;
							case json::value_t::boolean:
								out(it.value().get<bool>());
								type = typeid(bool).name();
								break;
							case json::value_t::array:
							case json::value_t::object:
								out(it.value().dump());
								type = typeid(String).name();
								break;
							default:
								break;
							}

							Write(name, type, data, false);
						}
					}
				}
				else
				{
					LOG_D(TAG, "Invalid clone from %s to %s", src_name.data(), target_name.data());
				}
				_ret_value = true;
			}
			else
			{
				LOG_W(TAG, "Invalid data in source model: %s", _src_model_name.c_str());
			}
		}
		else
		{	// copy src[*] to target[*]
			using namespace nlohmann;
			size_t _src_pos0 = _src_name.find("[");
			String _src_model_name = _src_name.substr(0, _src_pos0);
			String _src_path_str = _src_name.substr(_src_pos0, _src_pos - _src_pos0);
			_src_path_str.erase(std::remove(_src_path_str.begin(), _src_path_str.end(), '\"'), _src_path_str.end());
			_src_path_str.erase(std::remove(_src_path_str.begin(), _src_path_str.end(), ']'), _src_path_str.end());
			std::replace(_src_path_str.begin(), _src_path_str.end(), '[', '/');
			json _root_src;
			DynaArray type;
			Read(_src_model_name, type, _root_src);
			if (_root_src.is_null())
			{
				LOG_W(TAG, "Invalid data in source model: %s", _src_model_name.c_str());
			}
			else
			{
				size_t _target_pos0 = _target_name.find("[");
				String _target_model_name = _target_name.substr(0, _target_pos0);
				json _root_target;
				DynaArray _type;
				Read(_target_model_name, _type, _root_target);
				if (_root_target.is_null())
				{
					LOG_W(TAG, "Invalid data in target model: %s", _target_model_name.c_str());
				}
				else
				{
					String _target_path_str = _target_name.substr(_target_pos0, _target_pos - _target_pos0);
					json* _target = nullptr;
					json::json_pointer _src_ptr(_src_path_str);
					if (_target_path_str == "[]")
					{
						if (_root_src[_src_ptr].is_object())
						{
							_root_target.push_back(json::object());
							_target = &_root_target.back();
							_target->update(_root_src[_src_ptr]);
						}
						else
						{
							std::move(_root_src[_src_ptr].begin(), _root_src[_src_ptr].end(), std::back_inserter(_root_target));
						}
					}
					else
					{
						_target_path_str.erase(std::remove(_target_path_str.begin(), _target_path_str.end(), '\"'), _target_path_str.end());
						_target_path_str.erase(std::remove(_target_path_str.begin(), _target_path_str.end(), ']'), _target_path_str.end());
						std::replace(_target_path_str.begin(), _target_path_str.end(), '[', '/');
						json::json_pointer _target_ptr(_target_path_str);
						_root_target[_target_ptr].update(_root_src[_src_ptr]);
					}
					Write(_target_model_name, _type, _root_target);
					_ret_value = true;
				}
			}
		}
	}
	else
	{
		if (target_name != src_name && storage.find(src_name) != storage.end())
		{
			storage[target_name] = storage[src_name];
			storage[target_name].internal_use = false;
			_ret_value = true;
		}
	}
	return _ret_value;
}

bool Model::Persist(const DynaArray& path, const DynaArray& id, const DynaArray& version)
{
	const int MAX_VERSION_SIZE = 100;
	FILE* _file = fopen((path + id).data(), "wb");
	if (_file != nullptr)
	{
		ByteArray raw_data;
		{
			zpp::serializer::memory_output_archive out(raw_data);
			Map<String, ContentByString> _storage;
			TypeConversion(storage_, _storage);
			out(_storage);
		}
		ByteArray json_data;
		{
			zpp::serializer::memory_output_archive out(json_data);
			Map<String, JsonContentByString> _json_storage;
			TypeConversion(json_storage_, _json_storage);
			out(_json_storage);
		}
		assert(version.size() <= MAX_VERSION_SIZE);
		char _version[MAX_VERSION_SIZE + 1];
		memset(_version, 0, MAX_VERSION_SIZE + 1);
		strncpy(_version, version.data(), MAX_VERSION_SIZE);
		size_t _raw_data_size = raw_data.size();
		size_t _json_data_size = json_data.size();
		fwrite(_version, sizeof(char), MAX_VERSION_SIZE, _file);
		fwrite(&_raw_data_size, sizeof(size_t), 1, _file);
		fwrite(&_json_data_size, sizeof(size_t), 1, _file);
		fwrite(raw_data.data(), sizeof(char), raw_data.size(), _file);
		fwrite(json_data.data(), sizeof(char), json_data.size(), _file);
		fclose(_file);
		_file = nullptr;
		return true;
	}
	else
		return false;
}

bool Model::RestorePersistence(const DynaArray& path, const DynaArray& id, const DynaArray& version)
{
	const int MAX_VERSION_SIZE = 100;
	FILE* _file = fopen((path + id).data(), "rb");
	if (_file != nullptr)
	{
		fseek(_file, 0, SEEK_END);
		size_t _file_size = ftell(_file);
		fseek(_file, 0, SEEK_SET);
		if (_file_size > 0)
		{
			char _version[MAX_VERSION_SIZE + 1];
			size_t raw_data_size = 0, json_data_size = 0;
			fread(_version, sizeof(char), MAX_VERSION_SIZE, _file);
			fread(&raw_data_size, sizeof(size_t), 1, _file);
			fread(&json_data_size, sizeof(size_t), 1, _file);
			if (version != _version || raw_data_size == 0 || json_data_size == 0)
			{	// expiry or invalid snapshot, ignore it
				fclose(_file);
				_file = nullptr;
				return false;
			}
			try
			{
				ByteArray _raw_data(raw_data_size);
				fread((char*)_raw_data.data(), sizeof(char), _raw_data.size(), _file);
				zpp::serializer::memory_input_archive in(_raw_data);
				Map<String, ContentByString> _storage;
				in(_storage);
				storage_.clear();
				TypeConversion(_storage, storage_);

				ByteArray _json_data(json_data_size);
				fread((char*)_json_data.data(), sizeof(char), _json_data.size(), _file);
				zpp::serializer::memory_input_archive in_json(_json_data);
				Map<String, JsonContentByString> _json_storage;
				in_json(_json_storage);
				json_storage_.clear();
				TypeConversion(_json_storage, json_storage_);
			}
			catch (const std::exception& e)
			{
				LOG_E(TAG, "deserialization error, message=%s", e.what());
				fclose(_file);
				_file = nullptr;
				return false;
			}
			fclose(_file);
			_file = nullptr;
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

bool Model::RemovePersistence(const DynaArray& path, const DynaArray& id)
{
	return remove((path + id).data()) == 0;
}

template<typename C>
void Model::TypeConversion(const Map<String, ContentByString>& src, Map<DynaArray, Content, C>& dest) {
	std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [this](const Pair<String, ContentByString>& elem)->Pair<DynaArray, Content> {
		return { DynaArray(elem.first), Content(elem.second) };
		});
}

template<typename C>
void Model::TypeConversion(const Map<DynaArray, Content, C>& src, Map<String, ContentByString>& dest) {
	std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<DynaArray, Content>& elem)->Pair<String, ContentByString> {
		return { elem.first.str(), ContentByString(elem.second) };
		});
}

template<typename C>
void Model::TypeConversion(const Map<String, JsonContentByString>& src, Map<DynaArray, JsonContent, C>& dest) {
	std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [this](const Pair<String, JsonContentByString>& elem)->Pair<DynaArray, JsonContent> {
		return { DynaArray(elem.first), JsonContent(elem.second) };
		});
}

template<typename C>
void Model::TypeConversion(const Map<DynaArray, JsonContent, C>& src, Map<String, JsonContentByString>& dest) {
	std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<DynaArray, JsonContent>& elem)->Pair<String, JsonContentByString> {
		return { elem.first.str(), JsonContentByString(elem.second) };
		});
}

void Model::Snapshot()
{
	snapshot_ = storage_;
	json_snapshot_ = json_storage_;
}

void Model::RevertSnapshot()
{
	storage_ = snapshot_;
	snapshot_.clear();
	json_storage_ = json_snapshot_;
	json_snapshot_.clear();
}

void Model::Read(const DynaArray& name, DynaArray& type, nlohmann::json& value, bool auto_conversion)
{
	value = nlohmann::json();
	if (json_storage_.find(name) != json_storage_.end())
	{
		type = json_storage_[name].type;
		value = json_storage_[name].data;
	}
	else if (auto_conversion && storage_.find(name) != storage_.end())
	{
		using namespace nlohmann;
		try
		{
			DynaArray _stored_type = storage_[name].type;
			DynaArray _buf = storage_[name].data.str();
			String _value;
			try {
				Deserialize(_stored_type.data(), _buf, _value);
				const String whitespaces(" \t\f\v\n\r");
				std::size_t _the_first_nonspace = _value.find_first_not_of(whitespaces);
				std::size_t _the_last_nonspace = _value.find_last_not_of(whitespaces);
				if (_value.size() >= 2 && (_value[_the_first_nonspace] == '[' && _value[_the_last_nonspace] == ']' || _value[_the_first_nonspace] == '{' && _value[_the_last_nonspace] == '}'))
					value = json::parse(_value);
				else
				{
					Array<String> _string_array;
					Deserialize(_stored_type.data(), _buf, _string_array);
					if (_string_array.size() > 0)
					{
						value = json::array();
						for (const auto& elem : _string_array)
							value.emplace_back(elem);
					}
				}
			}
			catch (const json::exception& e)
			{
				LOG_D(TAG, "Error when parsing %s as JSON, message=%s", _value.c_str(), e.what());
				String _escaped_value;
				EscapeJSON(_value, _escaped_value);
				if (_escaped_value != "")
				{
					try
					{
						value = json::parse(_escaped_value);
						LOG_I(TAG, "JSON string escaped");
					}
					catch (const std::exception& e)
					{
						LOG_E(TAG, "Escaped!! Error when parsing %s as JSON, message=%s", _escaped_value.c_str(), e.what());
						return;
					}
				}
			}
			catch (...)
			{
				const String& _name = name.str();
				if (_name.substr(0, strlen("Bio.Cell.")) == "Bio.Cell.")
				{
					LOG_D(TAG, "No such key in Model, key name: %s", _name.c_str());
				}
				else
				{
					//LOG_W(TAG, "No such key in Model, key name: %s", _name.c_str());
					LOG_D(TAG, "No such key in Model, key name: %s", _name.c_str());
				}
				return;
			}
			type = typeid(value).name();
		}
		catch (const std::exception& e)
		{
			LOG_E(TAG, "Error when parsing model \"%s\" with \"%s\" as JSON, message=%s", name.data(), storage_[name].data.data(), e.what());
		}
	}
}

void Model::Write(const DynaArray& name, const DynaArray& type, const nlohmann::json& value, bool internal_use)
{
	RemoveSpecificStorage(name, false, storage_);
	json_storage_[name].type = type;
	json_storage_[name].data = value;
	json_storage_[name].internal_use = internal_use;
}

template<typename T>
void Model::Deserialize(const char* type, const DynaArray& data, T& value)
{
	ByteArray _data(data.size());
	memcpy(_data.data(), data.data(), data.size());
	zpp::serializer::memory_input_archive in(_data);
	in(value);
}