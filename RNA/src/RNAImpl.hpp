#pragma once
#include "RNAImpl.h"
#include "internal/IModel.h"
#include "internal/utils/serializer.h"
#include <typeinfo>

USING_BIO_NAMESPACE

#define TAG "RNA"

enum type_id_table_
{
	type_bool,
	type_int32_t,
	type_uint32_t,
	type_int64_t,
	type_uint64_t,
	type_double,
	type_string,

	type_bool_array,
	type_int32_t_array,
	type_uint32_t_array,
	type_int64_t_array,
	type_uint64_t_array,
	type_double_array,
	type_string_array,

	type_bool_set,
	type_int32_t_set,
	type_uint32_t_set,
	type_int64_t_set,
	type_uint64_t_set,
	type_double_set,
	type_string_set,

	type_bool_map,
	type_int32_t_map,
	type_uint32_t_map,
	type_int64_t_map,
	type_uint64_t_map,
	type_double_map,
	type_string_map,

	type_bool_uomap,
	type_int32_t_uomap,
	type_uint32_t_uomap,
	type_int64_t_uomap,
	type_uint64_t_uomap,
	type_double_uomap,
	type_string_uomap,
};

const Obj<IModel> RNAImpl::model() {
	return owner()->model();
}

void RNAImpl::activate() {
	assert(false);
}

void RNAImpl::add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src) {
	//owner()->add_event(msg_name, payload, owner());
	owner()->add_event(msg_name, payload, nullptr);
}

void RNAImpl::add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src) {
	owner()->add_priority_event(msg_name, payload, owner());
}

void RNAImpl::on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload)
{
	assert(false);
}

void RNAImpl::do_event(const DynaArray& msg_name)
{
 	assert(false);
}

void RNAImpl::SendEvent(const String& name, const String& payload)
{
	add_event(name, payload, nullptr);
}

void RNAImpl::OnEvent(const DynaArray& name)
{
	callback_->OnEvent(name);
}

void RNAImpl::Remove(const String& name, bool internal_only)
{
	model()->Remove(name, internal_only);
}

void RNAImpl::Clone(const String& target_name, const String& src_name)
{
	model()->Clone(target_name, src_name);
}

void RNAImpl::Bind(void* desc_pool)
{
	owner()->bind(this, desc_pool);
}

const char* RNAImpl::name()
{
	return IBiomolecule::name().data();
}

const char* RNAImpl::get_version() 
{ 
	return (std::to_string(VERSION_MAJOR)+"."+std::to_string(VERSION_MINOR)+"."+std::to_string(VERSION_BUILD)+"."+VERSION_REVISION).c_str(); 
};

#pragma region SINGLE
	// Single to Single
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const T1& src, T2& dest) { dest = (T2)src; };
	void RNAImpl::TypeConversion(const bool& src, String& dest) { dest = (src==true?"true":"false"); };
	void RNAImpl::TypeConversion(const String& src, bool& dest) { 
		if (src == "true")
			dest = true;
		else
		{
			if (is_number(src))
				dest = (bool)stod(src);
			else
				dest = false;
		}
	};
	template<typename T>
	void RNAImpl::TypeConversion(const T& src, String& dest) { dest = std::to_string(src); };
	template<typename T>
	void RNAImpl::TypeConversion(const String& src, T& dest) { 
		if (is_number(src))
			dest = (T)stod(src);
		else
			dest = (T)0.0;
	};
	void RNAImpl::TypeConversion(const String& src, String& dest) { dest = src; };

	// Single to Array
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const T1& src, Array<T2>& dest) { dest.push_back((T2)src); };
	void RNAImpl::TypeConversion(const bool& src, Array<String>& dest) { dest.push_back(src == true ? "true" : "false"); };
	template<typename T>
	void RNAImpl::TypeConversion(const T& src, Array<String>& dest) { dest.push_back(std::to_string(src)); };
	void RNAImpl::TypeConversion(const String& src, Array<String>& dest) 
	{ 
		if (src.size() > 2 && src[0] == '[' && src.back() == ']')
		{
			dest = split(src.substr(1, src.size() - 2), ",");
		}
	};
	template<typename T>
	void RNAImpl::TypeConversion(const String& src, Array<T>& dest) {
		Array<String> _string_array;
		if (src.size() > 2 && src[0] == '[' && src.back() == ']')
		{
			_string_array = split(src.substr(1, src.size() - 2), ",");
		}
		TypeConversion(_string_array, dest);
	};

	// Single to Set
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const T1& src, Set<T2>& dest) { dest.insert((T2)src); };
	void RNAImpl::TypeConversion(const bool& src, Set<String>& dest) { dest.insert(src == true ? "true" : "false"); };
	template<typename T>
	void RNAImpl::TypeConversion(const T& src, Set<String>& dest) { dest.insert(std::to_string(src)); };
	void RNAImpl::TypeConversion(const String& src, Set<String>& dest) { 
		Array<String> _string_array;
		if (src.size() > 2 && src[0] == '[' && src.back() == ']')
		{
			_string_array = split(src.substr(1, src.size() - 2), ",");
		}
		TypeConversion(_string_array, dest);
	};
	template<typename T>
	void RNAImpl::TypeConversion(const String& src, Set<T>& dest) {
		Array<String> _string_array;
		if (src.size() > 2 && src[0] == '[' && src.back() == ']')
		{
			_string_array = split(src.substr(1, src.size() - 2), ",");
		}
		TypeConversion(_string_array, dest);
	};
