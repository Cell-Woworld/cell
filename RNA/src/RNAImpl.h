#pragma once
#include "RNA.h"
#include "IBiomolecule.h"
#include <regex>

USING_BIO_NAMESPACE

#define READ_FUNC	{ TRead(name, value); }
#define WRITE_FUNC	{ TWrite(name, value, internal_use); }


class DynaArray;
class RNAImpl : public IRNAImpl, public IBiomolecule
{
	class strutil
	{
	public:
		template <typename ITR>
		static inline void SplitStringToIteratorUsing(const String& full, const char* delim, ITR& result)
		{
			// Optimize the common case where delim is a single character.
			if (delim[0] != '\0' && delim[1] == '\0') {
				char c = delim[0];
				const char* p = full.data();
				const char* end = p + full.size();
				while (p != end) {
					if (*p == c) {
						++p;
					}
					else {
						const char* start = p;
						while (++p != end && *p != c);
						*result++ = String(start, p - start);
					}
				}
				return;
			}

			String::size_type begin_index, end_index;
			begin_index = full.find_first_not_of(delim);
			while (begin_index != String::npos) {
				end_index = full.find_first_of(delim, begin_index);
				if (end_index == String::npos) {
					*result++ = full.substr(begin_index);
					return;
				}
				*result++ = full.substr(begin_index, (end_index - begin_index));
				begin_index = full.find_first_not_of(delim, end_index);
			}
		}

		static inline void SplitStringUsing(const String& full,
			const char* delim,
			Array<String>* result) {
			std::back_insert_iterator< Array<String> > it(*result);
			SplitStringToIteratorUsing(full, delim, it);
		}

		template <typename StringType, typename ITR>
		static inline void SplitStringToIteratorAllowEmpty(const StringType& full, const char* delim, int pieces, ITR& result)
		{
			String::size_type begin_index, end_index;
			begin_index = 0;

			for (int i = 0; (i < pieces - 1) || (pieces == 0); i++) {
				end_index = full.find_first_of(delim, begin_index);
				if (end_index == String::npos) {
					*result++ = full.substr(begin_index);
					return;
				}
				*result++ = full.substr(begin_index, (end_index - begin_index));
				begin_index = end_index + 1;
			}
			*result++ = full.substr(begin_index);
		}

		static inline void SplitStringAllowEmpty(const String& full, const char* delim, Array<String>* result)
		{
			std::back_insert_iterator<Array<String> > it(*result);
			SplitStringToIteratorAllowEmpty(full, delim, 0, it);
		}
	};
public:
	RNAImpl(IBiomolecule* owner, const char* name, RNA* callback)
		: IBiomolecule(owner)
		, callback_(callback) 
	{
		init(name);
		BuildTypeHastable();
	};
	virtual ~RNAImpl() {
		owner()->unbind(this);
	};

