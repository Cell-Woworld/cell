#include "../proto/Webservice.pb.h"
#include "internal/utils/nlohmann/json.hpp"
#include "Webservice.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

#include "UrlEncoder/Encoder.hpp"

#define TAG "Webservice"

namespace Webservice
{
	Mutex Webservice::send_event_mutex_;

#ifndef STATIC_API
	extern "C" PUBLIC_API BioSys::RNA * CreateInstance(BioSys::IBiomolecule * owner)
	{
		return new Webservice(owner);
	}
#endif

	Webservice::Webservice(BioSys::IBiomolecule* owner)
		:RNA(owner, "Webservice", this)
	{
		init();
	}

	Webservice::~Webservice()
	{
	}

	void Webservice::OnEvent(const DynaArray& name)
	{
		USING_BIO_NAMESPACE
		const String& _name = name.str();
		switch (BioSys::hash(_name))
		{
		case "Webservice.Start"_hash:
		{
			try {
				String _host = ReadValue<String>(_name + ".host");
				int _port = ReadValue<int>(_name + ".port");
				Array<String> _service_list = ReadValue<Array<String>>(_name + ".serviceList");
				std::cout << "****** Action name: " << _name << ", Port: " << _port << " ******" << "\n";
				StartListen(_host, _port, _service_list);
			}
			catch (const std::exception& e) {
				LOG_E(TAG, "Web Service error: %s", e.what());
			}
			break;
		}
		default:
			break;
		}
	}

	void Webservice::StartListen(const String& host, int port, const Array<String>& service_list)
	{
		try {
			std::thread([this, port, host, service_list] {
				using namespace httplib;

				//Server svr;
				SSLServer svr((GetRootPath() + host + ".cert").c_str(), (GetRootPath() + host + ".key").c_str());

				svr.Get("/hi", [](const Request& req, Response& res) {
					res.set_content("Hello World!", "text/plain");
					});

				for (auto service_info : service_list)
				{
					Start::ServiceInfo _service_info;
					if (_service_info.ParseFromString(service_info) == true)
					{
						interested_column_map_[_service_info.servicename()] = { _service_info.response(), Set<String>(_service_info.interestedcolumnlist().begin(), _service_info.interestedcolumnlist().end()) };
						std::replace(_service_info.mutable_servicename()->begin(), _service_info.mutable_servicename()->end(), '.', '/');
						svr.Post(((String)"/" + _service_info.servicename()).c_str(), [&](const Request& req, Response& res,
							const ContentReader& content_reader) {
							this->Handler(req, res, content_reader);
							});
					}
				}

				//svr.Get("/stop", [&](const Request& req, Response& res) { svr.stop(); });

				svr.listen("0.0.0.0", port);
			}).detach();
			std::cout << "Webservice: Listening on port " << port << std::endl;
		}
		catch (const std::exception& e) {
			LOG_E(TAG, "Webservice::StartListen() port=%d, exception: %s", port, e.what());
		}
	}

	void Webservice::SendEvent(const String& name, const String& payload)
	{
		MutexLocker _lock(send_event_mutex_);
		Remove(name + ".*");
		WriteValue(name + "." + TAG_PACKET, payload);
		RNA::SendEvent(name);
	}

	void Webservice::Handler(const httplib::Request& req, httplib::Response& res, const httplib::ContentReader& content_reader)
	{
		using namespace httplib;
		using json = nlohmann::json;
		String _message_name = req.path.substr(1);
		std::replace(_message_name.begin(), _message_name.end(), '/', '.');
		const Set<String>& _column_set = interested_column_map_[_message_name].column_set_;
		String _response = interested_column_map_[_message_name].response_;
		res.set_content(_response, "text/plain");

		if (req.is_multipart_form_data())
		{
			//MultipartFormDataItems files;
			json _root;
			String _name;

			content_reader(
				[&](const MultipartFormData& file) {
					//files.push_back(file);
					if (_column_set.size() > 0 && _column_set.count(_name) == 0)
					{
						_name = "";
						return false;
					}
					else
					{
						_name = file.name;
						return true;
					}
				},
				[&](const char* data, size_t data_length) {
					//files.back().content.append(data, data_length);
					if (data_length > 0 && !_name.empty())
					{
						if (_root.count(_name) > 0)
							_root[_name] += String(data, data_length);
						else
							_root[_name] = String(data, data_length);
					}
					return true;
				});
			SendEvent(_message_name, _root.dump());
		}
		else 
		{
			content_reader([&](const char* data, size_t data_length) {
				std::string body;
				body.append(data, data_length);
				json _root;
				try
				{
					_root = json::parse(body);
				}
				catch (const std::exception&)
				{
					ConvertUrlToJSON(body, _root);
				}
				if (!_root.is_null())
				{
					std::cout << _root.dump() << std::endl;
					if (_column_set.size() > 0)
					{
						for (auto itr = _root.begin(); itr != _root.end();)
						{
							if (_column_set.count(itr.key()) == 0)
								itr = _root.erase(itr);
							else
								++itr;
						}
					}
					String _message_name = req.path.substr(1);
					std::replace(_message_name.begin(), _message_name.end(), '/', '.');
					SendEvent(_message_name, _root.dump());
				}
				return true;
			});
		}
	}
	
	void Webservice::ConvertUrlToJSON(const String& content, nlohmann::json& root)
	{
		Encoder _encoder;
		String _content = _encoder.UrlDecode(content);
		Map<String, String> _query_map;
		GetQueryMap(_content, _query_map);
		for (const auto& elem : _query_map)
		{
			root[elem.first] = elem.second;
		}
	}

	void Webservice::GetQueryMap(const String& content, Map<String, String>& query_map)
	{
		const char separator = '&';
		size_t carat = 0;
		size_t stanza_end = content.find_first_of(separator);
		do
		{
			std::string stanza = content.substr(carat, ((stanza_end != std::string::npos) ? (stanza_end - carat) : std::string::npos));
			size_t key_value_divider = stanza.find_first_of('=');
			std::string key = stanza.substr(0, key_value_divider);
			std::string value;
			if (key_value_divider != std::string::npos)
			{
				value = stanza.substr((key_value_divider + 1));
			}

			if (query_map.count(key) != 0)
			{
				throw std::invalid_argument("Bad key in the query string!");
			}

			query_map.emplace(key, value);
			carat = ((stanza_end != std::string::npos) ? (stanza_end + 1)
				: std::string::npos);
			stanza_end = content.find_first_of(separator, carat);
		} while ((stanza_end != std::string::npos)
			|| (carat != std::string::npos));
	}
}