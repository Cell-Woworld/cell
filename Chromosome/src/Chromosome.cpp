#include "internal/Chromosome.h"
#include "internal/IModel.h"
#include "internal/mRNA.h"
#include "internal/DNA.h"
#include "internal/utils/serializer.h"
#include "internal/utils/cell_utils.h"
#include "compile_time.h"
#include <google/protobuf/compiler/importer.h>
#include <ctime>
#include "uuid.h"
#include <filesystem>
#include <typeinfo>
#include <float.h>

#define TAG "Chromosome"

#ifdef _WIN32
#include <windows.h>
#define DLL_EXTNAME ".dll"
#include <direct.h>
#define getcwd _getcwd

#elif defined __linux__
#include <dlfcn.h>
#define LoadLibrary(a) dlopen(a, RTLD_NOW)
#define FreeLibrary(a) dlclose(a)
#define GetProcAddress(a, b) dlsym(a, b)
#define DLL_EXTNAME ".so"
#include <unistd.h>

#elif defined __APPLE__
#include <dlfcn.h>
#define LoadLibrary(a)	dlopen(a, RTLD_NOW)
#define FreeLibrary(a)	dlclose(a)
#define GetProcAddress(a,b)	dlsym(a,b)
#define HINSTANCE void*
#define DLL_EXTNAME		".dylib"
#include <unistd.h>
#endif

USING_BIO_NAMESPACE
using Chromosome = BioSys::Chromosome;

#ifdef __cplusplus
#ifdef STATIC_API
extern "C" PUBLIC_API RNA * mRNA_CreateInstance(IBiomolecule * owner);
extern "C" PUBLIC_API IBiomolecule * DNA_CreateInstance(IBiomolecule * owner);
extern "C" PUBLIC_API RNA * RNA_CreateInstance(IBiomolecule * owner, const char* name);

extern "C" PUBLIC_API IBiomolecule * Chromosome_CreateInstance(IBiomolecule * owner)	//, const char* filename)
{
	return new Chromosome(owner);	//, filename);
}
#else
extern "C" PUBLIC_API IBiomolecule * CreateInstance(IBiomolecule * owner)	//, const char* filename)
{
	return new Chromosome(owner);	// , filename);
}
#endif

#else
#endif

enum type_id_table_
{
	type_bool,
	type_int32_t,
	type_uint32_t,
	type_longlong_t,
	type_ulonglong_t,
	type_double,
	type_string,

	type_bool_array,
	type_int32_t_array,
	type_uint32_t_array,
	type_longlong_t_array,
	type_ulonglong_t_array,
	type_double_array,
	type_string_array,

	type_json,

	MAX_TYPE_COUNT,
};

bool Chromosome::discard_no_such_model_warning_ = false;

Chromosome::Chromosome(IBiomolecule* owner)	//, const char* filename)
	: IBiomolecule(owner)
	, mRNA_({ nullptr, NULL })
	, DNA_({ nullptr, NULL })
{
	assert(owner != nullptr);
	BuildTypeHastable();
}

Chromosome::~Chromosome()
{
	LOG_D(TAG, "doing Chromosome::~Chromosome(%p, %s)", this, full_name().data());

	for (auto& _child : children_)
	{
		if (_child.second.obj_ != nullptr)
			_child.second.obj_.reset();
	}

	for (auto& handler_ : handlers_)
	{
		if (handler_.obj_ != nullptr)
			handler_.obj_.reset();
		if (handler_.instance_ != NULL)
			FreeLibrary(handler_.instance_);
	}

	//if (extra_desc_pool_.size() > 0)
	{
		unbind(this);
	}

	if (DNA_.first != nullptr)
		DNA_.first.reset();
	if (DNA_.second != NULL)
		FreeLibrary(DNA_.second);

	if (mRNA_.first != nullptr)
		mRNA_.first.reset();
	if (mRNA_.second != NULL)
		FreeLibrary(mRNA_.second);
	LOG_D(TAG, "Chromosome::~Chromosome(%s) done", full_name().data());
}

bool Chromosome::init(const char* filename, const char* content)
{
	namespace fs = std::filesystem;
	if (filename != nullptr && strcmp(filename, "*") == 0)
	{
		return IBiomolecule::init(filename);
	}

	String _name, _root_path;
	fs::path _filepath(filename);
	SplitPathName(fs::absolute(_filepath).string(), _root_path, _name);
	root_path_ = _root_path;
	bool _retval = IBiomolecule::init(_name.c_str());

	SaveVersion();
	Read("Bio.Cell.Model.DiscardNoSuchModelWarning", discard_no_such_model_warning_);

	{
#ifdef STATIC_API
		mRNA_.first = Obj<mRNA>((mRNA*)mRNA_CreateInstance(this));
#else
		HINSTANCE _instance = NULL;
		CREATE_RNA_INSTANCE_FUNCTION CreateInstanceF = NULL;
		if (_retval && (_instance = LoadLibrary((String("./") + PREFIX + "mRNA" + DLL_EXTNAME).c_str())) != NULL)
		{
			mRNA_.second = _instance;

			CreateInstanceF = (CREATE_RNA_INSTANCE_FUNCTION)GetProcAddress(_instance, CREATE_INSTANCE);
			if (CreateInstanceF)
				mRNA_.first = Obj<mRNA>((mRNA*)CreateInstanceF(this));
			else
			{
				LOG_W(TAG, "Chromosome::init(%s) fail to creating instance of mRNA", filename);
				_retval = false;
			}
		}
		else
		{
			LOG_W(TAG, "Chromosome::init(%s) fail to load mRNA", filename);
			_retval = false;
		}
#endif
		// for Bio.Cell.*
		bind(this, nullptr);
	}
	{
#ifdef STATIC_API
		DNA_.first = Obj<DNA>((DNA*)DNA_CreateInstance(this));
		if (_retval && DNA_.first != nullptr)
			_retval = DNA_.first->init(filename);
#else
		HINSTANCE _instance = NULL;
		CREATE_DNA_INSTANCE_FUNCTION CreateInstanceF = NULL;
		if (_retval && (_instance = LoadLibrary((String("./") + PREFIX + "DNA" + DLL_EXTNAME).c_str())) != NULL)
		{
			DNA_.second = _instance;

			CreateInstanceF = (CREATE_DNA_INSTANCE_FUNCTION)GetProcAddress(_instance, CREATE_INSTANCE);
			if (CreateInstanceF)
				DNA_.first = Obj<DNA>((DNA*)CreateInstanceF(this));
			if (DNA_.first != nullptr)
			{
				if (content == nullptr)
					_retval = DNA_.first->init(filename);
				else
					_retval = DNA_.first->init(content, false);
			}
		}
#endif
	}
	//if (!internal_queue_.empty())
	//{
	//	add_event("", "", nullptr);
	//}
	return _retval;
}