#pragma endregion

#pragma region ARRAY
	// Array to Single
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Array<T1>& src, T2& dest) { dest = (src.size() > 0?(T2)src.front():dest);  };
	template<typename T>
	void RNAImpl::TypeConversion(const Array<T>& src, String& dest) { 
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
	void RNAImpl::TypeConversion(const Array<String>& src, T& dest) { 
		if (src.size() > 1)
			TypeConversion(src[0], dest);
		else
			dest = (T)0.0;
	};
	void RNAImpl::TypeConversion(const Array<String>& src, String& dest) {
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
	void RNAImpl::TypeConversion(const Array<bool>& src, String& dest) { 
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
	void RNAImpl::TypeConversion(const Array<T1>& src, Array<T2>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const T1& elem)->T2 {
			return (T2)elem;
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Array<T>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const T& elem)->String {
			return std::to_string(elem);
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Array<String>& src, Array<T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [this](const String& elem)->T {
			T _retval;
			this->TypeConversion(elem, _retval);
			return _retval;
			});
	};
	void RNAImpl::TypeConversion(const Array<String>& src, Array<String>& dest) {
		dest = src;
	};
	void RNAImpl::TypeConversion(const Array<bool>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const bool& elem)->String {
			return elem == true ? "true" : "false";
			});
	};

	// Array to Set
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Array<T1>& src, Set<T2>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const T1& elem)->T2 {
			return (T2)elem;
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Array<T>& src, Set<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const T& elem)->String {
			return std::to_string(elem);
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Array<String>& src, Set<T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [this](const String& elem)->T {
			T _retval;
			this->TypeConversion(elem, _retval);
			return _retval;
			});
	};
	void RNAImpl::TypeConversion(const Array<String>& src, Set<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const String& elem)->String {
			return elem;
			});
	};
#pragma endregion

#pragma region SET
	// Set to Single
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Set<T1>& src, T2& dest) { 
		if (src.size() > 0)
			dest = (T2)*src.begin();
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Set<T>& src, String& dest) {
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
	void RNAImpl::TypeConversion(const Set<String>& src, T& dest) {
		if (src.size() > 0)
			TypeConversion(*src.begin(), dest);
	};
	void RNAImpl::TypeConversion(const Set<String>& src, String& dest) {
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
	void RNAImpl::TypeConversion(const Set<bool>& src, String& dest) { dest = (src.size() > 0 && *src.begin() == true ? "true" : "false"); };

	// Set to Array
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Set<T1>& src, Array<T2>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const T1& elem)->T2 {
			return (T2)elem;
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Set<T>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const T& elem)->String {
			return std::to_string(elem);
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Set<String>& src, Array<T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [this](const String& elem)->T {
			T _retval;
			this->TypeConversion(elem, _retval);
			return _retval;
			});
	};
	void RNAImpl::TypeConversion(const Set<String>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const String& elem)->String {
			return elem;
			});
	};
	void RNAImpl::TypeConversion(const Set<bool>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const bool& elem)->String {
			return (elem == true ? "true" : "false");
			});
	};

	// Set to Set
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Set<T1>& src, Set<T2>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const T1& elem)->T2 {
			return (T2)elem;
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Set<T>& src, Set<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const T& elem)->String {
			return std::to_string(elem);
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Set<String>& src, Set<T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [this](const String& elem)->T {
			T _retval;
			this->TypeConversion(elem, _retval);
			return _retval;
			});
	};
	void RNAImpl::TypeConversion(const Set<String>& src, Set<String>& dest) {
		dest = src;
	};
	void RNAImpl::TypeConversion(const Set<bool>& src, Set<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const bool& elem)->String {
			return (elem==true?"true":"false");
			});
	};
