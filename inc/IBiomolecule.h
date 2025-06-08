#pragma once
#include "type_def.h"

BIO_BEGIN_NAMESPACE

class IModel;
class IBiomolecule
{
public:
	IBiomolecule(IBiomolecule* owner) : owner_(owner), name_(""), full_name_("") {
	};
	virtual ~IBiomolecule() {
		owner_ = nullptr;
	};

public:
	const DynaArray& name() { return name_; };
	const DynaArray& full_name() { return full_name_; };
	void name(const DynaArray& name) { name_ = name; };
	void full_name(const DynaArray& name) { full_name_ = name; };
	IBiomolecule* owner() const { return owner_; };

public:
	virtual bool init(const char* name) {
		name_ = name;
		if (owner() != nullptr)
			full_name_ = owner()->full_name() + "." + name_;
		else
			full_name_ = name_;
		return true;
	};
	virtual const Obj<IModel> model() = 0;
	virtual void add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src) = 0;
	virtual void add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src) = 0;
	virtual void on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload) = 0;
	virtual void do_event(const DynaArray& msg_name) = 0;
	virtual void activate() {};
	virtual void bind(IBiomolecule* src, const char* filename) {};
	virtual void unbind(IBiomolecule* src) {};
	virtual const char* get_root_path() = 0;
	virtual const char* get_version() = 0;
	virtual void* FindMessageTypeByName(const char* name) { return nullptr; };
	virtual void* FindValueByName(const char* msg_name, int index, const char* name) { return nullptr; };

private:
	IBiomolecule* owner_;
	DynaArray name_;
	DynaArray full_name_;
};

BIO_END_NAMESPACE