void Chromosome::bind(IBiomolecule* src, const char* filename)
{
	mRNA_.first->bind(src, filename);
	owner()->bind(src, filename);
};

void Chromosome::unbind(IBiomolecule* src)
{
	if (mRNA_.first != nullptr)
		mRNA_.first->unbind(src);
	owner()->unbind(src);
};

void Chromosome::add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src)
{
	if (src != nullptr)
	{
		if (src == DNA_.first.get())
		{
			DynaArray _payload = "";
			IBiomolecule* _packer = this;
			while (_packer->full_name() != "")
			{
				if (((Chromosome*)_packer)->mRNA_.first->pack(root_path_, msg_name, _payload) == true)
				{
					owner()->add_priority_event(msg_name, _payload, this);
					break;
				}
				else
				{
					_packer = _packer->owner();
				}
			}
			if (_packer->full_name() == "")
			{
				LOG_D(TAG, "RAISE: pack(%s) not found", msg_name.data());
				owner()->add_priority_event(msg_name, "", this);
			}
		}
		else
		{
			owner()->add_event(msg_name, payload, src);				// just forwarding to Cell
		}
	}
	else
	{
		if (payload != "")			// just forwarding to Cell
			owner()->add_event(msg_name, payload, this);
		else
		{
			DynaArray _payload = "";
			IBiomolecule* _packer = this;
			while (_packer->full_name() != "")
			{
				if (((Chromosome*)_packer)->mRNA_.first->pack(root_path_, msg_name, _payload) == true)
				{
					owner()->add_event(msg_name, _payload, this);
					break;
				}
				else
				{
					_packer = _packer->owner();
				}
			}
			if (_packer->full_name() == "")
			{
				LOG_D(TAG, "SEND: pack(%s) not found", msg_name.data());
				owner()->add_event(msg_name, "", this);
			}
		}
	}
};

void Chromosome::add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src)
{
	IBiomolecule* _src = src;
	if (_src == nullptr)
		_src = this;
	else
	{
		if (IdentifySource(_src) == false)
		{
			LOG_W(TAG, "Chromosome::add_priority_event(%s) unrecognized source!", msg_name.data());
			//assert(false);
			// return;		// source is invalid but don't ignore it. if ignore it, it will cause NextCall() of MySQL stop
			_src = this;
		}
	}
	if (payload != "")			// just forwarding to Cell
		owner()->add_priority_event(msg_name, payload, _src);
	else
	{
		DynaArray _payload = "";
		IBiomolecule* _packer = this;
		while (_packer->full_name() != "")
		{
			if (((Chromosome*)_packer)->mRNA_.first->pack(root_path_, msg_name, _payload) == true)
			{
				owner()->add_priority_event(msg_name, _payload, _src);
				break;
			}
			else
			{
				_packer = _packer->owner();
			}
		}
		if (_packer->full_name() == "")
		{
			LOG_D(TAG, "RAISE: pack(%s) not found", msg_name.data());
			owner()->add_priority_event(msg_name, "", _src);
		}
	}
}

void Chromosome::on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload)
{
	// TODO: ' msg_name == "" ' is not good enough. It is just for the reason of triggering the internal message queue at initial time.
	if (name() == "*" || msg_name == "" || name_space == "" || name_space.str().find(full_name().data()) != String::npos)
	{
		switch (hash(msg_name.str()))
		{
		case "Bio.Chromosome.Init"_hash:
		{
			Bio::Chromosome::Init _init;
			if (_init.ParseFromString(payload.str()) == true)
			{
				Write(msg_name.str() + ".filename", _init.filename());
				do_event(msg_name);
			}
			break;
		}
		case "Bio.Cell.Snapshot"_hash:
		case "Bio.Cell.RevertSnapshot"_hash:
		case "Bio.Chromosome.RestoreActivity"_hash:
		case "Bio.Chromosome.AddProto"_hash:
		{
			do_event(msg_name);
			break;
		}
		case "Bio.Chromosome.RegisterActivity"_hash:
		{
			if (mRNA_.first->unpack(msg_name, payload) == true)
			{
				Write(msg_name.str() + ".namespace", name_space.str());
				do_event(msg_name);
			}
			break;
		}
		case "Bio.Chromosome.UnregisterActivity"_hash:
		{
			if (mRNA_.first->unpack(msg_name, payload) == true)
			{
				do_event(msg_name);
			}
			break;
		}
		default:
			if (msg_name != "" && mRNA_.first->unpack(msg_name, payload) == true)
			{
				//LOG_D(TAG, "%s gets message: %s", full_name().data(), msg_name.data());
				if (name_space == full_name())
				{
					//bool _destroying = (msg_name == "Bio.Cell.Destroyed");
					//if (!_destroying)
					//	DNA_.first->on_event("", msg_name, payload);
					//for (auto _child : children_)
					//{
					//	if (_child.second.autoforward_ == true)
					//		_child.second.obj_->on_event(_child.second.obj_->full_name(), msg_name, "");
					//}
					//if (_destroying)
					//	DNA_.first->on_event("", msg_name, payload);

					unsigned long long _src = 0;
					Read((String)"src_instance." + msg_name.str(), _src);
					DNA_.first->on_event("", msg_name, payload);
					for (auto& _child : children_)
					{
						if (_child.second.autoforward_ == true || _child.second.obj_.get() == (Chromosome*)_src)
							_child.second.obj_->on_event(_child.second.obj_->full_name(), msg_name, DynaArray(ByteArray(1, '\0')));
						else
						{
							LOG_D(TAG, "message (%s) of (namespace:%s, source:%s) will not be handled in this namespace (%s)\n", msg_name.data(), name_space.data(), ((Chromosome*)_src)->full_name().data(), _child.second.obj_->full_name().data());
						}
					}
					// don't clear parameters of a message, it may not be used in this process but the next few process
					//if (payload != "")
					//	Remove(msg_name.str() + (String)".*");
				}
				else
				{
					DNA_.first->on_event("", msg_name, payload);
					for (auto& _child : children_)
					{
						if (_child.second.autoforward_ == true)
							_child.second.obj_->on_event(name_space, msg_name, DynaArray(ByteArray(1, '\0')));
					}
				}
			}
			break;
		}
	}
	else
	{
		LOG_D(TAG, "message (%s) of namespace(%s) will not be handled in this namespace (%s)\n", msg_name.data(), name_space.data(), full_name().data());
	}
}

