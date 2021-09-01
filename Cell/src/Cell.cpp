#include "Cell.h"
#include "CellAggregation.h"
#include "../Model/src/Model.h"
#include "internal/mtDNA.h"
#include "internal/Chromosome.h"
#include "internal/utils/serializer.h"
#include "compile_time.h"
#include "../proto/Cell.pb.h"
#include "uuid.h"

#define TAG "Cell"

#ifndef VERSION_MAJOR
#define VERSION_MAJOR 0
#endif

#ifndef VERSION_MINOR
#define VERSION_MINOR 0
#endif

#ifndef VERSION_BUILD
#define VERSION_BUILD 1
#endif

#ifndef VERSION_REVISION
#define VERSION_REVISION ""
#endif

#ifdef _WIN32
#include <windows.h>
#define DLL_EXTNAME ".dll"

#elif defined __linux__
#include <dlfcn.h>
#define LoadLibrary(a) dlopen(a, RTLD_NOW)
#define FreeLibrary(a) dlclose(a)
#define GetProcAddress(a, b) dlsym(a, b)
#define DLL_EXTNAME ".so"

#elif defined __APPLE__
#include <dlfcn.h>
#include <CoreFoundation/CFBundle.h>
#define LoadLibrary(a)	dlopen(a, RTLD_NOW)
#define FreeLibrary(a)	dlclose(a)
#define GetProcAddress(a,b)	dlsym(a,b)
#define HINSTANCE void*
#define DLL_EXTNAME		".dylib"
#endif

USING_BIO_NAMESPACE
using Cell = BioSys::Cell;

#ifdef __cplusplus
#ifdef STATIC_API
extern "C" PUBLIC_API IBiomolecule* Cell_CreateInstance(const char* filename)
{
	return new Cell(filename, nullptr);
}
#else
extern "C" PUBLIC_API IBiomolecule* CreateInstance(const char* filename)
{
	return new Cell(filename, nullptr);
}
#endif
#else
#endif

Map<String, Cell::CellContainer> Cell::division_list_;
Mutex Cell::division_list_mutex_;
Cond_Var Cell::wait_apoptosis_;
Mutex Cell::apoptosis_mutex_;
Obj<std::mt19937> Cell::generator_ = nullptr;
bool Cell::evolution_ = false;
static const int SHARED_LEVEL = 1;

