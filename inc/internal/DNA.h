#pragma once
#include "IBiomolecule.h"
#include "internal/IStateMachine.h"
#include "internal/utils/nlohmann/json.hpp"
#include <regex>

using json = nlohmann::json;

class ConditionEval;
class ActionEval;

BIO_BEGIN_NAMESPACE

class DNA : public IBiomolecule
{
	friend class ConditionEval;
	friend class ActionEval;
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
	PUBLIC_API DNA(IBiomolecule* owner);
	PUBLIC_API virtual ~DNA();

public:
	virtual void init(const char* name);
	virtual const Obj<IModel> model() {
		return owner()->model();
	};
	virtual void add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src);
	virtual void add_priority_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src);
	virtual void on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload);
	virtual void do_event(const DynaArray& msg_name);
	virtual const char* get_root_path() { return owner()->get_root_path(); };
	virtual const char* get_version();

public:
	void on_action(int type, const char* name, const Array<Pair<String,String>>& params);
	bool on_condition(const char* name, const Array<Pair<String, String>>& params);
	bool get_content(const char* name, String& content);
	template<typename T>
	void Read(const String& name, T& value);
	void Read(const String& name, bool& value);
	template<typename T>
	void Write(const String& name, const T& value);

private:
	void WriteParams(const String& name, const Array<Pair<String, String>>& params);
	void RemoveParams(const String& name, const Array<Pair<String, String>>& params);
	//bool is_digital(const String& s) {
	//	return !s.empty() && std::find_if(s.begin(),
	//		s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
	//};
	bool is_number(const String& token) {
		return std::regex_match(token, std::regex(("((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?")));
	}
	template<typename T>
	const char* GetType(const T& value);
	template<typename T>
	void Deserialize(const char* type, const DynaArray& data, T& value);
	template<typename T1, typename T2>
	void Deserialize(T1, const DynaArray& data, T2& value);
	template <typename T2>
	void Deserialize(bool, const DynaArray& data, T2& value);
	template <typename T2>
	void Deserialize(Array<bool>, const DynaArray& data, T2& value);

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
	void TypeConversion(const String& src, String& dest);
	void TypeConversion(const String& src, bool& dest);
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

	void Deserialize(const char* type, const DynaArray& data, bool& value);
	void Clone(const String& target_name, const String& src_name, const String& default_value = "");
	void Remove(const String& name);
	void Pushback(const String& target_name, const String& src_name, const String& default_value = "");
	void BuildTypeHastable();
	void SaveVersion();
	void ReplaceModelNameWithValue(String& target, const String& resolved_model_name = "");
	bool findAndReplaceAll(String& data, const String& toSearch, const String& replaceStr);
	void deepAssign(const String& map_key, const String& map_value, size_t left_bracket_pos);
	void ReplacePathIndex(String& path_str);
	bool FindAndUnserialize(json& root, const String& path_str, json** obj, String& key);
	void RemoveElement(json& root, const String& path_str, bool forever = false);
	size_t FindRightBracket(const String& target, size_t pos, const Pair<char,char>& bracket);

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
	void get_state_list(Array<String>& state_list);
	void set_state_list(const Array<String>& state_list);

private:
	static bool discard_no_such_model_warning_;

private:
	IStateMachine state_machine_;
	Map<size_t, int> type_hash_table_;
	std::hash<String> hash_;
	Obj<ConditionEval> cond_eval_;
	Obj<ActionEval> action_eval_;
};

typedef DNA* (*CREATE_DNA_INSTANCE_FUNCTION)(IBiomolecule*);

BIO_END_NAMESPACE