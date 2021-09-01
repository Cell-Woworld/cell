#pragma once
#include "IBiomolecule.h"
#include <windows.h>
#include <random>

BIO_BEGIN_NAMESPACE

class IModel;
class mtDNA;
class Chromosome;
class CellAggregation : public IBiomolecule
{
public:
	CellAggregation(const char* uuid, IBiomolecule* owner) : IBiomolecule(owner) {
		aggregation_list_[uuid] = owner;
	};
	virtual ~CellAggregation() {};

public:
	virtual const Obj<IModel> model() { return nullptr; };
	virtual void activate() {};
	virtual void add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src);
	virtual void add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src) {};
	virtual void on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload);
	virtual void do_event(const DynaArray& msg_name);
	virtual const char* get_root_path() { return ""; };
	virtual const char* get_version() { return ""; };

public:
	bool empty() { return aggregation_list_.empty(); };
	bool has(const DynaArray& uuid);

private:
	Map<DynaArray, IBiomolecule*> aggregation_list_;
};

BIO_END_NAMESPACE