Cell::Cell(const char* filename, const char* uuid)
	: IBiomolecule(nullptr)
	, destroying_(false)
	, mtDNA_(new mtDNA())
{
	filename_ = filename==nullptr?"":filename;

	if (generator_ == nullptr)
	{
		std::random_device rd;
		auto seed_data = std::array<int, std::mt19937::state_size> {};
		std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
		std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
		generator_ = Obj<std::mt19937>(new std::mt19937(seq));
	}
	bool _stem_cell = false;
	if (uuid == nullptr)
	{
		uuid_ = uuids::to_string(uuids::uuid_random_generator{ *generator_ }());
		_stem_cell = true;
	}
	else
		uuid_ = uuid;

#ifdef STATIC_API
#ifdef __APPLE__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef resourcesURL = CFBundleCopyBundleURL(mainBundle);
	CFStringRef _str = CFURLCopyFileSystemPath(resourcesURL, kCFURLPOSIXPathStyle);
	CFRelease(resourcesURL);
	char _bundle_path[PATH_MAX];
	CFStringGetCString(_str, _bundle_path, FILENAME_MAX, kCFStringEncodingASCII);
	CFRelease(_str);
	//printf("Bundle path=%s\n", _bundle_path);
	filename_ = String(_bundle_path) + "/" + filename_;
#endif
	model_ = std::make_pair(Obj<IModel>(new Model()), (HINSTANCE)NULL);
	Write("Bio.Cell.uuid", uuid_);
	SaveVersion();

	chromosome_list_.emplace_back(std::make_pair(Obj<Chromosome>(new Chromosome(this)), (HINSTANCE)NULL));
	Obj<Chromosome>& _chromosome = chromosome_list_.back().first;
	if (_stem_cell)
	{
		Bio::Chromosome::Init _init;
		_init.set_filename(filename_);
		add_event(Bio::Chromosome::Init::descriptor()->full_name(), _init.SerializeAsString(), _chromosome.get());
	}
	else
	{
		_chromosome->init(filename_.c_str());
	}
#else
	HINSTANCE _instance = NULL;
	{
		CREATE_MODEL_INSTANCE_FUNCTION CreateInstanceF = NULL;
		if ((_instance = LoadLibrary((String("./") + PREFIX + "Model" + DLL_EXTNAME).c_str())) != NULL)
			CreateInstanceF = (CREATE_MODEL_INSTANCE_FUNCTION)GetProcAddress(_instance, CREATE_INSTANCE);
		if (CreateInstanceF)
		{
			model_ = std::make_pair(Obj<IModel>(CreateInstanceF()), _instance);
			Write("Bio.Cell.uuid", uuid_);
		}
	}
	SaveVersion();
	{
		CREATE_CHROMOSOME_INSTANCE_FUNCTION CreateInstanceF = NULL;
		if ((_instance = LoadLibrary((String("./") + PREFIX + "Chromosome" + DLL_EXTNAME).c_str())) != NULL)
			CreateInstanceF = (CREATE_CHROMOSOME_INSTANCE_FUNCTION)GetProcAddress(_instance, CREATE_INSTANCE);
		if (CreateInstanceF)
		{
			Obj<Chromosome> _chromosome = Obj<Chromosome>(CreateInstanceF(this, filename_.c_str()));
			chromosome_list_.emplace_back(std::make_pair(_chromosome, _instance));
			if (_stem_cell)
			{
				Bio::Chromosome::Init _init;
				_init.set_filename(filename_);
				add_event(Bio::Chromosome::Init::descriptor()->full_name(), _init.SerializeAsString(), _chromosome.get());
			}
			else
			{
				_chromosome->init(filename_.c_str());
			}
		}
		else
		{
			FreeLibrary(_instance);
		}
	}
#endif
}

Cell::~Cell() {
	destroying_ = true;
	CondLocker lk(message_queue_mutex_);
	wait_queue_empty_.wait(lk, [this]{ return message_queue_.empty() && internal_queue_.empty(); });

	if (mtDNA_ != nullptr)
		mtDNA_.reset();

	for (auto& _chromosome : chromosome_list_)
	{
		if (_chromosome.first != nullptr)
			_chromosome.first.reset();
		if (_chromosome.second != NULL)
			FreeLibrary(_chromosome.second);
	}

	if (model_.first != nullptr)
		model_.first.reset();
	if (model_.second != NULL)
	{
		FreeLibrary(model_.second);
		model_.second = NULL;
	}

	//MutexLocker _lock(division_list_mutex_);	// recursive mutex
	DeaggregateAll();
	LOG_D(TAG, "~Cell(%p) done", this);
}

void Cell::activate()
{
	BioMessage _biomsg;
	{
		MutexLocker _lock(message_queue_mutex_);
		if (message_queue_.empty())
			return;
		_biomsg = message_queue_.front();
	}

	ProcessEvent(_biomsg);

	bool _internal_loop = false;
	{
		MutexLocker _lock(message_queue_mutex_);
		if (!internal_queue_.empty())
		{
			_biomsg = internal_queue_.front();
			_internal_loop = true;
		}
	}
	while (_internal_loop == true)
	{
		ProcessEvent(_biomsg);
		{
			MutexLocker _lock(message_queue_mutex_);
			internal_queue_.pop();
			if (!internal_queue_.empty())
			{
				_biomsg = internal_queue_.front();
				_internal_loop = true;
			}
			else
			{
				_internal_loop = false;
			}
		}
	}

	{
		MutexLocker _lock(message_queue_mutex_);
		message_queue_.pop_front();
		if (message_queue_.size() > 0)
			InvokemtDNA();
		else
			wait_queue_empty_.notify_all();
	}
}

