#pragma once
#include "internal/IModel.h"
#include "internal/utils/serializer.h"

BIO_BEGIN_NAMESPACE

class CompareDynaArray
{
public:
	bool operator()(const DynaArray& str1, const DynaArray& str2) const;
};

class Content;
class ContentByString;
class JsonContent;
class JsonContentByString;
class Model : public IModel
{
public:
	Model();
	virtual ~Model();

private:
	virtual void Read(const DynaArray& name, DynaArray& type, DynaArray& value, bool auto_conversion = true);
	virtual void Write(const DynaArray& name, const DynaArray& type, const DynaArray& value, bool internal_use = false);
	virtual void Write(const DynaArray& name, const DynaArray& type, const ByteArray& value, bool internal_use = false);
	virtual void Remove(const DynaArray& name, bool internal_only = false);
	virtual void Clone(const DynaArray& target_name, const DynaArray& src_name);

	virtual void Read(const DynaArray& name, DynaArray& type, nlohmann::json& value, bool auto_conversion = true);
	virtual void Write(const DynaArray& name, const DynaArray& type, const nlohmann::json& value, bool internal_use = false);

	virtual bool Persist(const DynaArray& path, const DynaArray& id, const DynaArray& version);
	virtual bool RestorePersistence(const DynaArray& path, const DynaArray& id, const DynaArray& version);
	virtual bool RemovePersistence(const DynaArray& path, const DynaArray& id);
	virtual void Snapshot();
	virtual void RevertSnapshot();

	template<typename C>
	void TypeConversion(const Map<DynaArray, Content, C>& src, Map<String, ContentByString>& dest);
	template<typename C>
	void TypeConversion(const Map<String, ContentByString>& src, Map<DynaArray, Content, C>& dest);
	template<typename C>
	void TypeConversion(const Map<DynaArray, JsonContent, C>& src, Map<String, JsonContentByString>& dest);
	template<typename C>
	void TypeConversion(const Map<String, JsonContentByString>& src, Map<DynaArray, JsonContent, C>& dest);

	template <typename T> bool RemoveSpecificStorage(const DynaArray& name, bool internal_only, T& storage);
	template <typename T> bool CloneSpecificStorage(const DynaArray& target_name, const DynaArray& src_name, T& storage);
	template<typename T>
	inline const char* GetType(const T& value);
	template<typename T>
	void Deserialize(const char* type, const DynaArray& data, T& value);
private:
	std::map<DynaArray, Content, CompareDynaArray> storage_;
	std::map<DynaArray, Content, CompareDynaArray> snapshot_;
	std::map<DynaArray, JsonContent, CompareDynaArray> json_storage_;
	std::map<DynaArray, JsonContent, CompareDynaArray> json_snapshot_;
};

BIO_END_NAMESPACE