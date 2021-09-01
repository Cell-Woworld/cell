#include "Model.h"
#include "dynarray.h"
#include <iostream>

#define TAG "Model"

BIO_BEGIN_NAMESPACE
class Content
{
public:
	Content() {};
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
	ContentByString() {};
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
BIO_END_NAMESPACE

namespace
{
	zpp::serializer::register_types<
		zpp::serializer::make_type<BioSys::ContentByString, zpp::serializer::make_id("v1::BioSys::ContentByString")>
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

void Model::Read(const DynaArray& name, DynaArray& type, DynaArray& value)
{
	//zpp::serializer::memory_input_archive in(storage_[name]);
	//in(value);
	if (storage_.find(name) != storage_.end())
	{ 
		type= storage_[name].type;
		value = storage_[name].data;
	}
}

void Model::Write(const DynaArray& name, const DynaArray& type, const DynaArray& value, bool internal_use)
{
	//zpp::serializer::memory_output_archive out(storage_[name]);
	//out(value);
	storage_[name].type = type;
	storage_[name].data = value;
	storage_[name].internal_use = internal_use;
}

void Model::Remove(const DynaArray& name, bool internal_only)
{
	const String& _model_name = name.str();
	size_t _pos = _model_name.find(".*");
	if (_pos != String::npos)
	{
		String _prefix = _model_name.substr(0, _pos+1);
		for (auto _itr = storage_.lower_bound(_prefix); _itr != storage_.end() && _itr->first.str().compare(0, _prefix.size(), _prefix) == 0; )
			_itr = (internal_only ? (_itr->second.internal_use ? storage_.erase(_itr) : ++_itr) : storage_.erase(_itr));
	}
	else
	{
		auto _itr = storage_.find(name);
		if (_itr != storage_.end())
		{
			if (!internal_only)
				storage_.erase(_itr);
			else
			{
				if (_itr->second.internal_use)
					storage_.erase(_itr);
			}
		}
	}
}

void Model::Clone(const DynaArray& target_name, const DynaArray& src_name)
{
	const String& _src_name = src_name.str();
	size_t _src_pos = _src_name.find(".*");
	if (_src_pos != String::npos)
	{
		const String& _target_name = target_name.str();
		size_t _target_pos = _target_name.find(".*");
		String _src_prefix = _src_name.substr(0, _src_pos + 1);
		String _target_prefix = _target_pos==String::npos? _target_name:_target_name.substr(0, _target_pos);
		for (auto _itr = storage_.lower_bound(_src_prefix); _itr != storage_.end() && _itr->first.str().compare(0, _src_prefix.size(), _src_prefix) == 0; ++_itr)
		{
			String _postfix = _itr->first.str().substr(_src_prefix.size() - 1);
			storage_[_target_prefix + _postfix] = _itr->second;
		}
	}
	else
	{
		if (target_name != src_name && storage_.find(src_name) != storage_.end())
		{
			storage_[target_name] = storage_[src_name];
			storage_[target_name].internal_use = false;
		}
	}
}

bool Model::Persist(const DynaArray& path, const DynaArray& id)
{
	FILE* _file = fopen((path + id).data(), "wb");
	if (_file != nullptr)
	{
		ByteArray data;
		zpp::serializer::memory_output_archive out(data);
		Map<String, ContentByString> _storage;
		TypeConversion(storage_, _storage);
		out(_storage);
		fwrite(data.data(), sizeof(char), data.size(), _file);
		fclose(_file);
		_file = nullptr;
		return true;
	}
	else
		return false;
}

bool Model::RestorePersistence(const DynaArray& path, const DynaArray& id)
{
	FILE* _file = fopen((path + id).data(), "rb");
	if (_file != nullptr)
	{
		fseek(_file, 0, SEEK_END);
		size_t _file_size = ftell(_file);
		fseek(_file, 0, SEEK_SET);
		if (_file_size > 0)
		{
			storage_.clear();
			ByteArray _data(_file_size);
			fread((char*)_data.data(), sizeof(char), _data.size(), _file);
			zpp::serializer::memory_input_archive in(_data);
			Map<String, ContentByString> _storage;
			try
			{
				in(_storage);
				TypeConversion(_storage, storage_);
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

void Model::Snapshot()
{
	snapshot_ = storage_;
}

void Model::RevertSnapshot()
{
	storage_ = snapshot_;
	snapshot_.clear();
}