void Cell::add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src)
{
	MutexLocker _lock(message_queue_mutex_);
	if (destroying_ == true)
		return;

	if (src != nullptr)
	{
		IBiomolecule* _namespace_owner = src;
		for (int i = SHARED_LEVEL; i > 0 && _namespace_owner->owner() != this; i--)
		{
			_namespace_owner = _namespace_owner->owner();
		}
		if (_namespace_owner != nullptr)
		{ 
			message_queue_.emplace_back(BioMessage(_namespace_owner->full_name(), msg_name, payload, src));
			//LOG_D(TAG, "add_event(%s) namespace=%s, source=%s", msg_name.data(), _namespace_owner->full_name().data(), src->full_name().data());
		}
		else
		{
			message_queue_.emplace_back(BioMessage(full_name(), msg_name, payload, src));
			//LOG_D(TAG, "add_event(%s) namespace=%s, source=%s", msg_name.data(), full_name().data(), this->full_name().data());
		}
	}
	else
	{
		message_queue_.emplace_back(BioMessage(full_name(), msg_name, payload, this));
		//LOG_D(TAG, "add_event(%s) namespace=%s, source=%s", msg_name.data(), full_name().data(), this->full_name().data());
	}
	if (message_queue_.size() == 1)
		InvokemtDNA();
}

void Cell::add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src)
{
	MutexLocker _lock(message_queue_mutex_);
	IBiomolecule* _namespace_owner = src;
	for (int i = SHARED_LEVEL; i > 0 && _namespace_owner->owner() != this; i--)
	{
		_namespace_owner = _namespace_owner->owner();
	}
	if (_namespace_owner != nullptr)
	{
		internal_queue_.emplace(BioMessage(_namespace_owner->full_name(), msg_name, payload, src));
	}
	else
	{
		internal_queue_.emplace(BioMessage(full_name(), msg_name, payload, src));
	}
}

void Cell::InvokemtDNA()
{
	assert(mtDNA_!= nullptr);
	mtDNA_->AddTask(Obj<IBiomolecule>(this, [](IBiomolecule*) {}));
}

void Cell::do_event(const DynaArray& msg_name) {
	if (msg_name == "Bio.Cell.WaitApoptosis")
		WaitApoptosis();
};

void Cell::on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload)
{
	assert(false);
}

void Cell::WaitApoptosis()
{
	while (true)
	{
		CondLocker lk(apoptosis_mutex_);
		wait_apoptosis_.wait(lk);
		//std::this_thread::sleep_for(std::chrono::milliseconds(3000));

		MutexLocker _lock(division_list_mutex_);
		for (auto itr = division_list_.begin(); itr != division_list_.end(); )
		{
			if (itr->second.active == false && itr->second.instance != nullptr)
			{
				itr->second.instance.reset();
				itr = division_list_.erase(itr);
			}
			else
			{
				itr++;
			}
		}
		LOG_I(TAG, "Cell.WaitApoptosis(), cell=%p, division count=%zd", this, division_list_.size());
		if (division_list_.empty()) 
		{
			LOG_I(TAG, "Evolution=%d", evolution_);
		}
		//if (evolution_ == true && division_list_.size() == 1)
		if (evolution_ == true && division_list_.empty())
		//if (division_list_.empty())
		{
			break;
		}
	}
};