#pragma endregion

#pragma region MAP
	// XXX to Map : not supported
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const T1& src, Map<String, T2>& dest) { };
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Array<T1>& src, Map<String, T2>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const Array<String>& src, Map<String, T>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const String& src, Map<String, T>& dest) {};
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Set<T1>& src, Map<String, T2>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const Set<String>& src, Map<String, T>& dest) { };

	// Map to Single
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Map<String, T1>& src, T2& dest) {
		if (src.size() > 0)
			dest = (T2)src.begin()->second;
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, T>& src, String& dest) {
		dest = "[";
		for (auto elem : src)
			dest += "{\"key\":" + elem.first + ",\"value\":" + std::to_string(elem.second) + "},";
		if (dest.size() > 1)
			dest.back() = ']';
		else
			dest.push_back(']');
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, String>& src, T& dest) {
		if (src.size() > 0)
			TypeConversion(src.begin()->second, dest);
	};
	void RNAImpl::TypeConversion(const Map<String, String>& src, String& dest) {
		dest = "[";
		for (auto elem : src)
			dest += "{\"key\":" + elem.first + ",\"value\":" + elem.second + "},";
		if (dest.size() > 1)
			dest.back() = ']';
		else
			dest.push_back(']');
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, bool>& src, T& dest) {
		if (src.size() > 0)
			dest = (T)(src.begin()->second?1:0);
	};
	void RNAImpl::TypeConversion(const Map<String, bool>& src, String& dest) {
		dest = "[";
		for (auto elem : src)
			dest += "{\"key\":" + elem.first + ",\"value\":" + (elem.second==true?"true":"false") + "},";
		if (dest.size() > 1)
			dest.back() = ']';
		else
			dest.push_back(']');
	};

	// Map to Set : not supported
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Map<String, T1>& src, Set<T2>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, T>& src, Set<String>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, String>& src, Set<T>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, bool>& src, Set<T>& dest) { };
	void RNAImpl::TypeConversion(const Map<String, bool>& src, Set<String>& dest) { };
	void RNAImpl::TypeConversion(const Map<String, String>& src, Set<String>& dest) { };

	// Map to Array
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Map<String, T1>& src, Array<T2>& dest) { 
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->T2 {
			return (T2)elem.second;
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, T>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->String {
			return String("[") + elem.first + "," + std::to_string(elem.second) + "]";
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, String>& src, Array<T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [this](const auto& elem)->T {
			T _retval;
			this->TypeConversion(elem.second, _retval);
			return _retval;
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, bool>& src, Array<T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->T {
			return (T)(elem.second?1:0);
			});
	};
	void RNAImpl::TypeConversion(const Map<String, String>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->String {
			return String("[") + elem.first + "," + elem.second + "]";
			});
	};
	void RNAImpl::TypeConversion(const Map<String, bool>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->String {
			return String("[") + elem.first + "," + (elem.second == true ? "true" : "false") + "]";
			});
	};

	// Map to Map
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Map<String, T1>& src, Map<String, T2>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, T1>& elem)->Pair<String, T2> {
			return { elem.first, (T2)elem.second };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, String>& src, Map<String, T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [this](const Pair<String, String>& elem)->Pair<String, T> {
			T _retval;
			this->TypeConversion(elem.second, _retval);
			return { elem.first, _retval };
		});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, T>& src, Map<String, String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, T>& elem)->Pair<String, String> {
			return { elem.first, std::to_string(elem.second) };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, bool>& src, Map<String, T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String,bool>& elem)->Pair<String, T> {
			return { elem.first, (T)(elem.second?1:0) };
		});
	};
	void RNAImpl::TypeConversion(const Map<String, bool>& src, Map<String, String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, bool>& elem)->Pair<String, String> {
			return { elem.first, (elem.second ? "true" : "false") };
			});
	};
	void RNAImpl::TypeConversion(const Map<String, String>& src, Map<String, String>& dest) {
		dest = src;
	};

	// Map to UoMap
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Map<String, T1>& src, UoMap<String, T2>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, T1>& elem)->Pair<String, T2> {
			return { elem.first, (T2)elem.second };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, String>& src, UoMap<String, T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [this](const Pair<String, String>& elem)->Pair<String, T> {
			T _retval;
			this->TypeConversion(elem.second, _retval);
			return { elem.first, _retval };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, T>& src, UoMap<String, String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, T>& elem)->Pair<String, String> {
			return { elem.first, std::to_string(elem.second) };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const Map<String, bool>& src, UoMap<String, T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, bool>& elem)->Pair<String, T> {
			return { elem.first, (T)(elem.second ? 1 : 0) };
			});
	};
	void RNAImpl::TypeConversion(const Map<String, bool>& src, UoMap<String, String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, bool>& elem)->Pair<String, String> {
			return { elem.first, (elem.second ? "true" : "false") };
			});
	};
	void RNAImpl::TypeConversion(const Map<String, String>& src, UoMap<String, String>& dest) {
		dest = UoMap<String, String>(src.begin(), src.end());
	};
