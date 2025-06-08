#pragma once
#include "IBiomolecule.h"
#include "RNA.h"
#include <regex>

namespace google {
	namespace protobuf {
		class DescriptorDatabase;
		class DescriptorPool;
		class Message;
		class FieldDescriptor;
		class MergedDescriptorDatabase;
		class DynamicMessageFactory;
	}
}

BIO_BEGIN_NAMESPACE

class mRNA : public RNA
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
	PUBLIC_API mRNA(IBiomolecule* owner);
	PUBLIC_API virtual ~mRNA();

public:
	PUBLIC_API virtual void bind(IBiomolecule* src, const char* filename);
	PUBLIC_API virtual void unbind(IBiomolecule* src);
	PUBLIC_API virtual bool pack(const DynaArray& root_path, const DynaArray& name, DynaArray& payload);
	PUBLIC_API virtual bool unpack(const DynaArray& name, const DynaArray& payload, const DynaArray& field_name = "");
	PUBLIC_API virtual bool decompose(const DynaArray& name, const DynaArray& payload, const DynaArray& field_name);
	PUBLIC_API virtual void assign(const DynaArray& name);

private:
	bool fill_model_by_field_name(const String& name, const String& content);
	//bool is_digital(const String& s) {
	//	return !s.empty() && std::find_if(s.begin(),
	//		s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
	//};
	bool is_number(const String& token) {
		return std::regex_match(token, std::regex(("((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?")));
	}
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
	};

	Array<String> SplitByString(const String& input, const String& separator);

	bool packFromPacket(const String& root_path, Obj<google::protobuf::Message>& message, Set<String>& bin_field_set);
	bool packFromColumnsSource(const String& name, Obj<google::protobuf::Message>& message, Array<String>& columns_source);
	void RetrieveBinaryData(String& target, const String& name, Set<String>& bin_field_set);
	void assignValue(google::protobuf::Message* message, google::protobuf::DynamicMessageFactory& message_factory,
		const google::protobuf::FieldDescriptor* _field_desc, const Array<String>& path, const String& value);
	void WriteModelByField(const google::protobuf::FieldDescriptor* _field_desc, const google::protobuf::Reflection* _reflection, const google::protobuf::Message* _message, const String& _model_name);
	void WriteJSONByField(const google::protobuf::FieldDescriptor* _field_desc, const google::protobuf::Reflection* _reflection, const google::protobuf::Message* _message, nlohmann::json& _target);
	template <typename T>
	void pack_field(std::function<void(google::protobuf::Message*, const google::protobuf::FieldDescriptor*, T)> func1, std::function<void(google::protobuf::Message*, const google::protobuf::FieldDescriptor*, T)> func2, const google::protobuf::FieldDescriptor* field_desc, google::protobuf::Message* message, const String& model_name);
	template <typename T>
	void unpack_field(std::function<T(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor*, int)> func1, std::function<T(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor*)> func2, const google::protobuf::FieldDescriptor* field_desc, const google::protobuf::Reflection* reflection, const google::protobuf::Message* message, const String& model_name);
	void SplitPathName(const String& full_filename, String& path, String& filename);

	void SaveVersion();
	const google::protobuf::Descriptor* FindMessageTypeByName(const String& name);
	const google::protobuf::EnumValueDescriptor* FindValueByName(const String& msg_name, int index, const String& name);
private:
	Array<google::protobuf::DescriptorDatabase*> databases_;
	Array<IBiomolecule*> databases_src_;
	Obj<google::protobuf::MergedDescriptorDatabase> merged_database_;
	Obj<google::protobuf::DescriptorPool> desc_pool_;
	Array<Obj<google::protobuf::DescriptorPool>> extra_desc_pool_;
};

BIO_END_NAMESPACE