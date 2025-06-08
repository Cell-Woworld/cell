#pragma once
#include "IBiomolecule.h"
#include "../proto/Cell.pb.h"
#include <google/protobuf/descriptor.h>
#include <windows.h>
#include "nlohmann/json.hpp"
#include <regex>

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
	virtual void bind(IBiomolecule* src, const char* filename);
	virtual void unbind(IBiomolecule* src);
	virtual bool init(const char* filename, const char* content = nullptr);
	virtual const char* get_root_path() { return root_path_.data(); };
	virtual const char* get_version();
	virtual void* FindMessageTypeByName(const char* name);

public:
	virtual bool IdentifySource(const IBiomolecule* chromosome, IBiomolecule* namespace_owner = nullptr);

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
	Map<size_t, int> type_hash_table_;
	std::hash<String> hash_;
	//Array<Obj<google::protobuf::DescriptorPool>> extra_desc_pool_;
private:
	static bool discard_no_such_model_warning_;

private:
	void SplitPathName(const String& full_filename, String& path, String& filename);
	template<typename T>
	const char* GetType(const T& value);
	template<typename T>
	void Deserialize(const char* type, const DynaArray& data, T& value);
	template<typename T>
	void Read(const String& name, T& value, bool auto_conversion = true);
	void Read(const String& name, bool& value, bool auto_conversion = true);
	void Read(const String& name, nlohmann::json& value, bool auto_conversion = true);
	template<typename T>
	void Write(const String& name, const T& value);
	void Write(const String& name, const nlohmann::json& value);
	void Remove(const String& name);
	void SaveVersion();
	time_t epoch();
	void Snapshot(Bio::Chromosome::SessionInfo& session_info);
	void RevertSnapshot(const Bio::Chromosome::SessionInfo& session_info);
	void RestoreActivity(const Bio::Chromosome::SessionInfo& session_info);
	void AddProto(const String& filename);
	void RemoveSourceFromMsgQueue(const IBiomolecule* chromosome);
	void BuildTypeHastable();

	template<typename T1, typename T2>
	void Deserialize(T1, const DynaArray& data, T2& value);
	template <typename T2>
	void Deserialize(bool, const DynaArray& data, T2& value);
	template <typename T2>
	void Deserialize(Array<bool>, const DynaArray& data, T2& value);
	template <typename T2>
	void Deserialize(nlohmann::json, const DynaArray& data, T2& value);
	void Deserialize(const char* type, const DynaArray& data, bool& value);

	template<typename T1, typename T2>
	void TypeConversion(const T1& src, Array<T2>& dest);
	template<typename T>
	void TypeConversion(const String& src, T& dest);
	template<typename T>
	void TypeConversion(const String& src, Array<T>& dest);
	template<typename T>
	void TypeConversion(const T& src, Array<String>& dest);
	template<typename T1, typename T2> void TypeConversion(const T1& src, T2& dest);
	template<typename T> void TypeConversion(const T& src, String& dest);
	void TypeConversion(const bool& src, String& dest);
	void TypeConversion(const String& src, String& dest);
	template<typename T1, typename T2> void TypeConversion(const Array<T1>& src, T2& dest);
	template<typename T> void TypeConversion(const Array<T>& src, String& dest);
	template<typename T> void TypeConversion(const Array<String>& src, T& dest);
	void TypeConversion(const Array<String>& src, String& dest);
	void TypeConversion(const String& src, Array<String>& dest);
	void TypeConversion(const Array<bool>& src, String& dest);
	template<typename T1, typename T2> void TypeConversion(const Array<T1>& src, Array<T2>& dest);
	template<typename T> void TypeConversion(const Array<T>& src, Array<String>& dest);
	template<typename T> void TypeConversion(const Array<String>& src, Array<T>& dest);
	void TypeConversion(const Array<String>& src, Array<String>& dest);
	void TypeConversion(const double& src, String& dest);
	void TypeConversion(const double& src, Array<String>& dest);

	template <typename T>
	String to_string_with_precision(const T a_value, const int n = 6);
	bool is_number(const String& token) {
		return std::regex_match(token, std::regex(("((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?")));
	}
};

typedef Chromosome* (*CREATE_CHROMOSOME_INSTANCE_FUNCTION)(IBiomolecule*, const char*);

BIO_END_NAMESPACE