#pragma endregion

#pragma region UNORDERED_MAP
	// XXX to UoMap : not supported
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const T1& src, UoMap<String, T2>& dest) { };
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Array<T1>& src, UoMap<String, T2>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const Array<String>& src, UoMap<String, T>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const String& src, UoMap<String, T>& dest) {};
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const Set<T1>& src, UoMap<String, T2>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const Set<String>& src, UoMap<String, T>& dest) { };

	// UoMap to Single
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const UoMap<String, T1>& src, T2& dest) {
		if (src.size() > 0)
			dest = (T2)src.begin()->second;
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, T>& src, String& dest) {
		dest = "[";
		for (auto elem : src)
			dest += "{\"key\":" + elem.first + ",\"value\":" + std::to_string(elem.second) + "},";
		if (dest.size() > 1)
			dest.back() = ']';
		else
			dest.push_back(']');
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, T& dest) {
		if (src.size() > 0)
		{
			T _retval;
			TypeConversion(src.begin()->second, _retval);
			dest = _retval;
		}
	};
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, String& dest) {
		dest = "[";
		for (auto elem : src)
			dest += "{\"key\":" + elem.first + ",\"value\":" + elem.second + "},";
		if (dest.size() > 1)
			dest.back() = ']';
		else
			dest.push_back(']');
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, T& dest) {
		if (src.size() > 0)
			dest = (T)(src.begin()->second ? 1 : 0);
	};
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, String& dest) {
		dest = "[";
		for (auto elem : src)
			dest += "{\"key\":" + elem.first + ",\"value\":" + (elem.second ? "true" : "false") + "},";
		if (dest.size() > 1)
			dest.back() = ']';
		else
			dest.push_back(']');
	};

	// UoMap to Set : not supported
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const UoMap<String, T1>& src, Set<T2>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, T>& src, Set<String>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, Set<T>& dest) { };
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, Set<T>& dest) { };
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, Set<String>& dest) { };
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, Set<String>& dest) { };

	// UoMap to Array
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const UoMap<String, T1>& src, Array<T2>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->T2 {
			return (T2)elem.second;
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, T>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->String {
			return String("[") + elem.first + "," + std::to_string(elem.second) + "]";
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, Array<T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [this](const auto& elem)->T {
			T _retval;
			this->TypeConversion(elem.second, _retval);
			return _retval;
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, Array<T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->T {
			return (T)(elem.second ? 1 : 0);
			});
	};
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->String {
			return String("[") + elem.first + "," + elem.second + "]";
			});
	};
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, Array<String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::back_inserter(dest), [](const auto& elem)->String {
			return String("[") + elem.first + "," + (elem.second == true ? "true" : "false") + "]";
			});
	};

	// UoMap to UoMap
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const UoMap<String, T1>& src, UoMap<String, T2>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.end()), [](const Pair<String, T1>& elem)->Pair<String, T2> {
			return { elem.first, (T2)elem.second };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, UoMap<String, T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.end()), [this](const Pair<String, String>& elem)->Pair<String, T> {
			T _retval;
			this->TypeConversion(elem.second, _retval);
			return { elem.first, _retval };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, T>& src, UoMap<String, String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.end()), [](const Pair<String, T>& elem)->Pair<String, String> {
			return { elem.first, std::to_string(elem.second) };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, UoMap<String, T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.end()), [](const Pair<String, bool>& elem)->Pair<String, T> {
			return { elem.first, (T)(elem.second ? 1 : 0) };
			});
	};
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, UoMap<String, String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.end()), [](const Pair<String, bool>& elem)->Pair<String, String> {
			return { elem.first, (elem.second ? "true" : "false") };
			});
	};
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, UoMap<String, String>& dest) {
		dest = src;
	};

	// UoMap to Map
	template<typename T1, typename T2>
	void RNAImpl::TypeConversion(const UoMap<String, T1>& src, Map<String, T2>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, T1>& elem)->Pair<String, T2> {
			return { elem.first, (T2)elem.second };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, Map<String, T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [this](const Pair<String, String>& elem)->Pair<String, T> {
			T _retval;
			this->TypeConversion(elem.second, _retval);
			return { elem.first, _retval };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, T>& src, Map<String, String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, T>& elem)->Pair<String, String> {
			return { elem.first, std::to_string(elem.second) };
			});
	};
	template<typename T>
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, Map<String, T>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, bool>& elem)->Pair<String, T> {
			return { elem.first, (T)(elem.second ? 1 : 0) };
			});
	};
	void RNAImpl::TypeConversion(const UoMap<String, bool>& src, Map<String, String>& dest) {
		std::transform(src.cbegin(), src.cend(), std::inserter(dest, dest.begin()), [](const Pair<String, bool>& elem)->Pair<String, String> {
			return { elem.first, (elem.second ? "true" : "false") };
			});
	};
	void RNAImpl::TypeConversion(const UoMap<String, String>& src, Map<String, String>& dest) {
		dest = Map<String,String>(src.begin(), src.end());
	};