	void Remove(const String& name, bool internal_only = false);
	virtual void Clone(const String& target_name, const String& src_name);
	virtual void SendEvent(const String& name, const String& payload = "");
	virtual void OnEvent(const DynaArray& name);
	virtual void Bind(void* desc_pool);
	virtual const char* name();
	virtual const char* version() { return get_version(); };
	virtual const Obj<IModel> model();
	virtual void activate();
	virtual void add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src);
	virtual void add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src);
	virtual void on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload);
	virtual void do_event(const DynaArray& msg_name);
	virtual const char* get_root_path() { return owner()->get_root_path(); };
	virtual const char* get_version();

	DECLARE_EXPORT_RW_API(int32_t, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(uint32_t, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(int64_t, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(uint64_t, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(double, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(String, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Array<int32_t>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Array<uint32_t>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Array<int64_t>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Array<uint64_t>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Array<double>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Array<String>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Map_String_int32, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Map_String_uint32, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Map_String_int64, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Map_String_uint64, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Map_String_double, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Map_String_String, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(UoMap_String_int32, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(UoMap_String_uint32, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(UoMap_String_int64, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(UoMap_String_uint64, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(UoMap_String_double, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(UoMap_String_String, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Set<int32_t>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Set<uint32_t>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Set<int64_t>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Set<uint64_t>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Set<double>, READ_FUNC, WRITE_FUNC)
	DECLARE_EXPORT_RW_API(Set<String>, READ_FUNC, WRITE_FUNC)

	DECLARE_EXPORT_RW_API(bool, , )
	DECLARE_EXPORT_RW_API(Array<bool>, , )
	DECLARE_EXPORT_RW_API(Map_String_bool, , )
	DECLARE_EXPORT_RW_API(UoMap_String_bool, , )
	DECLARE_EXPORT_RW_API(Set<bool>, , )

	virtual void Write(const String& name, const char* value, bool internal_use = false);
private:
	//bool is_digital(const String& s) {
	//	return !s.empty() && std::find_if(s.begin(),
	//		s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
	//};
	bool is_number(const String& token) {
		return std::regex_match(token, std::regex(("((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?")));
	}
	RNA* callback_;

	Map<size_t, int> type_hash_table_;
	void BuildTypeHastable();

private:
	template <typename T>
	void TRead(const String& name, T& value);
	template <typename T>
	void TWrite(const String& name, const T& value, bool internal_use = false);
	template <typename T>
	const char* GetType(const T& value);
	template <typename T>
	void Deserialize(const char* type, const DynaArray& data, T& value);
	void Deserialize(const char* type, const DynaArray& data, Array<bool>& value);
	template <typename T1, typename T2>
	void Deserialize(T1, const DynaArray& data, T2& value);
	template <typename T2>
	void Deserialize(bool, const DynaArray& data, T2& value);
	template <typename T2>
	void Deserialize(Array<bool>, const DynaArray& data, T2& value);

#pragma region SINGLE
	// Single to Single
	template<typename T1, typename T2> void TypeConversion(const T1& src, T2& dest);
	template<typename T> void TypeConversion(const T& src, String& dest);
	template<typename T> void TypeConversion(const String& src, T& dest);
	void TypeConversion(const bool& src, String& dest);
	void TypeConversion(const String& src, bool& dest);
	void TypeConversion(const String& src, String& dest);
#pragma endregion SINGLE

#pragma region ARRAY
	// Array to Single
	template<typename T1, typename T2> void TypeConversion(const Array<T1>& src, T2& dest);
	template<typename T> void TypeConversion(const Array<T>& src, String& dest);
	template<typename T> void TypeConversion(const Array<String>& src, T& dest);
	void TypeConversion(const Array<String>& src, String& dest);
	void TypeConversion(const Array<bool>& src, String& dest);

	// Array to Array
	template<typename T1, typename T2> void TypeConversion(const Array<T1>& src, Array<T2>& dest);
	template<typename T> void TypeConversion(const Array<T>& src, Array<String>& dest);
	template<typename T> void TypeConversion(const Array<String>& src, Array<T>& dest);
	void TypeConversion(const Array<String>& src, Array<String>& dest);
	void TypeConversion(const Array<bool>& src, Array<String>& dest);

	// Single to Array
	template<typename T1, typename T2> void TypeConversion(const T1& src, Array<T2>& dest);
	template<typename T> void TypeConversion(const T& src, Array<String>& dest);
	template<typename T> void TypeConversion(const String& src, Array<T>& dest);
	void TypeConversion(const bool& src, Array<String>& dest);
	void TypeConversion(const String& src, Array<String>& dest);

	// Set to Array
	template<typename T1, typename T2> void TypeConversion(const Set<T1>& src, Array<T2>& dest);
	template<typename T> void TypeConversion(const Set<T>& src, Array<String>& dest);
	template<typename T> void TypeConversion(const Set<String>& src, Array<T>& dest);
	void TypeConversion(const Set<String>& src, Array<String>& dest);
	void TypeConversion(const Set<bool>& src, Array<String>& dest);
#pragma endregion ARRAY

#pragma region SET
	// Single to Set
	template<typename T1, typename T2> void TypeConversion(const T1& src, Set<T2>& dest);
	template<typename T> void TypeConversion(const T& src, Set<String>& dest);
	template<typename T> void TypeConversion(const String& src, Set<T>& dest);
	void TypeConversion(const bool& src, Set<String>& dest);
	void TypeConversion(const String& src, Set<String>& dest);

	// Set to Single
	template<typename T1, typename T2> void TypeConversion(const Set<T1>& src, T2& dest);
	template<typename T> void TypeConversion(const Set<T>& src, String& dest);
	template<typename T> void TypeConversion(const Set<String>& src, T& dest);
	void TypeConversion(const Set<String>& src, String& dest);
	void TypeConversion(const Set<bool>& src, String& dest);

	// Array to Set
	template<typename T1, typename T2> void TypeConversion(const Array<T1>& src, Set<T2>& dest);
	template<typename T> void TypeConversion(const Array<T>& src, Set<String>& dest);
	template<typename T> void TypeConversion(const Array<String>& src, Set<T>& dest);
	void TypeConversion(const Array<String>& src, Set<String>& dest);

	// Set to Set
	template<typename T1, typename T2> void TypeConversion(const Set<T1>& src, Set<T2>& dest);
	template<typename T> void TypeConversion(const Set<T>& src, Set<String>& dest);
	template<typename T> void TypeConversion(const Set<String>& src, Set<T>& dest);
	void TypeConversion(const Set<String>& src, Set<String>& dest);
	void TypeConversion(const Set<bool>& src, Set<String>& dest);
#pragma endregion SET

#pragma region MAP
	// XXX to Map : not supported
	template<typename T1, typename T2> void TypeConversion(const T1& src, Map<String, T2>& dest);
	template<typename T1, typename T2> void TypeConversion(const Array<T1>& src, Map<String, T2>& dest);
	template<typename T> void TypeConversion(const Array<String>& src, Map<String, T>& dest);
	template<typename T> void TypeConversion(const String& src, Map<String, T>& dest);
	template<typename T1, typename T2> void TypeConversion(const Set<T1>& src, Map<String, T2>& dest);
	template<typename T> void TypeConversion(const Set<String>& src, Map<String, T>& dest);

	// Map to Single
	template<typename T1, typename T2> void TypeConversion(const Map<String, T1>& src, T2& dest);
	template<typename T> void TypeConversion(const Map<String, T>& src, String& dest);
	template<typename T> void TypeConversion(const Map<String, String>& src, T& dest);
	template<typename T> void TypeConversion(const Map<String, bool>& src, T& dest);
	void TypeConversion(const Map<String, String>& src, String& dest);
	void TypeConversion(const Map<String, bool>& src, String& dest);

	// Map to Set : not supported
	template<typename T1, typename T2> void TypeConversion(const Map<String, T1>& src, Set<T2>& dest);
	template<typename T> void TypeConversion(const Map<String, T>& src, Set<String>& dest);
	template<typename T> void TypeConversion(const Map<String, String>& src, Set<T>& dest);
	template<typename T> void TypeConversion(const Map<String, bool>& src, Set<T>& dest);
	void TypeConversion(const Map<String, bool>& src, Set<String>& dest);
	void TypeConversion(const Map<String, String>& src, Set<String>& dest);

	// Map to Array
	template<typename T1, typename T2> void TypeConversion(const Map<String, T1>& src, Array<T2>& dest);
	template<typename T> void TypeConversion(const Map<String, T>& src, Array<String>& dest);
	template<typename T> void TypeConversion(const Map<String, String>& src, Array<T>& dest);
	template<typename T> void TypeConversion(const Map<String, bool>& src, Array<T>& dest);
	void TypeConversion(const Map<String, String>& src, Array<String>& dest);
	void TypeConversion(const Map<String, bool>& src, Array<String>& dest);

	// Map to Map
	template<typename T1, typename T2> void TypeConversion(const Map<String, T1>& src, Map<String, T2>& dest);
	template<typename T> void TypeConversion(const Map<String, String>& src, Map<String, T>& dest);
	template<typename T> void TypeConversion(const Map<String, T>& src, Map<String, String>& dest);
	template<typename T> void TypeConversion(const Map<String, bool>& src, Map<String, T>& dest);
	void TypeConversion(const Map<String, bool>& src, Map<String, String>& dest);
	void TypeConversion(const Map<String, String>& src, Map<String, String>& dest);

	// Map to UoMap
	template<typename T1, typename T2> void TypeConversion(const Map<String, T1>& src, UoMap<String, T2>& dest);
	template<typename T> void TypeConversion(const Map<String, String>& src, UoMap<String, T>& dest);
	template<typename T> void TypeConversion(const Map<String, T>& src, UoMap<String, String>& dest);
	template<typename T> void TypeConversion(const Map<String, bool>& src, UoMap<String, T>& dest);
	void TypeConversion(const Map<String, bool>& src, UoMap<String, String>& dest);
	void TypeConversion(const Map<String, String>& src, UoMap<String, String>& dest);
#pragma endregion

#pragma region UNORDERED_MAP
	// XXX to UoMap : not supported
	template<typename T1, typename T2> void TypeConversion(const T1& src, UoMap<String, T2>& dest);
	template<typename T1, typename T2> void TypeConversion(const Array<T1>& src, UoMap<String, T2>& dest);
	template<typename T> void TypeConversion(const Array<String>& src, UoMap<String, T>& dest);
	template<typename T> void TypeConversion(const String& src, UoMap<String, T>& dest);
	template<typename T1, typename T2> void TypeConversion(const Set<T1>& src, UoMap<String, T2>& dest);
	template<typename T> void TypeConversion(const Set<String>& src, UoMap<String, T>& dest);

	// UoMap to Single
	template<typename T1, typename T2> void TypeConversion(const UoMap<String, T1>& src, T2& dest);
	template<typename T> void TypeConversion(const UoMap<String, T>& src, String& dest);
	template<typename T> void TypeConversion(const UoMap<String, String>& src, T& dest);
	template<typename T> void TypeConversion(const UoMap<String, bool>& src, T& dest);
	void TypeConversion(const UoMap<String, String>& src, String& dest);
	void TypeConversion(const UoMap<String, bool>& src, String& dest);

	// UoMap to Set : not supported
	template<typename T1, typename T2> void TypeConversion(const UoMap<String, T1>& src, Set<T2>& dest);
	template<typename T> void TypeConversion(const UoMap<String, T>& src, Set<String>& dest);
	template<typename T> void TypeConversion(const UoMap<String, String>& src, Set<T>& dest);
	template<typename T> void TypeConversion(const UoMap<String, bool>& src, Set<T>& dest);
	void TypeConversion(const UoMap<String, bool>& src, Set<String>& dest);
	void TypeConversion(const UoMap<String, String>& src, Set<String>& dest);

	// UoMap to Array
	template<typename T1, typename T2> void TypeConversion(const UoMap<String, T1>& src, Array<T2>& dest);
	template<typename T> void TypeConversion(const UoMap<String, T>& src, Array<String>& dest);
	template<typename T> void TypeConversion(const UoMap<String, String>& src, Array<T>& dest);
	template<typename T> void TypeConversion(const UoMap<String, bool>& src, Array<T>& dest);
	void TypeConversion(const UoMap<String, String>& src, Array<String>& dest);
	void TypeConversion(const UoMap<String, bool>& src, Array<String>& dest);

	// UoMap to UoMap
	template<typename T1, typename T2> void TypeConversion(const UoMap<String, T1>& src, UoMap<String, T2>& dest);
	template<typename T> void TypeConversion(const UoMap<String, String>& src, UoMap<String, T>& dest);
	template<typename T> void TypeConversion(const UoMap<String, T>& src, UoMap<String, String>& dest);
	template<typename T> void TypeConversion(const UoMap<String, bool>& src, UoMap<String, T>& dest);
	void TypeConversion(const UoMap<String, bool>& src, UoMap<String, String>& dest);
	void TypeConversion(const UoMap<String, String>& src, UoMap<String, String>& dest);

	// UoMap to Map
	template<typename T1, typename T2> void TypeConversion(const UoMap<String, T1>& src, Map<String, T2>& dest);
	template<typename T> void TypeConversion(const UoMap<String, String>& src, Map<String, T>& dest);
	template<typename T> void TypeConversion(const UoMap<String, T>& src, Map<String, String>& dest);
	template<typename T> void TypeConversion(const UoMap<String, bool>& src, Map<String, T>& dest);
	void TypeConversion(const UoMap<String, bool>& src, Map<String, String>& dest);
	void TypeConversion(const UoMap<String, String>& src, Map<String, String>& dest);
#pragma endregion
	Array<String> split(const String& full, const char* delim, bool skip_empty = true)
	{
		Array<String> result;
		if (skip_empty) {
			strutil::SplitStringUsing(full, delim, &result);
		}
		else {
			strutil::SplitStringAllowEmpty(full, delim, &result);
		}
		return result;
	}
};

#include "RNAImpl.hpp"