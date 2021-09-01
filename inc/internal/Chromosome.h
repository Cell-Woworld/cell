#pragma once
#include "IBiomolecule.h"
#include "../proto/Cell.pb.h"
#include <google/protobuf/descriptor.h>
#include <windows.h>

BIO_BEGIN_NAMESPACE

class mRNA;
class RNA;
class DNA;

class Chromosome : public IBiomolecule
{
public:
	PUBLIC_API Chromosome(IBiomolecule* owner); //, const char* filename);
	PUBLIC_API virtual ~Chromosome();

public:
	virtual const Obj<IModel> model() {
		return owner()->model();
	};
	virtual void activate() {
		//return owner_->activate();
		assert(false);
	};
	virtual void add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src);
	virtual void add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src);
	virtual void on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload);
	virtual void do_event(const DynaArray& msg_name);
	virtual void bind(IBiomolecule* src, void* desc_pool);
	virtual void unbind(IBiomolecule* src);
	virtual void init(const char* filename);
	virtual const char* get_root_path() { return root_path_.data(); };
	virtual const char* get_version();

private:
	struct ChildInfo
	{
		Obj<Chromosome> obj_;
		String gene_;
		bool autoforward_;
	};
	struct HandlerInfo
	{
		Obj<RNA> obj_;
		HINSTANCE instance_;
		String name_;
	};
	struct ActivityInfo
	{
		String namespace_;
		String name_;
		String payload_;
	};
	Pair<Obj<mRNA>, HINSTANCE> mRNA_;
	Pair<Obj<DNA>, HINSTANCE> DNA_;
	Array<HandlerInfo> handlers_;
	Map<String, ChildInfo> children_;
	Map<String, ActivityInfo> activities_;
	DynaArray root_path_;
	Array<Obj<google::protobuf::DescriptorPool>> extra_desc_pool_;

private:
	void SplitPathName(const String& full_filename, String& path, String& filename);
	template<typename T>
	const char* GetType(const T& value);
	template<typename T>
	void Deserialize(const char* type, const DynaArray& data, T& value);
	template<typename T>
	void Read(const String& name, T& value);
	template<typename T>
	void Write(const String& name, const T& value);
	void Remove(const String& name);
	void SaveVersion();
	time_t epoch();
	void Snapshot(Bio::Chromosome::SessionInfo& session_info);
	void RevertSnapshot(const Bio::Chromosome::SessionInfo& session_info);
	void RestoreActivity(const Bio::Chromosome::SessionInfo& session_info);
	void AddProto(const String& filename);
	void RemoveSourceFromMsgQueue(const Chromosome* chromosome);
};

typedef Chromosome* (*CREATE_CHROMOSOME_INSTANCE_FUNCTION)(IBiomolecule*, const char*);

BIO_END_NAMESPACE
