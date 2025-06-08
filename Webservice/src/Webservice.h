#pragma once
#include "RNA.h"

namespace httplib {
	struct Request;
	struct Response;
	class ContentReader;
}

namespace Webservice
{
	struct ResponseInfo
	{
		String response_;
		Set<String> column_set_;
	};
	class Webservice : public BioSys::RNA
	{
	public:
		PUBLIC_API Webservice(BioSys::IBiomolecule* owner);
		PUBLIC_API virtual ~Webservice();
		void SendEvent(const String& name, const String& payload);

	protected:
		virtual void OnEvent(const DynaArray& name);

	private:
		void StartListen(const String& host, int port, const Array<String>& service_name_list);
		void Handler(const httplib::Request& req, httplib::Response& res, const httplib::ContentReader& content_reader);
		void ConvertUrlToJSON(const String& content, nlohmann::json& root);
		void GetQueryMap(const String& content, Map<String, String>& query_map);

	private:
		static Mutex send_event_mutex_;

	private:
		Map<String, ResponseInfo> interested_column_map_;
		Cond_Var wait_response_;
	};

}