void Chromosome::do_event(const DynaArray& msg_name)
{
	switch (hash(msg_name.str()))
	{
	case "Bio.Chromosome.Init"_hash:
	{
		String _filename;
		Read(msg_name.str() + ".filename", _filename);
		init(_filename.c_str());
		break;
	}
	case "Bio.Cell.NewRNA"_hash:
	{
		String _name;
		Read(msg_name.str() + ".name", _name);
#ifdef STATIC_API
		Obj<RNA> _new_rna(RNA_CreateInstance(this, _name.c_str()));
		handlers_.push_back({ _new_rna, (HINSTANCE)NULL, _name });
		Write("return.Bio.Cell.NewRNA", (_new_rna != nullptr));

		size_t _pos = 0;
		if ((_pos = _name.find_last_of("/\\")) == String::npos)
			_pos = 0;
		else
			_pos++;
		int _major_version = 0;
		Read((String)"Bio.Cell.version.major." + _name.substr(_pos), _major_version);
		const time_t ONE_MONTH = 30 * 24 * 60 * 60;
		time_t _compile_time = 0;
		Read((String)"Bio.Cell.version.time." + _name.substr(_pos), _compile_time);
		if (_major_version < 1 && (_compile_time < epoch() - ONE_MONTH || epoch() < _compile_time))
			_new_rna = nullptr;
#else
		String _rel_path, _filename;
		SplitPathName(_name, _rel_path, _filename);

		HINSTANCE _instance = NULL;
		CREATE_RNA_INSTANCE_FUNCTION CreateInstanceF = NULL;
		String _full_name = root_path_.str() + _rel_path + PREFIX + _filename + DLL_EXTNAME;
		if ((_instance = LoadLibrary(_full_name.c_str())) == NULL)
		{
			char _working_folder[MAX_PATH] = { '\0' };
			getcwd(_working_folder, MAX_PATH);
			String _module_name = String(_working_folder) + "\\" + PREFIX + _filename + DLL_EXTNAME;
			if ((_instance = LoadLibrary(_module_name.c_str())) == NULL)
			{
				String _module_name = String("./") + PREFIX + _filename + DLL_EXTNAME;
				if ((_instance = LoadLibrary(_module_name.c_str())) == NULL)
				{
					printf("ERROR! When loading %s\n", _full_name.c_str());
					Write("return.Bio.Cell.NewRNA", false);
					return;
				}
			}
		}

		CreateInstanceF = (CREATE_RNA_INSTANCE_FUNCTION)GetProcAddress(_instance, CREATE_INSTANCE);
		if (CreateInstanceF)
		{
			LOG_D(TAG, "Bio.Cell.NewRNA(%s) objInstance(%X).CreateInstance(%X)", _name.c_str(), (unsigned int)(unsigned long long)_instance, (unsigned int)(unsigned long long)CreateInstanceF);
			Obj<RNA> _new_rna(CreateInstanceF(this));
			Write("return.Bio.Cell.NewRNA", (_new_rna != nullptr));

			size_t _pos = 0;
			if ((_pos = _name.find_last_of("/\\")) == String::npos)
				_pos = 0;
			else
				_pos++;
			int _major_version = 0;
			Read((String)"Bio.Cell.version.major." + _name.substr(_pos), _major_version);
			const time_t ONE_MONTH = 30 * 24 * 60 * 60;
			time_t _compile_time = 0;
			Read((String)"Bio.Cell.version.time." + _name.substr(_pos), _compile_time);
			if (_major_version < 1 && (_compile_time < epoch() - ONE_MONTH || epoch() < _compile_time))
				_new_rna = nullptr;

			//if (_new_rna != nullptr)
			handlers_.push_back({ _new_rna, _instance, _name });
		}
		else
		{
			handlers_.push_back({ nullptr, _instance, _name });
			Write("return.Bio.Cell.NewRNA", false);
		}
#endif
		break;
	}
	case "Bio.Chromosome.New"_hash:
	{
		namespace fs = std::filesystem;
		String _filename;
		Read(msg_name.str() + ".src", _filename);
		String _srcexpr, _expr;
		Read(msg_name.str() + ".srcexpr", _srcexpr);
		if (!_srcexpr.empty())
			Read(_srcexpr, _expr);
		String _id;
		Read(msg_name.str() + ".id", _id);
		String _auto_forward;
		Read(msg_name.str() + ".autoforward", _auto_forward);
		bool _ignore_error = false;
		Read(msg_name.str() + ".ignore_error", _ignore_error);
		bool _retval = false;
		if (children_.count(_id) == 0)
		{
			if (_filename != "" && _expr != "")
			{
				Obj<Chromosome> _obj(new Chromosome(this));
				children_[_id] = { _obj, _filename, (_auto_forward == "true") ? true : false };
				_retval = _obj->init((root_path_.str() + _filename).c_str(), _expr.c_str());
			}
			else if (_filename != "")
			{
				if (fs::exists(root_path_.str() + _filename))
				{
					Obj<Chromosome> _obj(new Chromosome(this));
					children_[_id] = { _obj, _filename, (_auto_forward == "true") ? true : false };
					_retval = _obj->init((root_path_.str() + _filename).c_str());
				}
				else if (_ignore_error == false || fs::exists(_filename))
				{
					Obj<Chromosome> _obj(new Chromosome(this));
					children_[_id] = { _obj, _filename, (_auto_forward == "true") ? true : false };
					_retval = _obj->init(_filename.c_str());
				}
			}
		}
		else
		{
			_retval = true;
			LOG_W(TAG, "component [%s,%s] exists", _id.c_str(), _filename.c_str());
		}
		Write("return.invoke." + _id, _retval);
		break;
	}
	case "Bio.Chromosome.Remove"_hash:
	{
		String _id;
		Read(msg_name.str() + ".id", _id);
		if (children_.count(_id) > 0)
		{
			LOG_D(TAG, "doing Bio.Chromosome.Remove(%s): %p", _id.c_str(), children_[_id].obj_.get());
			IBiomolecule* _removed_molecule = children_[_id].obj_.get();
			children_[_id].obj_->on_event(children_[_id].obj_->full_name(), "Bio.Cell.Destroyed", "");
			children_[_id].autoforward_ = false;
			//children_[_id].obj_.reset();
			children_.erase(_id);			// try to recover this but may be not allowed
			RemoveSourceFromMsgQueue(_removed_molecule);
		}
		LOG_D(TAG, "Bio.Chromosome.Remove(%s) done", _id.c_str());
		break;
	}
	case "Bio.Chromosome.AssignByMessage"_hash:
	{
		mRNA_.first->assign(msg_name);
		break;
	}
	case "Bio.Chromosome.DecomposeMessage"_hash:
	{
		String _message_name, _field_name, _payload;
		Read(msg_name.str() + ".message_name", _message_name);
		Read(msg_name.str() + ".field_name", _field_name);
		Read(msg_name.str() + ".payload", _payload);
		IBiomolecule* _packer = this;
		while (_packer->full_name() != "")
		{
			if (((Chromosome*)_packer)->mRNA_.first->decompose(_message_name, _payload, _field_name) == true)
			{
				break;
			}
			else
			{
				_packer = _packer->owner();
			}
		}
		break;
	}
	case "Bio.Cell.Snapshot"_hash:
	{
		Bio::Chromosome::SessionInfo _session_info;
		Snapshot(_session_info);
		Write(Bio::Chromosome::SessionInfo::descriptor()->full_name() + ".Result", _session_info.SerializeAsString());
		break;
	}
	case "Bio.Cell.RevertSnapshot"_hash:
	{
		String _session_info_serialized;
		Bio::Chromosome::SessionInfo _session_info;
		Read(Bio::Chromosome::SessionInfo::descriptor()->full_name() + ".Result", _session_info_serialized);
		if (_session_info_serialized != "" && _session_info.ParseFromString(_session_info_serialized) == true)
		{
			RevertSnapshot(_session_info);
		}
		break;
	}
	case "Bio.Cell.RemoveSourceFromMsgQueue"_hash:
	{
		owner()->do_event("Bio.Cell.RemoveSourceFromMsgQueue");
		break;
	}
	case "Bio.Chromosome.RegisterActivity"_hash:
	{
		String _id, _namespace, _name, _payload;
		Read(msg_name.str() + ".id", _id);
		Read(msg_name.str() + ".namespace", _namespace);
		Read(msg_name.str() + ".name", _name);
		Read(msg_name.str() + ".payload", _payload);
		activities_[_id] = { _namespace, _name, _payload };
		LOG_I(TAG, "Chromosome::RegisterActivity(%s, %s) payload size=%lld, total count=%lld", _name.c_str(), _id.c_str(), _payload.size(), activities_.size());
		break;
	}
	case "Bio.Chromosome.UnregisterActivity"_hash:
	{
		String _id, _name;
		Read(msg_name.str() + ".id", _id);
		Read(msg_name.str() + ".name", _name);
		if (activities_.count(_id) > 0 && activities_[_id].name_ == _name)
		{
			activities_.erase(_id);
			LOG_I(TAG, "Chromosome::UnregisterActivity(%s, %s) total count=%lld", _name.c_str(), _id.c_str(), activities_.size());
		}
		break;
	}
	case "Bio.Chromosome.RestoreActivity"_hash:
	{
		String _session_info_serialized;
		Bio::Chromosome::SessionInfo _session_info;
		Read(Bio::Chromosome::SessionInfo::descriptor()->full_name() + ".Result", _session_info_serialized);
		if (_session_info_serialized != "" && _session_info.ParseFromString(_session_info_serialized) == true)
		{
			RestoreActivity(_session_info);
		}
		break;
	}
	case "Bio.Chromosome.AddProto"_hash:
	{
		String _filename;
		Read(msg_name.str() + ".filename", _filename);
		if (_filename == "")
			break;
		AddProto(root_path_.str() + _filename);
		break;
	}
	case "Bio.Chromosome.GenerateUUID"_hash:
	{
		String _target_model;
		Read(msg_name.str() + ".target_model_name", _target_model);
		if (_target_model != "")
			Write(_target_model, uuids::to_string(uuids::uuid_system_generator()()));
		break;
	}
	case "Bio.Chromosome.SetAutoForward"_hash:
	{
		String _id;
		Read("Bio.Chromosome.SetAutoForward.id", _id);
		if (children_.count(_id) > 0)
		{
			Read("Bio.Chromosome.SetAutoForward.on", children_[_id].autoforward_);
		}
		break;
	}
	case "Bio.Chromosome.StringFormat"_hash:
	{
		DNA_.first->do_event(Bio::Chromosome::StringFormat::descriptor()->full_name());
		break;
	}
	case "Bio.Chromosome.Substring"_hash:
	{
		DNA_.first->do_event(Bio::Chromosome::Substring::descriptor()->full_name());
		break;
	}
	case "Bio.Chromosome.ReplaceString"_hash:
	{
		DNA_.first->do_event(Bio::Chromosome::ReplaceString::descriptor()->full_name());
		break;
	}
	case "Bio.Chromosome.SplitString"_hash:
	{
		DNA_.first->do_event(Bio::Chromosome::SplitString::descriptor()->full_name());
		break;
	}
	case "Bio.Chromosome.GetActiveStates"_hash:
	{
		DNA_.first->do_event(Bio::Chromosome::GetActiveStates::descriptor()->full_name());
		break;
	}
	case "Bio.Chromosome.SetActiveStates"_hash:
	{
		DNA_.first->do_event(Bio::Chromosome::SetActiveStates::descriptor()->full_name());
		break;
	}
	case "Bio.Chromosome.GenerateRandom"_hash:
	{
		int _min, _max;
		String _target_model;
		Read(msg_name.str() + ".min_limit", _min);
		Read(msg_name.str() + ".max_limit", _max);
		Read(msg_name.str() + ".target_model_name", _target_model);
		typedef std::chrono::high_resolution_clock hrclock;
#if defined(_M_X64) || defined(__amd64__)
		unsigned long long _seed = hrclock::now().time_since_epoch().count();
		std::mt19937_64 _generator(_seed);
#else
		unsigned int _seed = (unsigned int)hrclock::now().time_since_epoch().count();
		std::mt19937 _generator(_seed);
#endif
		std::uniform_int_distribution<int> _distribution(_min, _max);
		int _generated_random = _distribution(_generator);
		LOG_I(TAG, "Bio.Chromosome.GenerateRandom min=%d, max=%d, generated=%d", _min, _max, _generated_random);
		if (_target_model != "")
			Write(_target_model, std::to_string(_generated_random));
		break;
	}
	case "Bio.Chromosome.Sort"_hash:
	{
		json _source;
		String _source_model;
		String _target_model;
		Read(msg_name.str() + ".source", _source);
		Read(msg_name.str() + ".source_model_name", _source_model);
		Read(msg_name.str() + ".target_model_name", _target_model);
		if (_source.empty())
		{
			if (!_source_model.empty())
				Read(_source_model, _source);
		}
		if (!_source.empty())
		{
			try
			{
				std::sort(_source.begin(), _source.end());
				if (!_target_model.empty())
					Write(_target_model, _source);
			}
			catch (const json::exception)
			{
				String _source_str;
				Read(msg_name.str() + ".source", _source_str);
				Read(_source_model, _source_str);
				Array<String> _string_array = split(_source_str, ",");
				if (_string_array.empty() && !_source_model.empty())
				{
					Read(_source_model, _string_array);
				}
				std::sort(_string_array.begin(), _string_array.end());
				if (!_target_model.empty())
					Write(_target_model, _string_array);
			}
		}
		break;
	}
	default:
	{
		for (auto _elem : handlers_)
		{
			if (_elem.obj_ != nullptr)
				_elem.obj_->OnEvent(msg_name);
		}
		break;
	}
	}
}