#pragma endregion

template<typename T>
inline const char* RNAImpl::GetType(const T& value)
{
	return typeid(value).name();
}

template<typename T>
void RNAImpl::TRead(const String& name, T& value)
{
	DynaArray _stored_type;
	DynaArray _buf;
	size_t _pos = name.find_first_not_of(' ');
	String _name = "";
	if (_pos != String::npos)
		_name = name.substr(_pos);
	else
		_name = name;
	model()->Read(_name, _stored_type, _buf);
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
void RNAImpl::TWrite(const String& name, const T& value, bool internal_use)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	out(value);
	model()->Write(name, GetType(value), data, internal_use);
}


void RNAImpl::Read(const String& name, bool& value)
{
	TRead(name, value);
}

void RNAImpl::Write(const String& name, const bool& value, bool internal_use)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	int8_t _value = (value == true ? 1 : 0);
	out(_value);
	model()->Write(name, GetType(value), data, internal_use);
}

void RNAImpl::Write(const String& name, const char* value, bool internal_use)
{
	Write(name, (String)value, internal_use);
}

void RNAImpl::Read(const String& name, Array<bool>& value)
{
	TRead(name, value);
}

void RNAImpl::Write(const String& name, const Array<bool>& value, bool internal_use)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	Array<int8_t> _value;
	std::transform(value.cbegin(), value.cend(), std::back_inserter(_value), [](const bool& elem)->int8_t {
		return elem == true ? 1 : 0;
		});
	out(_value);
	model()->Write(name, GetType(value), data, internal_use);
}

