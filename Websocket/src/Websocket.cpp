#include "Websocket.h"
#include "../proto/Websocket.pb.h"
#include "../proto/Cell.pb.h"
#include "../proto/Web.pb.h"

#define TAG "Websocket"

namespace Websocket
{

#ifdef STATIC_API
	extern "C" PUBLIC_API  RNA * Websocket_CreateInstance(IBiomolecule * owner)
	{
		return new Websocket(owner);
	}
#else
	extern "C" PUBLIC_API RNA* CreateInstance(IBiomolecule* owner)
	{
		return new Websocket(owner);
	}
#endif

	Map<Websocket::T_WebSocket*, String> Websocket::ws_uuid_map_;
	Map<Websocket::T_WSS*, String> Websocket::wss_uuid_map_;
	Mutex Websocket::ws_uuid_map_mutex_;
	bool Websocket::ssl_ = false;
	Set<void*> Websocket::ignored_ws_set_;
	Queue<Pair<String, String>> Websocket::message_queue_;
	uWS::Loop* Websocket::ws_loop_ = nullptr;

	Websocket::Websocket(IBiomolecule* owner)
		: RNA(owner, "Websocket", this)
	{
		owner->init("*");		// to receive all messages of whole namespaces
		init();
	}

	Websocket::~Websocket()
	{
		MutexLocker _locker(ws_uuid_map_mutex_);
		if (!ssl_)
		{
			T_WebSocket* _ws = (T_WebSocket*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
			if (_ws != nullptr && ws_uuid_map_.count(_ws) > 0)
				ws_uuid_map_.erase(_ws);
		}
		else
		{
			T_WSS* _ws = (T_WSS*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
			if (_ws != nullptr && wss_uuid_map_.count(_ws) > 0)
				wss_uuid_map_.erase(_ws);
		}
	}

	void Websocket::OnEvent(const DynaArray& name)
	{
		const String& _name = name.str();
		switch (hash(_name))
		{
		// for Websocket Server
		case "Websocket.Start"_hash:
		{
			int _port = ReadValue<int>("Websocket.Start.port");
			String _cert_filename = ReadValue<String>("Websocket.Start.cert_filename");
			String _key_filename = ReadValue<String>("Websocket.Start.key_filename");
			if (_cert_filename != "" && _key_filename != "")
				ssl_ = true;
			std::cout << "****** Action name: " << _name << ", Port: " << _port << " ******" << "\n";
			StartListen(_port, GetRootPath() + _cert_filename, GetRootPath() + _key_filename);
			break;
		}
		// for Websocket Service
		case "Websocket.Service.Config"_hash:
		{
			std::cout << "****** Action name: " << _name << " ******" << "\n";
			if (!ssl_)
			{
				T_WebSocket* _ws = (T_WebSocket*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
				//String _stem_uuid = ReadValue<String>(Service::Config::descriptor()->full_name() + ".stem_id");
				//WriteValue("Bio.Cell.Stem.uuid", _stem_uuid);
				//PerSocketData* _user_data = (PerSocketData*)ws_->getUserData();
				//memset(_user_data->uuid, 0, sizeof(_user_data->uuid));
				//strcpy(_user_data->uuid, _uuid.c_str());
				if (_ws != nullptr)
				{
					MutexLocker _locker(ws_uuid_map_mutex_);

					if (ignored_ws_set_.count(_ws) > 0)
					{
						ignored_ws_set_.erase(_ws);
						// destroy cell
						LOG_D(TAG, "Connection has been closed, ignored id=%p", _ws);
						Bio::Cell::ForwardEvent _forward_event;
						_forward_event.set_uuid(ReadValue<String>("Bio.Cell.uuid"));
						_forward_event.set_name(Service::ConnectionClosed::descriptor()->full_name());
						this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
						return;
					}

					//std::string _remote_address(_ws->getRemoteAddressAsText());

					std::string_view _remote_address_bin(_ws->getRemoteAddress());
					String _remote_address;
					for (size_t i = _remote_address_bin.size() - 4; i < _remote_address_bin.size(); i++)
					{
						_remote_address += std::to_string((unsigned char)_remote_address_bin[i]) + ".";
					}
					if (!_remote_address.empty())
						_remote_address.pop_back();

					WriteValue("Bio.Cell.Network.RemoteIPAddress", _remote_address);
					String _uuid = ReadValue<String>("Bio.Cell.uuid");
					ws_uuid_map_.insert(make_pair(_ws, _uuid));
					while (!message_queue_.empty())
					{
						Bio::Cell::ForwardEvent _forward_event;
						_forward_event.set_uuid(_uuid);
						_forward_event.set_name(message_queue_.front().first);
						_forward_event.set_payload(message_queue_.front().second);
						this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
						LOG_I(TAG, "(From queue) Forward message %s to %s, payload=%s", _forward_event.name().c_str(), _forward_event.uuid().c_str(), _forward_event.payload().c_str());
						message_queue_.pop();
					}
				}
			}
			else
			{
				T_WSS* _ws = (T_WSS*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
				//String _stem_uuid = ReadValue<String>(Service::Config::descriptor()->full_name() + ".stem_id");
				//WriteValue("Bio.Cell.Stem.uuid", _stem_uuid);
				//PerSocketData* _user_data = (PerSocketData*)ws_->getUserData();
				//memset(_user_data->uuid, 0, sizeof(_user_data->uuid));
				//strcpy(_user_data->uuid, _uuid.c_str());
				if (_ws != nullptr)
				{
					MutexLocker _locker(ws_uuid_map_mutex_);

					if (ignored_ws_set_.count(_ws) > 0)
					{
						ignored_ws_set_.erase(_ws);
						// destroy cell
						LOG_D(TAG, "Connection has been closed, ignored id=%p", _ws);
						Bio::Cell::ForwardEvent _forward_event;
						_forward_event.set_uuid(ReadValue<String>("Bio.Cell.uuid"));
						_forward_event.set_name(Service::ConnectionClosed::descriptor()->full_name());
						this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
						return;
					}

					//std::string _remote_address(_ws->getRemoteAddressAsText());

					std::string_view _remote_address_bin(_ws->getRemoteAddress());
					String _remote_address;
					for (size_t i = _remote_address_bin.size() - 4; i < _remote_address_bin.size(); i++)
					{
						_remote_address += std::to_string((unsigned char)_remote_address_bin[i]) + ".";
					}
					if (!_remote_address.empty())
						_remote_address.pop_back();

					WriteValue("Bio.Cell.Network.RemoteIPAddress", _remote_address);
					String _uuid = ReadValue<String>("Bio.Cell.uuid");
					wss_uuid_map_.insert(make_pair(_ws, _uuid));
					while (!message_queue_.empty())
					{
						Bio::Cell::ForwardEvent _forward_event;
						_forward_event.set_uuid(_uuid);
						_forward_event.set_name(message_queue_.front().first);
						_forward_event.set_payload(message_queue_.front().second);
						this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
						message_queue_.pop();
					}
				}
			}
			break;
		}
		/*
		case "Web.AddProto"_hash:
		{
			Web::AddProto _add_proto;
			if (_add_proto.ParseFromString(ReadValue<String>("Web.AddProto.@payload")) == true)
			{
				printf("AddProto's content:\n%s\n", _add_proto.content().c_str());
			}
			//break;
		}
		*/
		default:
		{
			if (name != ReadValue<String>("Bio.Cell.Current.Event"))
				break;
			//MutexLocker _locker(ws_uuid_map_mutex_);
			if (!ssl_)
			{
				T_WebSocket* _ws = (T_WebSocket*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
				if (_ws != nullptr && ws_uuid_map_.count(_ws) > 0)
				{
					if (_name == Web::RawMessage::descriptor()->full_name())
					{
						String _raw_message = ReadValue<String>(_name + ".payload");
						//_ws->send(_raw_message);
						ws_loop_->defer([_ws, _raw_message]() {
							_ws->send(_raw_message);
							});
					}
					else
					{
						Web::WebEvent _command;
						_command.set_name(_name);
						_command.set_payload(ReadValue<String>(String("encode.") + _name));
						try
						{
							//_ws->send(_command.SerializeAsString());
							String _message = _command.SerializeAsString();
							ws_loop_->defer([_ws, _message]() {
								_ws->send(_message);
								});
						}
						catch (const std::exception& e)
						{
							printf("Web socket is not available. exception: %s\n", e.what());
						}
						catch (...)
						{
							printf("Web socket is not available.\n");
						}
					}
				}
			}
			else
			{
				T_WSS* _ws = (T_WSS*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
				if (_ws != nullptr && wss_uuid_map_.count(_ws) > 0)
				{
					if (_name == Web::RawMessage::descriptor()->full_name())
					{
						String _raw_message = ReadValue<String>(_name + ".payload");
						//_ws->send(_raw_message);
						ws_loop_->defer([_ws, _raw_message]() {
							_ws->send(_raw_message);
							});
					}
					else
					{
						Web::WebEvent _command;
						_command.set_name(_name);
						_command.set_payload(ReadValue<String>(String("encode.") + _name));
						try
						{
							//_ws->send(_command.SerializeAsString());
							String _message = _command.SerializeAsString();
							ws_loop_->defer([_ws, _message]() {
								_ws->send(_message);
								});
						}
						catch (const std::exception& e)
						{
							printf("Web socket is not available. exception: %s\n", e.what());
						}
						catch (...)
						{
							printf("Web socket is not available.\n");
						}
					}
				}
			}
			break;
		}
		}
	}

	void Websocket::StartListen(int port, const String& cert_filename, const String& key_filename)
	{
		/* ws->getUserData returns one of these */
		static String _cert_filename = (cert_filename == "") ? NULL : cert_filename.c_str();
		static String _key_filename = (key_filename == "") ? NULL : key_filename.c_str();
		std::thread([this, port] {
			if (!ssl_) {
				uWS::TemplatedApp<!SSL>(
					{
						.key_file_name = _key_filename.c_str(),
						.cert_file_name = _cert_filename.c_str(),
						.passphrase = NULL,
						.dh_params_file_name = NULL,
						.ca_file_name = NULL
					}
					).ws<PerSocketData>("/*", {
					/* Settings */
					.compression = uWS::SHARED_COMPRESSOR,
					.maxPayloadLength = 64 * 1024 * 1024,
					.idleTimeout = 60 * 16,
					.closeOnBackpressureLimit = false,
					.resetIdleTimeoutOnSend = true,
					.sendPingsAutomatically = true,
					/* Handlers */
					.upgrade = nullptr,
					.open = [this](auto* ws) {
						uint64_t _incoming_connection = (uint64_t)(void*)ws;
						NewConnection _new_connection;
						_new_connection.set_id(_incoming_connection);
						this->SendEvent(NewConnection::descriptor()->full_name(), _new_connection.SerializeAsString());
					},
					.message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) {
						//if (this->ws_ != nullptr)
						//	this->ws_->send(message, opCode);		// echo
						Web::WebEvent _web_event;
						String _web_event_payload(message);
						if (_web_event_payload == "" || _web_event.ParseFromString(_web_event_payload) == false)
						{
							LOG_D(TAG, "Invalid web event: %s\n", _web_event_payload.c_str());
							Web::WebEvent _customized_message;
							_customized_message.set_name("customized message");
							_customized_message.set_payload(_web_event_payload);
							_web_event.set_name(Web::WebEvent::descriptor()->full_name());
							_web_event.set_payload(_customized_message.SerializeAsString());
						}
						{
							MutexLocker _locker(ws_uuid_map_mutex_);
							if (ws_uuid_map_.count(ws) == 0)
								this->message_queue_.push(make_pair(_web_event.name(), _web_event.payload()));
							else
							{
								Bio::Cell::ForwardEvent _forward_event;
								_forward_event.set_uuid(ws_uuid_map_[ws]);
								_forward_event.set_name(_web_event.name());
								_forward_event.set_payload(_web_event.payload());
								this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
								LOG_T(TAG, "(Directly) Forward message %s to %s, payload=%s", _forward_event.name().c_str(), _forward_event.uuid().c_str(), _forward_event.payload().c_str());
							}
						}
					},
					.drain = [](auto*/*ws*/) {
						/* Check ws->getBufferedAmount() here */
					},
					.ping = [](auto*/*ws*/, std::string_view) {
						/* Not implemented yet */
					},
					.pong = [](auto*/*ws*/, std::string_view) {
						/* Not implemented yet */
					},
					.close = [this](auto* ws, int code, std::string_view message) {
						MutexLocker _locker(ws_uuid_map_mutex_);
						if (ws_uuid_map_.count(ws) > 0)
						{
							LOG_I(TAG, "connection closed, id=%p", ws);
							Bio::Cell::ForwardEvent _forward_event;
							_forward_event.set_uuid(ws_uuid_map_[ws]);
							_forward_event.set_name(Service::ConnectionClosed::descriptor()->full_name());
							this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
							//_forward_event.set_uuid(ws_uuid_map_[ws]);
							//_forward_event.set_name(Bio::Cell::Destroyed::descriptor()->full_name());
							//this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
							ws_uuid_map_.erase(ws);
						}
						else
						{
							LOG_D(TAG, "connection closed, but id(%p) is not found", ws);
							ignored_ws_set_.insert(ws);
						}
					}
					}).listen(port, [port](auto* token) {
						if (token) {
							std::cout << "Listening on port " << port << std::endl;
						}
						ws_loop_ = uWS::Loop::get();
						}).run();
			} else {
				uWS::TemplatedApp<SSL>(
					{
						.key_file_name = _key_filename.c_str(),
						.cert_file_name = _cert_filename.c_str(),
						.passphrase = NULL,
						.dh_params_file_name = NULL,
						.ca_file_name = NULL
					}
					).ws<PerSocketData>("/*", {
						/* Settings */
						.compression = uWS::SHARED_COMPRESSOR,
						.maxPayloadLength = 64 * 1024 * 1024,
						.idleTimeout = 60 * 16,
						.closeOnBackpressureLimit = false,
						.resetIdleTimeoutOnSend = true,
						.sendPingsAutomatically = true,
						/* Handlers */
						.upgrade = nullptr,
						.open = [this](auto* ws) {
							uint64_t _incoming_connection = (uint64_t)(void*)ws;
							NewConnection _new_connection;
							_new_connection.set_id(_incoming_connection);
							this->SendEvent(NewConnection::descriptor()->full_name(), _new_connection.SerializeAsString());
						},
						.message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) {
							//if (this->ws_ != nullptr)
							//	this->ws_->send(message, opCode);		// echo
							Web::WebEvent _web_event;
							String _web_event_payload(message);
							if (_web_event_payload == "" || _web_event.ParseFromString(_web_event_payload) == false)
							{
								LOG_D(TAG, "Customized web event: %s\n", _web_event_payload.c_str());
								Web::WebEvent _customized_message;
								_customized_message.set_name("customized message");
								_customized_message.set_payload(_web_event_payload);
								_web_event.set_name(Web::WebEvent::descriptor()->full_name());
								_web_event.set_payload(_customized_message.SerializeAsString());
							}
							{
								MutexLocker _locker(ws_uuid_map_mutex_);
								if (wss_uuid_map_.count(ws) == 0)
									this->message_queue_.push(make_pair(_web_event.name(), _web_event.payload()));
								else
								{
									Bio::Cell::ForwardEvent _forward_event;
									_forward_event.set_uuid(wss_uuid_map_[ws]);
									_forward_event.set_name(_web_event.name());
									_forward_event.set_payload(_web_event.payload());
									this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
								}
							}
						},
						.drain = [](auto*/*ws*/) {
							/* Check ws->getBufferedAmount() here */
						},
						.ping = [](auto*/*ws*/, std::string_view) {
							/* Not implemented yet */
						},
						.pong = [](auto*/*ws*/, std::string_view) {
							/* Not implemented yet */
						},
						.close = [this](auto* ws, int code, std::string_view message) {
							MutexLocker _locker(ws_uuid_map_mutex_);
							if (wss_uuid_map_.count(ws) > 0)
							{
								Bio::Cell::ForwardEvent _forward_event;
								_forward_event.set_uuid(wss_uuid_map_[ws]);
								_forward_event.set_name(Service::ConnectionClosed::descriptor()->full_name());
								this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
								//_forward_event.set_uuid(wss_uuid_map_[ws]);
								//_forward_event.set_name(Bio::Cell::Destroyed::descriptor()->full_name());
								//this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
								wss_uuid_map_.erase(ws);
							}
							else
							{
								LOG_D(TAG, "connection closed, but id(%p) is not found", ws);
								ignored_ws_set_.insert(ws);
							}
						}
						}).listen(port, [port](auto* token) {
							if (token) {
								std::cout << "Listening on port " << port << std::endl;
							}
							ws_loop_ = uWS::Loop::get();
							}).run();
			}
		}).detach();
	}
}