void Chromosome::SplitPathName(const String& full_filename, String& path, String& filename)
{
	String _full_filename = full_filename;
	std::replace(_full_filename.begin(), _full_filename.end(), '\\', '/');
	size_t _start_pos = _full_filename.rfind('/');
	size_t _end_pos = _full_filename.rfind(".scxml");
	if (_start_pos == String::npos)
	{
		filename = _full_filename.substr(0, _end_pos);
		path = "./";
	}
	else
	{
		filename = _full_filename.substr(_start_pos + 1, _end_pos - _start_pos - 1);
		path = _full_filename.substr(0, _start_pos + 1);
	}
}


template<typename T>
const char* Chromosome::GetType(const T& value)
{
	return typeid(value).name();
}

template <typename T2>
void Chromosome::Deserialize(bool, const DynaArray& data, T2& value)
{
	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		ByteArray _data(data.size());
		memcpy(_data.data(), data.data(), data.size());
		zpp::serializer::memory_input_archive in(_data);

		int8_t _value;
		in(_value);
		TypeConversion(_value == 1, value);
	}
}

template <typename T>
void Chromosome::Deserialize(const char* type, const DynaArray& data, T& value)
{
	if (type == "")
		throw std::exception();

	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		size_t _type_hash = hash_((String)type);
		if (type_hash_table_[_type_hash] == type_bool)
			Deserialize(bool(), data, value);
		else
		{
			if (_type_hash == hash_((String)GetType(value)))
			{
				ByteArray _data(data.size());
				memcpy(_data.data(), data.data(), data.size());
				zpp::serializer::memory_input_archive in(_data);
				in(value);
			}
			else
			{
				typedef long long longlong;
				typedef unsigned long long ulonglong;
				switch (type_hash_table_[_type_hash])
				{
				case type_int32_t:
					Deserialize(int32_t(), data, value);
					break;
				case type_uint32_t:
					Deserialize(uint32_t(), data, value);
					break;
				case type_longlong_t:
					Deserialize(longlong(), data, value);
					break;
				case type_ulonglong_t:
					Deserialize(ulonglong(), data, value);
					break;
				case type_double:
					Deserialize(double(), data, value);
					break;
				case type_string:
					Deserialize(String(), data, value);
					break;

				case type_bool_array:
					Deserialize(Array<bool>(), data, value);
					break;
				case type_int32_t_array:
					Deserialize(Array<int32_t>(), data, value);
					break;
				case type_uint32_t_array:
					Deserialize(Array<uint32_t>(), data, value);
					break;
				case type_longlong_t_array:
					Deserialize(Array<longlong>(), data, value);
					break;
				case type_ulonglong_t_array:
					Deserialize(Array<ulonglong>(), data, value);
					break;
				case type_double_array:
					Deserialize(Array<double>(), data, value);
					break;
				case type_string_array:
					Deserialize(Array<String>(), data, value);
					break;
				case type_json:
					Deserialize(nlohmann::json(), data, value);
					break;
				default:
					std::cout << "Unsupported type" << std::endl;
					break;
				}
			}
		}
	}
}