void Cell::ProcessEvent(const BioMessage& biomsg)
{
	if (biomsg.Title() == "Bio.Chromosome.Final")
	{
		for (auto _chromosome = chromosome_list_.begin(); chromosome_list_.size() > 1 && _chromosome != chromosome_list_.end(); ) //++_chromosome)
		{
			if (biomsg.Source() == _chromosome->first.get())
			{
				//LOG_D(TAG, "ProcessEvent(%s) source=%s", biomsg.Title().data(), biomsg.Source()->full_name().data());
				RemoveSourceFromMsgQueue(_chromosome->first.get());
				if (_chromosome->first != nullptr)
					_chromosome->first.reset();
				if (_chromosome->second != NULL)
					FreeLibrary(_chromosome->second);
				_chromosome = chromosome_list_.erase(_chromosome);
				break;
			}
			else
			{
				++_chromosome;
			}
		}
		//if (chromosome_list_.empty())
		if (chromosome_list_.size() == 1 && biomsg.Source() == chromosome_list_[0].first.get())
		{
			MutexLocker _lock(division_list_mutex_);
			LOG_D(TAG, "division should be finalized: %p", division_list_[uuid_].instance.get());
			division_list_[uuid_].active = false;
			//if (division_list_.empty())
				wait_apoptosis_.notify_all();
		}
	}
	else if (biomsg.Title() == Bio::Cell::RemoveSourceFromMsgQueue::descriptor()->full_name())
	{
		Bio::Cell::RemoveSourceFromMsgQueue _remove_source_from_msgQueue;
		if (_remove_source_from_msgQueue.ParseFromString(biomsg.Content().str()) == true
			&& _remove_source_from_msgQueue.source_chromosome() != 0)
		{
			RemoveSourceFromMsgQueue((Chromosome*)(void*)_remove_source_from_msgQueue.source_chromosome());
		}
	}
	else if (biomsg.Title() == Bio::Cell::Division::descriptor()->full_name())
	{
		Bio::Cell::Division _division;
		if (_division.ParseFromString(biomsg.Content().str()) == true
			&& _division.filename() != "")
		{
			String _root_path = biomsg.Source()->get_root_path();
			filename_ = _root_path + _division.filename();
		}

		uuids::uuid const _uuid = uuids::uuid_random_generator{ *generator_ }();
		String _uuid_string = uuids::to_string(_uuid);
		Write(String("return.") + Bio::Cell::Division::descriptor()->full_name(), _uuid_string);
		MutexLocker _lock(division_list_mutex_);
		division_list_.insert(make_pair(_uuid_string, CellContainer{ std::make_shared<Cell>(filename_.c_str(), _uuid_string.c_str()), true }));
		LOG_D(TAG, "new division: %p", division_list_[_uuid_string].instance.get());
	}
	else if (biomsg.Title() == Bio::Cell::ForwardEvent::descriptor()->full_name())
	{
		Bio::Cell::ForwardEvent _forward_event;
		if (_forward_event.ParseFromString(biomsg.Content().str()) == true)
		{
			MutexLocker _lock(division_list_mutex_);
			if (division_list_.count(_forward_event.uuid()) > 0)
			{
				const Obj<IBiomolecule>& _target = division_list_[_forward_event.uuid()].instance;
				division_list_[_forward_event.uuid()].instance->add_event(_forward_event.name(), _forward_event.payload(), this);
				if (isAggregation(_target) == true && _forward_event.toitself() == true)
				{
					this->add_event(_forward_event.name(), _forward_event.payload(), this);
				}
				if (_forward_event.name() == "Bio.Cell.Destroyed")
				{
					division_list_.erase(_forward_event.uuid());
					LOG_D(TAG, "Bio.Cell.Destroyed(), division count = %zd", division_list_.size());
				}
				if (division_list_.empty() && evolution_ == true)
				{
					add_event("Bio.Cell.Destroyed", "", nullptr);	// start killing myself for evolution
				}
			}
		}
	}
	else if (biomsg.Title() == Bio::Cell::Evolution::descriptor()->full_name())
	{
		MutexLocker _lock(division_list_mutex_);
		evolution_ = true;
		LOG_I(TAG, "Bio.Cell.Evolution(), division count = %zd", division_list_.size());
		if (division_list_.empty())
		{
			//add_event("Bio.Cell.Destroyed", "", nullptr);	// start killing myself for evolution
			wait_apoptosis_.notify_all();
		}
	}
	else if (biomsg.Title() == Bio::Cell::BroadcastEvent::descriptor()->full_name())
	{
		Bio::Cell::BroadcastEvent _broadcast_event;
		if (_broadcast_event.ParseFromString(biomsg.Content().str()) == true)
		{
			MutexLocker _lock(division_list_mutex_);
			for (auto itr = division_list_.begin(); itr != division_list_.end(); )
			{
				if (isAggregation(itr->second.instance))
					continue;
				if (_broadcast_event.toitself() == true || itr->second.instance.get() != this)
					itr->second.instance->add_event(_broadcast_event.name(), _broadcast_event.payload(), nullptr);
				if (_broadcast_event.name() == "Bio.Cell.Destroyed")
				{
					itr = division_list_.erase(itr);
					LOG_I(TAG, "Bio.Cell.Destroyed(), division count = %zd", division_list_.size());
				}
				else
				{ 
					++itr;
				}
				if (division_list_.empty() && evolution_ == true)
				{
					add_event("Bio.Cell.Destroyed", "", nullptr);	// start killing myself for evolution
				}
			}
		}
	}
	else if (biomsg.Title() == Bio::Cell::Aggregation::descriptor()->full_name())
	{
		Bio::Cell::Aggregation _aggeregation;
		if (_aggeregation.ParseFromString(biomsg.Content().str()) == true)
		{
			Aggregate(_aggeregation.uuid());
			LOG_I(TAG, "aggregate %s", _aggeregation.uuid().c_str());
		}
	}
	else if (biomsg.Title() == Bio::Cell::Deaggregation::descriptor()->full_name())
	{
		Bio::Cell::Deaggregation _deaggeregation;
		if (_deaggeregation.ParseFromString(biomsg.Content().str()) == true)
		{
			MutexLocker _lock(division_list_mutex_);
			if (_deaggeregation.uuid() == "*")
			{
				DeaggregateAll();
				LOG_I(TAG, "deaggregate all");
			}
			else if (division_list_.count(_deaggeregation.uuid()) > 0)
			{
				CellAggregation* _cell_aggregation = (CellAggregation*)division_list_[_deaggeregation.uuid()].instance.get();
				_cell_aggregation->add_event(_deaggeregation.GetTypeName(), uuid_, this);
				if (_cell_aggregation->empty())
					division_list_.erase(_deaggeregation.uuid());
				LOG_I(TAG, "deaggregate %s", _deaggeregation.uuid().c_str());
			}
		}
	}
	else if (biomsg.Title() == Bio::Cell::Snapshot::descriptor()->full_name())
	{
		LOG_D(TAG, "Bio.Cell.Snapshot begins");
		Bio::Cell::Snapshot _snapshot;
		if (biomsg.Content().size() > 0 && _snapshot.ParseFromString(biomsg.Content().str()) == true)
		{ 
			Snapshot(Bio::Cell::SessionInfo::descriptor()->full_name() + ".Result");
			for (auto _chromosome : chromosome_list_)
			{
				if (_chromosome.first != nullptr)
					_chromosome.first->on_event(biomsg.NameSpace(), biomsg.Title(), biomsg.Content());
			}
			String _path = _snapshot.path();
			if (_path.back() != '/' && _path.back() != '\\')
			{
				_path += '/';
			}
			model()->Persist(_path, _snapshot.id());
		}
		LOG_D(TAG, "Bio.Cell.Snapshot has finished");
	}
	else if (biomsg.Title() == Bio::Cell::RevertSnapshot::descriptor()->full_name())
	{
		LOG_D(TAG, "Bio.Cell.RevertSnapshot begins");
		Bio::Cell::RevertSnapshot _revert_snapshot;
		bool _restored = false;
		if (biomsg.Content().size() > 0 && _revert_snapshot.ParseFromString(biomsg.Content().str()) == true)
		{
			uint64_t _ws;
			Read("Websocket.Service.Config.id", _ws);
			String _path = _revert_snapshot.path();
			if (_path.back() != '/' && _path.back() != '\\')
			{
				_path += '/';
			}
			_restored = model()->RestorePersistence(_path, _revert_snapshot.id());
			if (_restored)
			{
				model()->RemovePersistence(_path, _revert_snapshot.id());

				Write("Websocket.Service.Config.id", _ws);
				model()->Snapshot();

				for (auto _chromosome : chromosome_list_)
				{
					if (_chromosome.first != nullptr)
						_chromosome.first->on_event(biomsg.NameSpace(), biomsg.Title(), "");
				}
				RevertSnapshot(Bio::Cell::SessionInfo::descriptor()->full_name() + ".Result");

				model()->RevertSnapshot();

				for (auto _chromosome : chromosome_list_)
				{
					if (_chromosome.first != nullptr)
						_chromosome.first->on_event(biomsg.NameSpace(), Bio::Chromosome::RestoreActivity::descriptor()->full_name(), "");
				}

				model()->Remove(Bio::Cell::SessionInfo::descriptor()->full_name() + ".Result");
				model()->Remove(Bio::Chromosome::SessionInfo::descriptor()->full_name() + ".Result");
			}
		}
		Write(Bio::Cell::RevertSnapshot::Result::descriptor()->full_name() + ".restored", _restored);
		LOG_D(TAG, "Bio.Cell.RevertSnapshot has finished");
	}
	else
	{
		Write("Bio.Cell.Current.EventSource", "");
		if (biomsg.Title() != Bio::Cell::Destroyed::descriptor()->full_name())
		{
			for (auto _chromosome : chromosome_list_)
			{
				//if (biomsg.Source() == _chromosome.first.get())
				bool _found = false;
				IBiomolecule* _molecule = biomsg.Source();
				while (!_found && _molecule != nullptr)
				{
					if (_molecule == _chromosome.first.get())
					{
						Write("Bio.Cell.Current.EventSource", String(biomsg.Source()->get_root_path()));
						_found = true;
					}
					else
					{
						_molecule = _molecule->owner();
					}
				}
				if (_found)
					break;
			}
		}
		for (auto _chromosome : chromosome_list_)
		{
			if (_chromosome.first != nullptr)
				_chromosome.first->on_event(biomsg.NameSpace(), biomsg.Title(), biomsg.Content());
		}
	}
}

