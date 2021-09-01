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
class Model : public IModel
{
public:
	Model();
	virtual ~Model();

private:
	virtual void Read(const DynaArray& name, DynaArray& type, DynaArray& value);
	virtual void Write(const DynaArray& name, const DynaArray& type, const DynaArray& value, bool internal_use = false);
	virtual void Remove(const DynaArray& name, bool internal_only = false);
	virtual void Clone(const DynaArray& target_name, const DynaArray& src_name);

	virtual bool Persist(const DynaArray& path, const DynaArray& id);
	virtual bool RestorePersistence(const DynaArray& path, const DynaArray& id);
	virtual bool RemovePersistence(const DynaArray& path, const DynaArray& id);
	virtual void Snapshot();
	virtual void RevertSnapshot();

	template<typename C>
	void TypeConversion(const Map<DynaArray, Content, C>& src, Map<String, ContentByString>& dest);
	template<typename C>
	void TypeConversion(const Map<String, ContentByString>& src, Map<DynaArray, Content, C>& dest);
private:
	std::map<DynaArray, Content, CompareDynaArray> storage_;
	std::map<DynaArray, Content, CompareDynaArray> snapshot_;
};

BIO_END_NAMESPACE