void Chromosome::Read(const String& name, bool& value, bool auto_conversion)
{
	DynaArray _stored_type;
	DynaArray _buf;
	model()->Read(name, _stored_type, _buf, auto_conversion);
#ifdef __linux__
	if (_stored_type == "l")
		_stored_type = "x";
	else if (_stored_type == "m")
		_stored_type = "y";
#endif
	try {
		Deserialize(_stored_type.data(), _buf, value);
	}
	catch (...)
	{
		if (!discard_no_such_model_warning_)
		{
			if (name.substr(0, strlen("Bio.Cell.")) == "Bio.Cell." || name.substr(0, strlen("Bio.Chromosome.")) == "Bio.Chromosome.")
			{
				LOG_D(TAG, "No such key in Model, key name: %s", name.c_str());
			}
			else
			{
				LOG_W(TAG, "No such key in Model, key name: %s", name.c_str());
			}
		}
	}
}

void Chromosome::Read(const String& name, nlohmann::json& value, bool auto_conversion)
{
	DynaArray _stored_type;
	model()->Read(name, _stored_type, value, auto_conversion);
	if (!auto_conversion && _stored_type != GetType(value))
	{
		value = json();
	}
}

template<typename T>
void Chromosome::Read(const String& name, T& value, bool auto_conversion)
{
	DynaArray _stored_type;
	DynaArray _buf;
	model()->Read(name, _stored_type, _buf, auto_conversion);
#ifdef __linux__
	if (_stored_type == "l")
		_stored_type = "x";
	else if (_stored_type == "m")
		_stored_type = "y";
#endif
	try {
		Deserialize(_stored_type.data(), _buf, value);
	}
	catch (...)
	{
		if (!discard_no_such_model_warning_)
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
}

void Chromosome::Write(const String& name, const nlohmann::json& value)
{
	model()->Write(name, GetType(value), value);
}

template<typename T>
void Chromosome::Write(const String& name, const T& value)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	out(value);
	model()->Write(name, GetType(value), (DynaArray)data);
}

