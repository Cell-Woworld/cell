#include "../proto/Webservice.pb.h"
#include "nlohmann/json.hpp"
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
				StartListen(_host, _port, _service_list);
			}
			catch (const std::exception& e) {
				LOG_E(TAG, "Web Service error: %s", e.what());
			}
			break;
		}
		case "Webservice.PerformResponse"_hash:
		{
			wait_response_.notify_all();
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

				Obj<Server> _svr = nullptr;
				if (!host.empty())
				{
					_svr = std::static_pointer_cast<Server>(std::make_shared<SSLServer>((GetRootPath() + host + ".cert").c_str(), (GetRootPath() + host + ".key").c_str()));
				}
				else
				{
					_svr = std::make_shared<Server>();
				}

				if (_svr == nullptr)
					return;

				for (auto service_info : service_list)
				{
					Start::ServiceInfo _service_info;
					if (_service_info.ParseFromString(service_info) == true)
					{
						interested_column_map_[_service_info.servicename()] = { _service_info.responsemodelname(), Set<String>(_service_info.interestedcolumnlist().begin(), _service_info.interestedcolumnlist().end()) };
						std::replace(_service_info.mutable_servicename()->begin(), _service_info.mutable_servicename()->end(), '.', '/');
						_svr->Post(((String)"/" + _service_info.servicename()).c_str(), [&](const Request& req, Response& res,
							const ContentReader& content_reader) {
							this->Handler(req, res, content_reader);
							});
					}
				}

				//_svr->Get("/stop", [&](const Request& req, Response& res) { _svr->stop(); });

				_svr->listen("0.0.0.0", port);
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
		const Set<String>& _column_set = interested_column_map_[_message_name].column_set_;
		String _response_model_name = interested_column_map_[_message_name].response_;
		std::replace(_message_name.begin(), _message_name.end(), '/', '.');
		WriteValue(_response_model_name, "");

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
			LOG_I(TAG, "Webservice::Handler() multipart_form_data request: %s(%s)", _message_name.c_str(), _root.dump().c_str());
			SendEvent(_message_name, _root.dump());
			// wait for response
			Mutex _response_mutex;
			CondLocker _lk(_response_mutex);
			wait_response_.wait(_lk, [this, _response_model_name] { return !ReadValue<String>(_response_model_name).empty(); });
			String _response = ReadValue<String>(_response_model_name);
			res.set_content(_response, "text/plain");
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
					LOG_I(TAG, "Webservice::Handler() request: %s(%s)", _message_name.c_str(), _root.dump().c_str());
					SendEvent(_message_name, _root.dump());
					// wait for response
					Mutex _response_mutex;
					CondLocker _lk(_response_mutex);
					wait_response_.wait(_lk, [this, _response_model_name] { return !ReadValue<String>(_response_model_name).empty(); });
					String _response = ReadValue<String>(_response_model_name);
					res.set_content(_response, "text/plain");
				}
				else
				{
					res.set_content("Error. Invalid parameters.", "text/plain");
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