void RNAImpl::Read(const String& name, Set<bool>& value)
{
	TRead(name, value);
}

void RNAImpl::Write(const String& name, const Set<bool>& value, bool internal_use)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	Set<int8_t> _value;
	std::transform(value.cbegin(), value.cend(), std::inserter(_value, _value.begin()), [](const bool& elem)->int8_t {
		return elem == true ? 1 : 0;
		});
	out(_value);
	model()->Write(name, GetType(value), data, internal_use);
}

void RNAImpl::Read(const String& name, Map<String, bool>& value)
{
	TRead(name, value);
}

void RNAImpl::Write(const String& name, const Map<String, bool>& value, bool internal_use)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	Map<String, int8_t> _value;
	std::transform(value.cbegin(), value.cend(), std::inserter(_value, _value.begin()), [](const Pair<String, bool>& elem)->auto {
		return std::make_pair(elem.first, (elem.second == true ? 1 : 0));
	});
	out(_value);
	model()->Write(name, GetType(value), data, internal_use);
}

void RNAImpl::Read(const String& name, UoMap<String, bool>& value)
{
	TRead(name, value);
}

void RNAImpl::Write(const String& name, const UoMap<String, bool>& value, bool internal_use)
{
	ByteArray data;
	zpp::serializer::memory_output_archive out(data);
	UoMap<String, int8_t> _value;
	std::transform(value.cbegin(), value.cend(), std::inserter(_value, _value.begin()), [](const Pair<String, bool>& elem)->auto {
		return std::make_pair(elem.first, (elem.second == true ? 1 : 0));
	});
	out(_value);
	model()->Write(name, GetType(value), data, internal_use);
}

template <typename T>
void RNAImpl::Deserialize(const char* type, const DynaArray& data, T& value)
{
	size_t _type_hash = hash((String)type);
	if (type == nullptr || type[0] == '\0')
		TypeConversion(String(""), value);
	else if (type_hash_table_[_type_hash] == type_bool)
		Deserialize(bool(), data, value);
	else
	{
		if (_type_hash == hash((String)GetType(value)))
		{
			ByteArray _data(data.size());
			memcpy(_data.data(), data.data(), data.size());
			zpp::serializer::memory_input_archive in(_data);
			in(value);
		}
		else
		{
			switch (type_hash_table_[_type_hash])
			{
			case type_int32_t:
				Deserialize(int32_t(), data, value);
				break;
			case type_uint32_t:
				Deserialize(uint32_t(), data, value);
				break;
			case type_int64_t:
				Deserialize(int64_t(), data, value);
				break;
			case type_uint64_t:
				Deserialize(uint64_t(), data, value);
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
			case type_int64_t_array:
				Deserialize(Array<int64_t>(), data, value);
				break;
			case type_uint64_t_array:
				Deserialize(Array<uint64_t>(), data, value);
				break;
			case type_double_array:
				Deserialize(Array<double>(), data, value);
				break;
			case type_string_array:
				Deserialize(Array<String>(), data, value);
				break;

			case type_bool_set:
				Deserialize(Set<bool>(), data, value);
				break;
			case type_int32_t_set:
				Deserialize(Set<int32_t>(), data, value);
				break;
			case type_uint32_t_set:
				Deserialize(Set<uint32_t>(), data, value);
				break;
			case type_int64_t_set:
				Deserialize(Set<int64_t>(), data, value);
				break;
			case type_uint64_t_set:
				Deserialize(Set<uint64_t>(), data, value);
				break;
			case type_double_set:
				Deserialize(Set<double>(), data, value);
				break;
			case type_string_set:
				Deserialize(Set<String>(), data, value);
				break;

			case type_bool_map:
				Deserialize(Map<String, bool>(), data, value);
				break;
			case type_int32_t_map:
				Deserialize(Map<String, int32_t>(), data, value);
				break;
			case type_uint32_t_map:
				Deserialize(Map<String, uint32_t>(), data, value);
				break;
			case type_int64_t_map:
				Deserialize(Map<String, int64_t>(), data, value);
				break;
			case type_uint64_t_map:
				Deserialize(Map<String, uint64_t>(), data, value);
				break;
			case type_double_map:
				Deserialize(Map<String, double>(), data, value);
				break;
			case type_string_map:
				Deserialize(Map<String, String>(), data, value);
				break;

			case type_bool_uomap:
				Deserialize(UoMap<String, bool>(), data, value);
				break;
			case type_int32_t_uomap:
				Deserialize(UoMap<String, int32_t>(), data, value);
				break;
			case type_uint32_t_uomap:
				Deserialize(UoMap<String, uint32_t>(), data, value);
				break;
			case type_int64_t_uomap:
				Deserialize(UoMap<String, int64_t>(), data, value);
				break;
			case type_uint64_t_uomap:
				Deserialize(UoMap<String, uint64_t>(), data, value);
				break;
			case type_double_uomap:
				Deserialize(UoMap<String, double>(), data, value);
				break;
			case type_string_uomap:
				Deserialize(UoMap<String, String>(), data, value);
				break;
			default:
				std::cout << "Not supported type" << std::endl;
				break;
			}
		}
	}
}