void Chromosome::Remove(const String& name)
{
	model()->Remove(name);
}

void Chromosome::SaveVersion()
{
	Write("Bio.Cell.version.Chromosome", (String)get_version());
	Write("Bio.Cell.version.major.Chromosome", VERSION_MAJOR);
	Write("Bio.Cell.version.minor.Chromosome", VERSION_MINOR);
	Write("Bio.Cell.version.build.Chromosome", VERSION_BUILD);
	Write("Bio.Cell.version.revision.Chromosome", (String)VERSION_REVISION);
	const long long TIME_DIFFERENCE = -8 * 60 * 60;
	Write("Bio.Cell.version.time.Chromosome", (long long)__TIME_UNIX__ + TIME_DIFFERENCE);
}

const char* Chromosome::get_version()
{
	static String _version = std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_BUILD) + "." + VERSION_REVISION;
	return _version.c_str();
}

time_t Chromosome::epoch()
{
	return std::time(nullptr);
}

void Chromosome::Snapshot(Bio::Chromosome::SessionInfo& session_info)
{
	// get active states from DNA
	Array<String> _state_list;
	DNA_.first->do_event(Bio::Chromosome::GetActiveStates::descriptor()->full_name());
	Read("Bio.Chromosome.GetActiveStates.Result.state_list", _state_list);
	for (auto elem : _state_list)
		session_info.add_state_list(std::move(elem));

	// get RNA list
	for (auto elem : handlers_)
	{
		session_info.add_rna_list(elem.name_);
	}

	// get activities
	for (auto elem : activities_)
	{
		Bio::Chromosome::SessionInfo::Activity* _activity = session_info.add_activities();
		_activity->set_name_space(elem.second.namespace_);
		_activity->set_name(elem.second.name_);
		_activity->set_payload(elem.second.payload_);
	}

	// get invoked children
	for (auto elem : children_)
	{
		Bio::Chromosome::SessionInfo* _child = session_info.add_children();
		_child->set_id(elem.first);
		_child->set_gene(elem.second.gene_);
		_child->set_auto_forward(elem.second.autoforward_);
		elem.second.obj_->Snapshot(*_child);
	}
}

void Chromosome::RevertSnapshot(const Bio::Chromosome::SessionInfo& session_info)
{
	// restore active states to DNA
	Array<String> _state_list(session_info.state_list().begin(), session_info.state_list().end());
	Write(Bio::Chromosome::SetActiveStates::descriptor()->full_name() + ".state_list", _state_list);
	DNA_.first->do_event(Bio::Chromosome::SetActiveStates::descriptor()->full_name());

	// restore RNA list
	for (auto elem : session_info.rna_list())
	{
		bool _found = false;
		for (auto handler : handlers_)
		{
			if (elem == handler.name_)
			{
				_found = true;
				break;
			}
		}
		if (!_found)
		{
			Write("Bio.Cell.NewRNA.name", elem);
			do_event("Bio.Cell.NewRNA");
		}
	}

	// restore invoked children
	for (auto elem : session_info.children())
	{
		if (children_.count(elem.id()) == 0)
		{
			Write("Bio.Chromosome.New.id", elem.id());
			Write("Bio.Chromosome.New.src", elem.gene());
			Write("Bio.Chromosome.New.autoforward", elem.auto_forward() ? (String)"true" : (String)"false");
			LOG_D(TAG, "Chromosome::RevertSnapshot(%s, %s)", elem.id().c_str(), elem.gene().c_str());
			do_event("Bio.Chromosome.New");
		}
		children_[elem.id()].obj_->RevertSnapshot(elem);
	}

	// restore activities
	//for (auto elem : session_info.activities())
	//{
	//	on_event(elem.name_space(), elem.name(), elem.payload());
	//}
}

