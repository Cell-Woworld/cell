#pragma once
#include "type_def.h"

class DynaArray;

BIO_BEGIN_NAMESPACE

class IModel
{
public:
	virtual ~IModel() {};

	virtual void Read(const DynaArray& name, DynaArray& type, DynaArray& value, bool auto_conversion = true) = 0;
	virtual void Write(const DynaArray& name, const DynaArray& type, const DynaArray& value, bool internal_use = false) = 0;
	virtual void Remove(const DynaArray& name, bool internal_only = false) = 0;
	virtual void Clone(const DynaArray& target_name, const DynaArray& src_name) = 0;

	virtual void Read(const DynaArray& name, DynaArray& type, nlohmann::json& value, bool auto_conversion = true) = 0;
	virtual void Write(const DynaArray& name, const DynaArray& type, const nlohmann::json& value, bool internal_use = false) = 0;

	virtual bool Persist(const DynaArray& path, const DynaArray& id, const DynaArray& version) = 0;
	virtual bool RestorePersistence(const DynaArray& path, const DynaArray& id, const DynaArray& version) = 0;
	virtual bool RemovePersistence(const DynaArray& path, const DynaArray& id) = 0;
	virtual void Snapshot() = 0;
	virtual void RevertSnapshot() = 0;
};

typedef IModel* (*CREATE_MODEL_INSTANCE_FUNCTION)();

BIO_END_NAMESPACE