const char* Cell::get_root_path() 
{ 
	//if (chromosome_list_.size() > 0
	//	&& chromosome_list_[0].first != nullptr)
	//	return chromosome_list_[0].first->get_root_path();
	//else
		return "";
};

template<typename T>
const char* Cell::GetType(const T& value)
{
	return typeid(value).name();
}

template <typename T>
void Cell::Deserialize(const char* type, const DynaArray& data, T& value)
{
	if (type == "")
		throw std::exception();

	const char* _target_type = GetType(value);

	assert(strcmp(type, _target_type) == 0);

	ByteArray _data(data.size());
	memcpy(_data.data(), data.data(), data.size());
	zpp::serializer::memory_input_archive in(_data);
	in(value);
}

template<typename T>
void Cell::Read(const String& name, T& value)
{
	DynaArray _stored_type;
	DynaArray _buf;
	model()->Read(name, _stored_type, _buf);
	try {
		Deserialize(_stored_type.data(), _buf, value);
	}
	catch (...)
	{
		if (name.substr(0, strlen("Bio.Cell.")) == "Bio.Cell.")
		{
			LOG_D(TAG, "No such key in Model, key name: %s", name.c_str());
		}
		else
		{
			LOG_W(TAG, "No such key in Model, key name: %s", name.c_str());
		}
	}
}