void Chromosome::RestoreActivity(const Bio::Chromosome::SessionInfo& session_info)
{
	// restore activities
	for (auto elem : session_info.activities())
	{
		on_event(elem.name_space(), elem.name(), elem.payload());
	}
}

void Chromosome::AddProto(const String& filename)
{
	/*
	using namespace google::protobuf;
	String _path, _filename;
	SplitPathName(filename, _path, _filename);

	compiler::DiskSourceTree _proto_source_tree;
	_proto_source_tree.MapPath("", _path);
	compiler::Importer _importer(&_proto_source_tree, nullptr);
	const FileDescriptor* _file_desc = _importer.Import(_filename);

	if (_file_desc)
	{
		FileDescriptorProto _fileDescriptorProto;
		_file_desc->CopyTo(&_fileDescriptorProto);
		extra_desc_pool_.push_back(Obj<DescriptorPool>(new DescriptorPool()));
		const FileDescriptor* _file_desc = extra_desc_pool_.back()->BuildFile(_fileDescriptorProto);
		if (_file_desc != nullptr)
			bind(nullptr, filename.c_str());
		else
		{
			LOG_E(TAG, "Fail to parsing \"%s\" when AddProto", filename.c_str());
			extra_desc_pool_.pop_back();
}
	}
	else
	{
		LOG_E(TAG, "Fail to AddProto \"%s\"", filename.c_str());
	}
	*/
	bind(nullptr, filename.c_str());
}

void Chromosome::RemoveSourceFromMsgQueue(const IBiomolecule* chromosome)
{
	//IBiomolecule* _molecule = this;
	//while (_molecule->full_name() != "")
	//{
	//	_molecule = _molecule->owner();
	//}
	//Write("Bio.Cell.RemoveSourceFromMsgQueue.source_chromosome", (uint64_t)chromosome);
	//_molecule->do_event("Bio.Cell.RemoveSourceFromMsgQueue");

	//DynaArray _payload = "";
	//IBiomolecule* _packer = this;
	Write("Bio.Cell.RemoveSourceFromMsgQueue.source_chromosome", (unsigned long long)chromosome);
	//while (_packer->full_name() != "")
	//{
	//	if (((Chromosome*)_packer)->mRNA_.first->pack(root_path_, "Bio.Cell.RemoveSourceFromMsgQueue", _payload) == true)
	//	{
	//		owner()->add_priority_event("Bio.Cell.RemoveSourceFromMsgQueue", _payload, this);
	//		break;
	//	}
	//	else
	//	{
	//		_packer = _packer->owner();
	//	}
	//}
	//if (_packer->full_name() == "")
	//{
	//	//LOG_D(TAG, "RAISE: pack(%s) not found", "Bio.Cell.RemoveSourceFromMsgQueue");
	//	//owner()->add_priority_event("Bio.Cell.RemoveSourceFromMsgQueue", "", this);
	//	owner()->do_event("Bio.Cell.RemoveSourceFromMsgQueue");
	//}
	owner()->do_event("Bio.Cell.RemoveSourceFromMsgQueue");
}

bool Chromosome::IdentifySource(const IBiomolecule* chromosome, IBiomolecule* namespace_owner)
{
	bool _ret = false;
	IBiomolecule* _namespace_owner = namespace_owner;
	if (namespace_owner == nullptr)
	{
		_namespace_owner = this;
		if (_namespace_owner->owner() != nullptr && strcmp(typeid(*(_namespace_owner->owner())).name(), typeid(Chromosome).name()) == 0)
		{
			_namespace_owner = _namespace_owner->owner();
		}
	}
	if (chromosome == _namespace_owner)
		_ret = true;
	else
	{
		for (auto& _child : ((Chromosome*)_namespace_owner)->children_)
		{
			if (_child.second.obj_.get() == chromosome)
				return true;
			if (_child.second.obj_->IdentifySource(chromosome, _child.second.obj_.get()) == true)
				return true;
		}
	}
	return _ret;
}

void* Chromosome::FindMessageTypeByName(const char* name)
{
	return (void*)google::protobuf::DescriptorPool::internal_generated_pool()->FindMessageTypeByName(name);
}

template <typename T1, typename T2>
void Chromosome::Deserialize(T1, const DynaArray& data, T2& value)
{
	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		ByteArray _data(data.size());
		memcpy(_data.data(), data.data(), data.size());
		zpp::serializer::memory_input_archive in(_data);

		T1 _value;
		in(_value);
		TypeConversion(_value, value);
	}
}

template <typename T2>
void Chromosome::Deserialize(Array<bool>, const DynaArray& data, T2& value)
{
	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		ByteArray _data(data.size());
		memcpy(_data.data(), data.data(), data.size());
		zpp::serializer::memory_input_archive in(_data);
		Array<int8_t> _value_in_storage;
		in(_value_in_storage);

		Array<bool> _value;
		std::transform(_value_in_storage.cbegin(), _value_in_storage.cend(), std::back_inserter(_value), [](const int8_t& elem)->bool {
			return elem == 1;
			});
		TypeConversion(_value, value);
	}
}

template <typename T2>
void Chromosome::Deserialize(json, const DynaArray& data, T2& value)
{
	if (data.size() != 0 || !discard_no_such_model_warning_)
	{
		String _value = data.str();
		TypeConversion(_value, value);
	}
}