void RNAImpl::Deserialize(const char* type, const DynaArray& data, Array<bool>& value)
{
	size_t _type_hash = hash((String)type);
	if (_type_hash == hash((String)GetType(value)))
	{
		ByteArray _data(data.size());
		memcpy(_data.data(), data.data(), data.size());
		zpp::serializer::memory_input_archive in(_data);
		Array<int8_t> _value;
		in(_value);

		std::transform(_value.cbegin(), _value.cend(), std::back_inserter(value), [](const int8_t& elem)->bool {
			return elem == 1;
			});
	}
	else
	{
		switch (type_hash_table_[_type_hash])
		{
		case type_int32_t:
			Deserialize(int32_t(), data, value);
			break;
		case type_uint32_t:
			Deserialize(uint32_t(), data, value);
			break;
		case type_int64_t:
			Deserialize(int64_t(), data, value);
			break;
		case type_uint64_t:
			Deserialize(uint64_t(), data, value);
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
		case type_int64_t_array:
			Deserialize(Array<int64_t>(), data, value);
			break;
		case type_uint64_t_array:
			Deserialize(Array<uint64_t>(), data, value);
			break;
		case type_double_array:
			Deserialize(Array<double>(), data, value);
			break;
		case type_string_array:
			Deserialize(Array<String>(), data, value);
			break;

		case type_bool_set:
			Deserialize(Set<bool>(), data, value);
			break;
		case type_int32_t_set:
			Deserialize(Set<int32_t>(), data, value);
			break;
		case type_uint32_t_set:
			Deserialize(Set<uint32_t>(), data, value);
			break;
		case type_int64_t_set:
			Deserialize(Set<int64_t>(), data, value);
			break;
		case type_uint64_t_set:
			Deserialize(Set<uint64_t>(), data, value);
			break;
		case type_double_set:
			Deserialize(Set<double>(), data, value);
			break;
		case type_string_set:
			Deserialize(Set<String>(), data, value);
			break;

		case type_bool_map:
			Deserialize(Map<String, bool>(), data, value);
			break;
		case type_int32_t_map:
			Deserialize(Map<String, int32_t>(), data, value);
			break;
		case type_uint32_t_map:
			Deserialize(Map<String, uint32_t>(), data, value);
			break;
		case type_int64_t_map:
			Deserialize(Map<String, int64_t>(), data, value);
			break;
		case type_uint64_t_map:
			Deserialize(Map<String, uint64_t>(), data, value);
			break;
		case type_double_map:
			Deserialize(Map<String, double>(), data, value);
			break;
		case type_string_map:
			Deserialize(Map<String, String>(), data, value);
			break;

		case type_bool_uomap:
			Deserialize(UoMap<String, bool>(), data, value);
			break;
		case type_int32_t_uomap:
			Deserialize(UoMap<String, int32_t>(), data, value);
			break;
		case type_uint32_t_uomap:
			Deserialize(UoMap<String, uint32_t>(), data, value);
			break;
		case type_int64_t_uomap:
			Deserialize(UoMap<String, int64_t>(), data, value);
			break;
		case type_uint64_t_uomap:
			Deserialize(UoMap<String, uint64_t>(), data, value);
			break;
		case type_double_uomap:
			Deserialize(UoMap<String, double>(), data, value);
			break;
		case type_string_uomap:
			Deserialize(UoMap<String, String>(), data, value);
			break;
		default:
			std::cout << "Not supported type" << std::endl;
			break;
		}
	}
}

