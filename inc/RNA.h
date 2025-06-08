#pragma once
#include "IBiomolecule.h"
#include <google/protobuf/descriptor_database.h>
#include "compile_time.h"

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
#define VERSION_REVISION " "
#endif

BIO_BEGIN_NAMESPACE

#define DECLARE_EXPORT_RW_API(type, rpostfix, wpostfix)		\
	virtual void Read(const String& name, type& value) rpostfix;	\
	virtual void Write(const String& name, const type& value, bool internal_use = false) wpostfix;
#define PURE_FUNC =0

class IRNAImpl
{
protected:
	typedef Map<String, bool> Map_String_bool;
	typedef Map<String, int32_t> Map_String_int32;
	typedef Map<String, uint32_t> Map_String_uint32;
	typedef Map<String, long long> Map_String_longlong;
	typedef Map<String, unsigned long long> Map_String_ulonglong;
	typedef Map<String, double> Map_String_double;
	typedef Map<String, String> Map_String_String;

	typedef UoMap<String, bool> UoMap_String_bool;
	typedef UoMap<String, int32_t> UoMap_String_int32;
	typedef UoMap<String, uint32_t> UoMap_String_uint32;
	typedef UoMap<String, long long> UoMap_String_longlong;
	typedef UoMap<String, unsigned long long> UoMap_String_ulonglong;
	typedef UoMap<String, double> UoMap_String_double;
	typedef UoMap<String, String> UoMap_String_String;
public:
	IRNAImpl() {};
	virtual ~IRNAImpl() {};
	DECLARE_EXPORT_RW_API(bool, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Array<bool>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Map_String_bool, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(UoMap_String_bool, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Set<bool>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(int32_t, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(uint32_t, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(long long, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(unsigned long long, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(double, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(String, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Array<int32_t>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Array<uint32_t>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Array<long long>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Array<unsigned long long>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Array<double>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Array<String>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Map_String_int32, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Map_String_uint32, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Map_String_longlong, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Map_String_ulonglong, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Map_String_double, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Map_String_String, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(UoMap_String_int32, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(UoMap_String_uint32, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(UoMap_String_longlong, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(UoMap_String_ulonglong, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(UoMap_String_double, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(UoMap_String_String, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Set<int32_t>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Set<uint32_t>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Set<long long>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Set<unsigned long long>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Set<double>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(Set<String>, PURE_FUNC, PURE_FUNC)
		DECLARE_EXPORT_RW_API(nlohmann::json, PURE_FUNC, PURE_FUNC)

		virtual void Write(const String& name, const char* value, bool internal_use = false) PURE_FUNC;

	virtual void Remove(const String& name, bool internal_only = false) PURE_FUNC;
	virtual void Clone(const String& target_name, const String& src_name) PURE_FUNC;
	virtual void SendEvent(const String& name, const String& payload) PURE_FUNC;
	virtual void RaiseEvent(const String& name, const String& payload, IBiomolecule* src) PURE_FUNC;
	virtual void OnEvent(const DynaArray& name) PURE_FUNC;

	virtual void Bind() PURE_FUNC;
	virtual const char* name() PURE_FUNC;
	virtual const char* version() PURE_FUNC;
};

class PUBLIC_API RNA
{
public:
	RNA(IBiomolecule* owner, const char* name, RNA* callback);
	virtual ~RNA();

public:
	void SendEvent(const String& name, const String& payload = "");
	void RaiseEvent(const String& name, const String& payload = "", IBiomolecule* src = nullptr);
	virtual void OnEvent(const DynaArray& name) {};
	virtual void* FindMessageTypeByName(const char* name) {
		return (void*)google::protobuf::DescriptorPool::internal_generated_pool()->FindMessageTypeByName(name);
	}
	virtual void* FindValueByName(const char* msg_name, int index, const char* name) {
		using namespace google::protobuf;
		const Descriptor* _desc = DescriptorPool::internal_generated_pool()->FindMessageTypeByName(msg_name);
		if (_desc != nullptr)
		{
			const FieldDescriptor* _field_desc = _desc->field(index);
			if (_field_desc != nullptr && _field_desc->type() == FieldDescriptor::TYPE_ENUM)
			{
				return (void*)_field_desc->enum_type()->FindValueByName(name);
			}
		}
		return nullptr;
	}
protected:
	template<typename T> T ReadValue(const String& strName);
	template<typename T> void WriteValue(const String& strName, const T& Value, bool internal_use = false);
	void Remove(const String& name, bool internal_only = false);
	void Clone(const String& target_name, const String& src_name);
	void init() {
		impl_->Bind();
		SaveVersion();
	};
	String GetRootPath();
	String GetVersion() {
		return (std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_BUILD) + "." + VERSION_REVISION).c_str();
	};

private:
	void SaveVersion()
	{
		WriteValue(String("Bio.Cell.version.") + impl_->name(), GetVersion());
		WriteValue(String("Bio.Cell.version.major.") + impl_->name(), VERSION_MAJOR);
		WriteValue(String("Bio.Cell.version.minor.") + impl_->name(), VERSION_MINOR);
		WriteValue(String("Bio.Cell.version.build.") + impl_->name(), VERSION_BUILD);
		WriteValue(String("Bio.Cell.version.revision.") + impl_->name(), (String)VERSION_REVISION);
		LOG_D(impl_->name(), "version: %s",
			ReadValue<String>(String("Bio.Cell.version.") + impl_->name()).empty() ? "" : ReadValue<String>(String("Bio.Cell.version.") + impl_->name()).c_str());
		const long long TIME_DIFFERENCE = -8 * 60 * 60;
		WriteValue(String("Bio.Cell.version.time.") + impl_->name(), (long long)__TIME_UNIX__ + TIME_DIFFERENCE);
	}
private:
	IRNAImpl* impl_;
};

template<typename T>
T RNA::ReadValue(const String& name)
{
	T _ret;
	impl_->Read(name, _ret);
	return _ret;
}

template<typename T>
void RNA::WriteValue(const String& name, const T& value, bool internal_use)
{
	impl_->Write(name, value, internal_use);
}

template<class>struct hasher;
template<>
struct hasher<std::string> {
	std::size_t constexpr operator()(char const* input)const {
		return *input ?
			static_cast<unsigned int>(*input) + 33 * (*this)(input + 1) :
			5381;
	}
	std::size_t operator()(const std::string& str) const {
		return (*this)(str.c_str());
	}
};
template<>
struct hasher<const char*> {
	std::size_t constexpr operator()(char const* input)const {
		return *input ?
			static_cast<unsigned int>(*input) + 33 * (*this)(input + 1) :
			5381;
	}
	std::size_t operator()(const std::string& str) const {
		return (*this)(str.c_str());
	}
};
template<typename T>
std::size_t constexpr hash(T&& t) {
	return hasher< typename std::decay<T>::type >()(std::forward<T>(t));
}
//std::size_t constexpr hash(const char* t) {
//	return hash(const std::string(t));
//}
inline namespace literals {
	std::size_t constexpr operator "" _hash(const char* s, size_t) {
		return hasher<std::string>()(s);
	}
}

typedef RNA* (*CREATE_RNA_INSTANCE_FUNCTION)(IBiomolecule* owner);

#ifdef STATIC_API
extern Map<String, BioSys::CREATE_RNA_INSTANCE_FUNCTION> RNA_Map_;
#endif

BIO_END_NAMESPACE