void Chromosome::Deserialize(const char* type, const DynaArray& data, bool& value)
{
	value = false;
	size_t _type_hash = hash_(type);
	if (type_hash_table_[_type_hash] == type_bool)
	{
		ByteArray _data(data.size());
		memcpy(_data.data(), data.data(), data.size());
		zpp::serializer::memory_input_archive in(_data);

		int8_t _value;
		in(_value);
		value = (_value == 1);
	}
	else
	{
		typedef long long longlong;
		typedef unsigned long long ulonglong;
		switch (type_hash_table_[_type_hash])
		{
		case type_int32_t:
			Deserialize(int32_t(), data, value);
			break;
		case type_uint32_t:
			Deserialize(uint32_t(), data, value);
			break;
		case type_longlong_t:
			Deserialize(longlong(), data, value);
			break;
		case type_ulonglong_t:
			Deserialize(ulonglong(), data, value);
			break;
		case type_double:
			Deserialize(double(), data, value);
			break;
		case type_string:
			Deserialize(String(), data, value);
			break;

		case type_bool_array:
			Deserialize(Array<bool>(), data, value);
			break;
		case type_int32_t_array:
			Deserialize(Array<int32_t>(), data, value);
			break;
		case type_uint32_t_array:
			Deserialize(Array<uint32_t>(), data, value);
			break;
		case type_longlong_t_array:
			Deserialize(Array<longlong>(), data, value);
			break;
		case type_ulonglong_t_array:
			Deserialize(Array<ulonglong>(), data, value);
			break;
		case type_double_array:
			Deserialize(Array<double>(), data, value);
			break;
		case type_string_array:
			Deserialize(Array<String>(), data, value);
			break;
		case type_json:
			Deserialize(nlohmann::json(), data, value);
			break;
		}
	}
}

template<typename T1, typename T2>
void Chromosome::TypeConversion(const T1& src, T2& dest) {
	dest = (T2)src;
}

template<typename T>
void Chromosome::TypeConversion(const T& src, String& dest) { dest = std::to_string(src); };
void Chromosome::TypeConversion(const double& src, String& dest)
{
	dest = to_string_with_precision(src, DBL_DIG);
};
template<typename T>
void Chromosome::TypeConversion(const String& src, T& dest) {
	if (is_number(src))
		dest = (T)stod(src);
	else
		dest = (T)0.0;
};
void Chromosome::TypeConversion(const bool& src, String& dest) { dest = (src == true ? "true" : "false"); };
void Chromosome::TypeConversion(const String& src, String& dest) { dest = src; };
template<typename T>
void Chromosome::TypeConversion(const T& src, Array<String>& dest) { dest.push_back(std::to_string(src)); };
void Chromosome::TypeConversion(const double& src, Array<String>& dest)
{
	dest.push_back(to_string_with_precision(src, DBL_DIG));
};
template<typename T1, typename T2>
void Chromosome::TypeConversion(const T1& src, Array<T2>& dest) { dest.push_back((T2)src); };
template<typename T>
void Chromosome::TypeConversion(const String& src, Array<T>& dest) {
	Array<String> _string_array;
	if (src.size() > 2 && src[0] == '[' && src.back() == ']')
	{
		_string_array = split(src.substr(1, src.size() - 2), ",");
	}
	TypeConversion(_string_array, dest);
};


template<typename T1, typename T2>
void Chromosome::TypeConversion(const Array<T1>& src, T2& dest) {
	dest = (src.size() > 0 ? (T2)src.front() : dest);
};

template<typename T>
void Chromosome::TypeConversion(const Array<T>& src, String& dest) {
	dest = "[";
	for (auto _elem : src)
	{
		dest += std::to_string(_elem) + ",";
	}
	if (dest.size() > 1)
		dest.back() = ']';
	else
		dest.push_back(']');
};

template<typename T>
void Chromosome::TypeConversion(const Array<String>& src, T& dest) {
	if (src.size() > 1)
		TypeConversion(src[0], dest);
	else
		dest = (T)0.0;
};

void Chromosome::TypeConversion(const Array<String>& src, String& dest) {
	dest = "[";
	for (auto _elem : src)
	{
		dest += _elem + ",";
	}
	if (dest.size() > 1)
		dest.back() = ']';
	else
		dest.push_back(']');
};

void Chromosome::TypeConversion(const String& src, Array<String>& dest)
{
	if (src.size() > 2 && src[0] == '[' && src.back() == ']')
	{
		String _src = std::regex_replace(src, std::regex("\",\""), ",");
		if (_src.size() > 3 && _src[1] == '\"')
			_src.erase(1, 1);
		if (_src.size() > 3 && _src[_src.size() - 2] == '\"')
			_src.erase(_src.size() - 2, 1);
		dest = split(_src.substr(1, _src.size() - 2), ",");
	}
};

void Chromosome::TypeConversion(const Array<bool>& src, String& dest) {
	dest = "[";
	for (auto _elem : src)
	{
		dest += String(_elem == true ? "true" : "false") + ",";
	}
	if (dest.size() > 1)
		dest.back() = ']';
	else
		dest.push_back(']');
};

// Array to Array
template<typename T1, typename T2>
void Chromosome::TypeConversion(const Array<T1>& src, Array<T2>& dest) {
	std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const T1& elem)->T2 {
		return (T2)elem;
		});
};

template<typename T>
void Chromosome::TypeConversion(const Array<T>& src, Array<String>& dest) {
	std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const T& elem)->String {
		return std::to_string(elem);
		});
};

template<typename T>
void Chromosome::TypeConversion(const Array<String>& src, Array<T>& dest) {
	std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [this](const String& elem)->T {
		T _retval;
		this->TypeConversion(elem, _retval);
		return _retval;
		});
};

void Chromosome::TypeConversion(const Array<String>& src, Array<String>& dest) {
	dest = src;
};

void Chromosome::BuildTypeHastable()
{
	const char* _type_id_table_name[] = {
		typeid(const bool).name(),
		typeid(int32_t).name(),
		typeid(uint32_t).name(),
		typeid(long long).name(),
		typeid(unsigned long long).name(),
		typeid(double).name(),
		typeid(String).name(),
		typeid(Array<bool>).name(),
		typeid(Array<int32_t>).name(),
		typeid(Array<uint32_t>).name(),
		typeid(Array<long long>).name(),
		typeid(Array<unsigned long long>).name(),
		typeid(Array<double>).name(),
		typeid(Array<String>).name(),
		typeid(json).name()
	};
	for (int i = 0; i < MAX_TYPE_COUNT; i++)
	{
		type_hash_table_[hash_(_type_id_table_name[i])] = i;
	}
}

template <typename T>
String Chromosome::to_string_with_precision(const T a_value, const int n)
{
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return out.str();
}