template<typename T>
void Cell::Write(const String& name, const T& value)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	out(value);
	model()->Write(name, GetType(value), data);
}

void Cell::SaveVersion()
{
	Write("Bio.Cell.version.Cell", (String)get_version());
	Write("Bio.Cell.version.major.Cell", VERSION_MAJOR);
	Write("Bio.Cell.version.minor.Cell", VERSION_MINOR);
	Write("Bio.Cell.version.build.Cell", VERSION_BUILD);
	Write("Bio.Cell.version.revision.Cell", (String)VERSION_REVISION);
	const time_t TIME_DIFFERENCE = -8 * 60 * 60;
	Write("Bio.Cell.version.time.Cell", (time_t)__TIME_UNIX__ + TIME_DIFFERENCE);
}

const char* Cell::get_version() 
{ 
	static String _version = std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_BUILD) + "." + VERSION_REVISION;
	return _version.c_str();
}

void Cell::DeaggregateAll()
{
	for (auto itr = division_list_.begin(); itr != division_list_.end(); )
	{
		if (isAggregation(itr->second.instance))
		{	// typeof CellAggregation
			CellAggregation* _cell_aggregation = (CellAggregation*)itr->second.instance.get();
			_cell_aggregation->add_event(Bio::Cell::Deaggregation::descriptor()->full_name(), uuid_, this);
			if (_cell_aggregation->empty())
				itr = division_list_.erase(itr);
			else
				++itr;
		}
		else
			++itr;
	}
}