template <typename T1, typename T2>
void RNAImpl::Deserialize(T1, const DynaArray& data, T2& value)
{
	ByteArray _data(data.size());
	memcpy(_data.data(), data.data(), data.size());
	zpp::serializer::memory_input_archive in(_data);

	T1 _value;
	in(_value);
	TypeConversion(_value, value);
}

template <typename T2>
void RNAImpl::Deserialize(bool, const DynaArray& data, T2& value)
{
	ByteArray _data(data.size());
	memcpy(_data.data(), data.data(), data.size());
	zpp::serializer::memory_input_archive in(_data);

	int8_t _value;
	in(_value);
	TypeConversion(_value == 1, value);
}

template <typename T2>
void RNAImpl::Deserialize(Array<bool>, const DynaArray& data, T2& value)
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

void RNAImpl::BuildTypeHastable()
{
	type_hash_table_ =
	{
		{ hash((String)typeid(bool).name()), type_bool },
		{ hash((String)typeid(int32_t).name()), type_int32_t },
		{ hash((String)typeid(uint32_t).name()), type_uint32_t},
		{ hash((String)typeid(int64_t).name()), type_int64_t },
		{ hash((String)typeid(uint64_t).name()), type_uint64_t },
		{ hash((String)typeid(double).name()), type_double },
		{ hash((String)typeid(String).name()), type_string },

		{ hash((String)typeid(Array<bool>).name()), type_bool_array },
		{ hash((String)typeid(Array<int32_t>).name()), type_int32_t_array },
		{ hash((String)typeid(Array<uint32_t>).name()), type_uint32_t_array },
		{ hash((String)typeid(Array<int64_t>).name()), type_int64_t_array },
		{ hash((String)typeid(Array<uint64_t>).name()), type_uint64_t_array },
		{ hash((String)typeid(Array<double>).name()), type_double_array },
		{ hash((String)typeid(Array<String>).name()), type_string_array },

		{ hash((String)typeid(Set<bool>).name()), type_bool_set },
		{ hash((String)typeid(Set<int32_t>).name()), type_int32_t_set },
		{ hash((String)typeid(Set<uint32_t>).name()), type_uint32_t_set },
		{ hash((String)typeid(Set<int64_t>).name()), type_int64_t_set },
		{ hash((String)typeid(Set<uint64_t>).name()), type_uint64_t_set },
		{ hash((String)typeid(Set<double>).name()), type_double_set },
		{ hash((String)typeid(Set<String>).name()), type_string_set },

		{ hash((String)typeid(Map<String, bool>).name()), type_bool_map },
		{ hash((String)typeid(Map<String, int32_t>).name()), type_int32_t_map },
		{ hash((String)typeid(Map<String, uint32_t>).name()), type_uint32_t_map },
		{ hash((String)typeid(Map<String, int64_t>).name()), type_int64_t_map },
		{ hash((String)typeid(Map<String, uint64_t>).name()), type_uint64_t_map },
		{ hash((String)typeid(Map<String, double>).name()), type_double_map },
		{ hash((String)typeid(Map<String, String>).name()), type_string_map },

		{ hash((String)typeid(UoMap<String, bool>).name()), type_bool_uomap },
		{ hash((String)typeid(UoMap<String, int32_t>).name()), type_int32_t_uomap },
		{ hash((String)typeid(UoMap<String, uint32_t>).name()), type_uint32_t_uomap },
		{ hash((String)typeid(UoMap<String, int64_t>).name()), type_int64_t_uomap },
		{ hash((String)typeid(UoMap<String, uint64_t>).name()), type_uint64_t_uomap },
		{ hash((String)typeid(UoMap<String, double>).name()), type_double_uomap },
		{ hash((String)typeid(UoMap<String, String>).name()), type_string_uomap },
	};
}
