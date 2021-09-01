#pragma once
#include "IBiomolecule.h"
#include "ICell.h"
#include <windows.h>
#include <random>

BIO_BEGIN_NAMESPACE

class IModel;
class mtDNA;
class Chromosome;
class Cell : public IBiomolecule
{
	class BioMessage
	{
	public:
		BioMessage(const DynaArray& name_space = "", const DynaArray& title = "", const DynaArray& content = "", IBiomolecule* src = nullptr)
			: name_space_(name_space)
			, title_(title)
			, content_(content)
			, src_molecule_(src) {
		};
		const DynaArray& NameSpace() const { return name_space_; };
		const DynaArray& Title() const { return title_; };
		const DynaArray& Content() const { return content_; };
		IBiomolecule* Source() const { return src_molecule_; };
		void Source(IBiomolecule* src) { src_molecule_ = src; };
		BioMessage& operator =(const BioMessage& _msg) {
			name_space_ = _msg.name_space_;
			title_ = _msg.title_;
			content_ = _msg.content_;
			src_molecule_ = _msg.src_molecule_;
			return *this;
		};
	private:
		DynaArray name_space_;
		DynaArray title_;
		DynaArray content_;
		IBiomolecule* src_molecule_;
	};
	struct CellContainer
	{
		Obj<IBiomolecule> instance;
		bool active;
	};
public:
	Cell(const char* filename, const char* uuid);
	virtual ~Cell();

public:
	virtual const Obj<IModel> model() {
		return model_.first;
	};
	virtual void activate();
	virtual void add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src);
	virtual void add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src);
	virtual void on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload);
	virtual void do_event(const DynaArray& msg_name);
	virtual const char* get_root_path();
	virtual const char* get_version();

private:
	void InvokemtDNA();
	void WaitApoptosis();
	void ProcessEvent(const BioMessage& biomsg);
	template<typename T>
	const char* GetType(const T& value);
	template<typename T>
	void Deserialize(const char* type, const DynaArray& data, T& value);
	template<typename T>
	void Read(const String& name, T& value);
	template<typename T>
	void Write(const String& name, const T& value);
	void SaveVersion();
	void DeaggregateAll();
	void Aggregate(const String& uuid);
	bool isAggregation(const Obj<IBiomolecule>& cell) { return cell != nullptr && cell->get_version() == ""; };
	void Snapshot(const String& target);
	void RevertSnapshot(const String& source);
	void RemoveSourceFromMsgQueue(const Chromosome* chromosome);

private:
	String filename_;
	String uuid_;
	Obj<mtDNA> mtDNA_;
	Pair<Obj<IModel>, HINSTANCE> model_;
	Array<Pair<Obj<Chromosome>, HINSTANCE>> chromosome_list_;
	DEQueue<BioMessage> message_queue_;
	Queue<BioMessage> internal_queue_;
	Mutex message_queue_mutex_;
	Cond_Var wait_queue_empty_;
	bool destroying_;
	static bool evolution_;
	static Mutex apoptosis_mutex_;
	static Cond_Var wait_apoptosis_;
	static Map<String, CellContainer> division_list_;
	static Mutex division_list_mutex_;
	static Obj<std::mt19937> generator_;
};

BIO_END_NAMESPACE