void Cell::Aggregate(const String& uuid)
{
	MutexLocker _lock(division_list_mutex_);
	if (division_list_.count(uuid) == 0)
	{
		division_list_.insert(make_pair(uuid, CellContainer{ std::make_shared<CellAggregation>(uuid_.c_str(), this), true }));
	}
	else
	{
		division_list_[uuid].instance->add_event(Bio::Cell::Aggregation::descriptor()->full_name(), uuid_, this);
	}
}

void Cell::Snapshot(const String& target)
{
	Bio::Cell::SessionInfo _sessioninfo;
	MutexLocker _lock(division_list_mutex_);
	for (auto itr = division_list_.begin(); itr != division_list_.end(); ++itr)
	{
		if (isAggregation(itr->second.instance))
		{	// typeof CellAggregation
			CellAggregation* _cell_aggregation = (CellAggregation*)itr->second.instance.get();
			if (_cell_aggregation->has(uuid_))
			{
				_sessioninfo.add_aggregation(itr->first);
			}
		}
	}
	if (_sessioninfo.aggregation_size() > 0)
	{
		Write("Bio.Cell.Snapshot", _sessioninfo.SerializeAsString());
	}
}

void Cell::RevertSnapshot(const String& source)
{
	String _sessioninfo_serilized;
	Bio::Cell::SessionInfo _sessioninfo;
	Read("Bio.Cell.Snapshot", _sessioninfo_serilized);
	if (_sessioninfo.ParseFromString(_sessioninfo_serilized) == true && _sessioninfo.aggregation_size() > 0)
	{
		//MutexLocker _lock(division_list_mutex_);	// multiple lock
		for (int i = 0; i < _sessioninfo.aggregation_size(); i++)
		{
			Aggregate(_sessioninfo.aggregation(i));
		}
	}
}

void Cell::RemoveSourceFromMsgQueue(const Chromosome* chromosome)
{
	MutexLocker _lock(message_queue_mutex_);
	if (!message_queue_.empty())
	{
		for (auto& item : message_queue_)
		{
			IBiomolecule* _molecule = item.Source();
			//while (_molecule != nullptr)
			{
				if (_molecule == chromosome)
				{
					item.Source(nullptr);
					//break;
				}
				//else
				//{
				//	_molecule = _molecule->owner();
				//}
			}
		}
	}
}