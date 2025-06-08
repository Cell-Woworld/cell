#pragma once
#include "IBiomolecule.h"
#include "internal/IStateMachine.h"
#include "nlohmann/json.hpp"
#include <regex>

using json = nlohmann::json;

class ConditionEval;
class ActionEval;

BIO_BEGIN_NAMESPACE

class DNA : public IBiomolecule
{
	friend class ConditionEval;
	friend class ActionEval;
public:
	PUBLIC_API DNA(IBiomolecule* owner);
	PUBLIC_API virtual ~DNA();

public:
	virtual bool init(const char* name, bool isFile = true);
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
	void Read(const String& name, T& value, bool auto_conversion = true);
	void Read(const String& name, bool& value, bool auto_conversion = true);
	void Read(const String& name, nlohmann::json& value, bool auto_conversion = true);
	template<typename T>
	void Write(const String& name, const T& value);
	void Write(const String& name, const nlohmann::json& value);
	void ReplacePathIndex(String& path_str);
	void WriteParams(const String& name, const Array<Pair<String, String>>& params);

private:
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
	template <typename T2>
	void Deserialize(json, const DynaArray& data, T2& value);

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
	void TypeConversion(const double& src, String& dest);
	void TypeConversion(const double& src, Array<String>& dest);

	void Deserialize(const char* type, const DynaArray& data, bool& value);
	void Clone(const String& target_name, const String& src_name, const String& default_value = "");
	void Remove(const String& name);
	void Pushback(const String& target_name, const String& src_name, const String& default_value = "");
	void PushbackValue(const String& target_name, const String& value);
	void BuildTypeHastable();
	void SaveVersion();
	//void ReplaceModelNameWithValue(String& target, const String& resolved_model_name = "");
	void ReplaceModelNameWithValue(String& target, size_t target_start = 0, size_t target_end = String::npos, const String& resolved_model_name = "", bool recursive = true);
	int findAndReplaceAll(String& data, const String& toSearch, const String& replaceStr, size_t target_start = 0, size_t target_end = String::npos);
	void deepAssign(const String& map_key, const String& map_value, size_t left_bracket_pos);
	bool FindAndUnserialize(json& root, const String& path_str, json** obj, String& key);
	nlohmann::json RemoveElement(json& root, const String& path_str, bool forever = false);
	size_t FindRightBracket(const String& target, size_t pos, const Pair<char,char>& bracket);
	void CorrectNegativeIndex(const String& target, const nlohmann::json& root, const String& path_str, nlohmann::json::json_pointer& new_value_ptr);

	void get_state_list(Array<String>& state_list);
	void set_state_list(const Array<String>& state_list);
	template <typename T>
	String to_string_with_precision(const T a_value, const int n = 6);

private:
	static bool discard_no_such_model_warning_;
	static int MAX_REGEXPR_SIZE;

private:
	IStateMachine state_machine_;
	Map<size_t, int> type_hash_table_;
	std::hash<String> hash_;
	Obj<ConditionEval> cond_eval_;
	Obj<ActionEval> action_eval_;
	Map<String, String> resolved_model_cache_;
	Map<String, nlohmann::json> for_each_cache_;
};

typedef DNA* (*CREATE_DNA_INSTANCE_FUNCTION)(IBiomolecule*);

BIO_END_NAMESPACE