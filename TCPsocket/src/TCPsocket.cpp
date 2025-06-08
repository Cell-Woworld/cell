#include "TCPsocket.h"
#include "TCPClient.h"
#include "TCPServer.h"
#include "../proto/TCPsocket.pb.h"
#include "../proto/Cell.pb.h"
#include <future>

#if defined(_WIN32)
#define GETSOCKETERRNO() (WSAGetLastError())
const char* get_error_text() {
	static char message[256] = { 0 };
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		0, WSAGetLastError(), 0, message, 256, 0);
	char* nl = strrchr(message, '\n');
	if (nl) *nl = 0;
	return message;
}
#else
#define GETSOCKETERRNO() (errno)
const char* get_error_text() {
	return  strerror(errno);
}
#endif

#define TAG "TCPsocket"

namespace TCPsocket
{
#ifdef STATIC_API
	extern "C" PUBLIC_API  RNA * TCPsocket_CreateInstance(IBiomolecule * owner)
	{
		return new TCPsocket(owner);
	}
#else
	extern "C" PUBLIC_API RNA* CreateInstance(IBiomolecule* owner)
	{
		return new TCPsocket(owner);
	}
#endif

	Map<TCPsocket::T_TCPsocket*, String> TCPsocket::socket_uuid_map_;
#ifdef OPENSSL
	Map<TCPsocket::T_SSLsocket*, String> TCPsocket::ssl_uuid_map_;
#endif
	Mutex TCPsocket::socket_uuid_map_mutex_;
	bool TCPsocket::ssl_ = false;
	Set<void*> TCPsocket::ignored_socket_set_;

	TCPsocket::TCPsocket(IBiomolecule* owner)
		: RNA(owner, "TCPsocket", this),
		YIELD(20),
		client_(new CTCPClient(nullptr)),
		server_(nullptr),
		receive_thread_(nullptr)
	{
		owner->init("*");		// to receive all messages of whole namespaces
		init();
	}

	TCPsocket::~TCPsocket()
	{
		MutexLocker _locker(socket_uuid_map_mutex_);
		if (!ssl_)
		{
			T_TCPsocket* _socket = (T_TCPsocket*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
			if (_socket != nullptr && socket_uuid_map_.count(_socket) > 0)
				socket_uuid_map_.erase(_socket);
		}
#ifdef OPENSSL
		else
		{
			T_SSLsocket* _ssl = (T_SSLsocket*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
			if (_ssl != nullptr && ssl_uuid_map_.count(_ssl) > 0)
				ssl_uuid_map_.erase(_ssl);
		}
#endif
		if (receive_thread_ != nullptr)
		{
			receive_thread_->join();
		}
	}

	void TCPsocket::OnEvent(const DynaArray& name)
	{
		const String& _name = name.str();
		switch (hash(_name))
		{
		// for TCPsocket Server
		case "TCPsocket.Start"_hash:
		{
			String _port = ReadValue<String>("TCPsocket.Start.port");
			String _cert_filename = ReadValue<String>("TCPsocket.Start.cert_filename");
			String _key_filename = ReadValue<String>("TCPsocket.Start.key_filename");
			if (_cert_filename != "" && _key_filename != "")
				ssl_ = true;
			std::cout << "****** Action name: " << _name << ", Port: " << _port << " ******" << "\n";
			StartListen(_port, GetRootPath() + _cert_filename, GetRootPath() + _key_filename);
			break;
		}
		// for TCPsocket Service
		case "TCPsocket.Service.Config"_hash:
		{
			std::cout << "****** Action name: " << _name << " ******" << "\n";
			if (!ssl_)
			{
				T_TCPsocket* _socket = (T_TCPsocket*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
				if (_socket != nullptr)
				{
					MutexLocker _locker(socket_uuid_map_mutex_);

					if (ignored_socket_set_.count(_socket) > 0)
					{
						ignored_socket_set_.erase(_socket);
						// destroy cell
						LOG_D(TAG, "Connection has been closed, ignored id=%p", _socket);
						Bio::Cell::ForwardEvent _forward_event;
						_forward_event.set_uuid(ReadValue<String>("Bio.Cell.uuid"));
						_forward_event.set_name(Service::ConnectionClosed::descriptor()->full_name());
						this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
						return;
					}

					//std::string _remote_address(_socket->getRemoteAddressAsText());
					/*
					std::string_view _remote_address_bin(_socket->getRemoteAddress());
					String _remote_address;
					for (size_t i = _remote_address_bin.size() - 4; i < _remote_address_bin.size(); i++)
					{
						_remote_address += std::to_string((unsigned char)_remote_address_bin[i]) + ".";
					}
					if (!_remote_address.empty())
						_remote_address.pop_back();

					WriteValue("Bio.Cell.Network.RemoteIPAddress", _remote_address);
					*/
					socket_uuid_map_.insert(make_pair(_socket, ReadValue<String>("Bio.Cell.uuid")));
				}
			}
#ifdef OPENSSL
			else
			{
				T_SSLsocket* _socket = (T_SSLsocket*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
				//PerSocketData* _user_data = (PerSocketData*)ws_->getUserData();
				//memset(_user_data->uuid, 0, sizeof(_user_data->uuid));
				//strcpy(_user_data->uuid, _uuid.c_str());
				if (_socket != nullptr)
				{
					MutexLocker _locker(socket_uuid_map_mutex_);

					if (ignored_socket_set_.count(_socket) > 0)
					{
						ignored_socket_set_.erase(_socket);
						// destroy cell
						LOG_D(TAG, "Connection has been closed, ignored id=%p", _socket);
						Bio::Cell::ForwardEvent _forward_event;
						_forward_event.set_uuid(ReadValue<String>("Bio.Cell.uuid"));
						_forward_event.set_name(Service::ConnectionClosed::descriptor()->full_name());
						this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
						return;
					}

					//std::string _remote_address(_socket->getRemoteAddressAsText());

					std::string_view _remote_address_bin(_socket->getRemoteAddress());
					String _remote_address;
					for (size_t i = _remote_address_bin.size() - 4; i < _remote_address_bin.size(); i++)
					{
						_remote_address += std::to_string((unsigned char)_remote_address_bin[i]) + ".";
					}
					if (!_remote_address.empty())
						_remote_address.pop_back();

					WriteValue("Bio.Cell.Network.RemoteIPAddress", _remote_address);
					ssl_uuid_map_.insert(make_pair(_socket, ReadValue<String>("Bio.Cell.uuid")));
				}
			}
#endif
			break;
		}
		case "TCPsocket.Connect"_hash:
		{
			String _address = ReadValue<String>("TCPsocket.Connect.address");
			String _port = ReadValue<String>("TCPsocket.Connect.port");
			header_size_ = ReadValue<int>("TCPsocket.Connect.header_size");
			is_protobuf_ = (header_size_ == 0);
			Map<String,int> _orig_protocol = ReadValue<Map<String, int>>("TCPsocket.Connect.protocol");
			for (auto itr = _orig_protocol.begin(); itr != _orig_protocol.end(); ++itr)
			{
				String _key;
				HexToASCII(itr->first, _key);
				protocol_[_key] = itr->second;
			}
#ifdef _WIN32
			// Not always starts a new thread, std::launch::async must be passed to force it.
			//std::future<bool> futConnect = std::async(std::launch::async,[&]() -> bool
			auto ConnectTask = [&]() -> bool
			{
				//m_pSSLTCPClient->SetSSLCerthAuth(CERT_AUTH_FILE); // not mandatory
				//m_pSSLTCPClient->SetSSLKeyFile(SSL_KEY_FILE); // not mandatory
				bool _retval = client_->Connect(_address, _port);
				if (_retval)
				{
					this->SendEvent(Connect::Result::descriptor()->full_name());
				}
				else
				{
					Connect::Error _error;
					_error.set_code(GETSOCKETERRNO());
					_error.set_message(get_error_text());
					this->SendEvent(Connect::Error::descriptor()->full_name(), _error.SerializeAsString());
				}
				return _retval;
			};
			std::packaged_task< bool(void) > packageConnect{ ConnectTask };
			std::future<bool> futConnect = packageConnect.get_future();
			std::thread ConnectThread{ std::move(packageConnect) }; // pack. task is not copyable
			ConnectThread.join();
#else
			auto ConnectTask = [&]() -> bool
			{
				//m_pSSLTCPClient->SetSSLKeyFile(SSL_KEY_FILE); // not mandatory
				//m_pSSLTCPClient->SetSSLCerthAuth(CERT_AUTH_FILE); // not mandatory
				bool _retval = client_->Connect(_address, _port);
				Connect::Result _result;
				_result.set_success(_retval);
				if (!_retval)
				{
					_result.set_code(GETSOCKETERRNO());
					_result.set_message(get_error_text());
				}
				this->SendEvent(Connect::Result::descriptor()->full_name(), _result.SerializeAsString());
				return _retval;
			};
			std::packaged_task< bool(void) > packageConnect{ ConnectTask };
			std::future<bool> futConnect = packageConnect.get_future();
			std::thread ConnectThread{ std::move(packageConnect) }; // pack. task is not copyable
			ConnectThread.join();
#endif
			break;
		}
		case "TCPsocket.StartReceive"_hash:
		{
			while (!client_->IsConnected())
				std::this_thread::sleep_for(std::chrono::milliseconds(YIELD));
			StartReceive();
			break;
		}
		default:
		{
			if (name != ReadValue<String>("Bio.Cell.Current.Event"))
				break;
			if (client_)
			{
				if (!ssl_)
				{
					Send(_name);
				}
				else
				{
				}
			}
			else
			{
				MutexLocker _locker(socket_uuid_map_mutex_);
				if (!ssl_)
				{
					T_TCPsocket* _socket = (T_TCPsocket*)ReadValue<address_t>(Service::Config::descriptor()->full_name() + ".id");
					if (_socket != nullptr && socket_uuid_map_.count(_socket) > 0)
					{
						Send(_name, _socket);
					}
				}
			}
			break;
		}
		}
	}

	void TCPsocket::StartListen(const String& port, const String& cert_filename, const String& key_filename)
	{
		client_ = nullptr;
		server_ = Obj<CTCPServer>(new CTCPServer(nullptr, port));
		header_size_ = ReadValue<int>("TCPsocket.Start.header_size");
		is_protobuf_ = (header_size_ == 0);
		Map<String, int> _orig_protocol = ReadValue<Map<String, int>>("TCPsocket.Start.protocol");
		
		for (auto itr = _orig_protocol.begin(); itr != _orig_protocol.end(); ++itr)
		{
			String _key;
			HexToASCII(itr->first, _key);
			protocol_[_key] = itr->second;
		}
		static String _cert_filename = (cert_filename == "") ? NULL : cert_filename.c_str();
		static String _key_filename = (key_filename == "") ? NULL : key_filename.c_str();
		std::future<void> futListen = std::async(std::launch::async, [&] {
			bool retval = true;
			while (retval)
			{
				T_TCPsocket* _accepted_socket = new T_TCPsocket();
				retval = server_->Listen(_accepted_socket->socket());
				if (retval == true)
				{
					uint64_t _incoming_connection = (uint64_t)(void*)_accepted_socket;
					NewConnection _new_connection;
					_new_connection.set_id(_incoming_connection);
					this->SendEvent(NewConnection::descriptor()->full_name(), _new_connection.SerializeAsString());
				}
			}
		});
		// client -> server
		auto ServerReceive = [&]() -> int {
			const int SELECT_TIMEOUT = 300;
			const size_t tenMeg = 10 * 1024 * 1024;
			std::vector<char> RcvBuffer(tenMeg);
			while (true)
			{
				{
					MutexLocker _locker(socket_uuid_map_mutex_);
					ASocket::Socket* _select_list = new ASocket::Socket[socket_uuid_map_.size()];
					T_TCPsocket** _select_socket = new T_TCPsocket * [socket_uuid_map_.size()];
					int i = 0;
					for (auto it = socket_uuid_map_.begin(); it != socket_uuid_map_.end(); ++it, i++)
					{
						_select_list[i] = it->first->socket();
						_select_socket[i] = it->first;
					}
					while (socket_uuid_map_.size() > 0)
					{
						size_t _selected_index = 0;
						int ret = ASocket::SelectSockets(_select_list, socket_uuid_map_.size(), SELECT_TIMEOUT, _selected_index);

						if (ret <= 0)
						{
							break;
						}
						else
						{
							int readCount = 0;
							if (is_protobuf_)
							{
								ProtoHeader _header;
								readCount = _select_socket[_selected_index]->Receive(RcvBuffer.data(), _header.ByteSizeLong(), true);
								if (readCount == _header.ByteSizeLong() && _header.ParseFromArray(RcvBuffer.data(), readCount) == true)
								{
									if (_header.size() > 0)
									{
										readCount = _select_socket[_selected_index]->Receive(RcvBuffer.data(), _header.size(), true);
										ProtoEvent _proto_event;
										if (readCount == _header.size() && _proto_event.ParseFromArray(RcvBuffer.data(), readCount) == true)
										{
											if (socket_uuid_map_.count(_select_socket[_selected_index]) > 0)
											{
												Bio::Cell::ForwardEvent _forward_event;
												_forward_event.set_uuid(socket_uuid_map_[_select_socket[_selected_index]]);
												_forward_event.set_name(_proto_event.name());
												_forward_event.set_payload(_proto_event.payload());
												this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
											}
										}
										else
										{
											printf("!!! ERROR! Invalid protobuf event: %s\n", RcvBuffer.data());
										}
									}
								}
							}
							else if (header_size_ > 0)
							{
								readCount = _select_socket[_selected_index]->Receive(RcvBuffer.data(), header_size_, true);
								if (readCount == header_size_)
								{
									String _header(RcvBuffer.data(), header_size_);
									String _hex_header;
									ASCIIToHex(_header, _hex_header);
									if (protocol_.count(RcvBuffer.data()) > 0 && protocol_[RcvBuffer.data()]>0)
									{
										int _content_size = protocol_[RcvBuffer.data()];
										readCount = _select_socket[_selected_index]->Receive(RcvBuffer.data(), _content_size, true);
										if (readCount == _content_size)
										{
											if (socket_uuid_map_.count(_select_socket[_selected_index]) > 0)
											{
												RawEventReceived _raw_event;
												_raw_event.mutable_payload()->set_header(_hex_header);
												_raw_event.mutable_payload()->set_content(RcvBuffer.data(), _content_size);
												Bio::Cell::ForwardEvent _forward_event;
												_forward_event.set_uuid(socket_uuid_map_[_select_socket[_selected_index]]);
												_forward_event.set_name(RawEventReceived::descriptor()->full_name());
												_forward_event.set_payload(_raw_event.SerializeAsString());
												this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
											}
										}
										else
										{
											printf("!!! ERROR! Invalid protobuf event: %s\n", RcvBuffer.data());
										}
									}
								}
							}
							if (readCount == 0)
							{
								if (socket_uuid_map_.count(_select_socket[_selected_index]) > 0)
								{
									LOG_I(TAG, "connection closed, id=%p", _select_socket[_selected_index]);
									Bio::Cell::ForwardEvent _forward_event;
									_forward_event.set_uuid(socket_uuid_map_[_select_socket[_selected_index]]);
									_forward_event.set_name(Service::ConnectionClosed::descriptor()->full_name());
									this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
									//_forward_event.set_uuid(socket_uuid_map_[ws]);
									//_forward_event.set_name(Bio::Cell::Destroyed::descriptor()->full_name());
									//this->SendEvent(Bio::Cell::ForwardEvent::descriptor()->full_name(), _forward_event.SerializeAsString());
									socket_uuid_map_.erase(_select_socket[_selected_index]);
									_select_socket[_selected_index] = nullptr;
								}
								else
								{
									LOG_D(TAG, "connection closed, but id(%p) is not found", _select_socket[_selected_index]);
									ignored_socket_set_.insert(_select_socket[_selected_index]);
								}
								break;
							}
						}
					}
					delete[] _select_list;
					delete[] _select_socket;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(YIELD));
			}

			return 0;
		};
		std::future<int> futServerReceive = std::async(std::launch::async, ServerReceive);
	}
	void TCPsocket::Send(const String& name, T_TCPsocket* socket)
	{
		if (is_protobuf_)
		{
			ProtoEvent _command;
			_command.set_name(name);
			_command.set_payload(ReadValue<String>(String("encode.") + name));
			try
			{
				if (socket != nullptr)
					socket->Send(_command.SerializeAsString());
				else
					client_->Send(_command.SerializeAsString());
			}
			catch (const std::exception& e)
			{
				printf("socket is not available. exception: %s\n", e.what());
			}
			catch (...)
			{
				printf("socket is not available.\n");
			}
		}
		else
		{
			if (name == RawEvent::descriptor()->full_name())
			{
				RawEvent _raw_event;
				if (_raw_event.ParseFromString(ReadValue<String>(String("encode.") + name)) == true)
				{
					String _header;
					HexToASCII(_raw_event.header(), _header);
					try
					{
						String _content = _header + _raw_event.content();
						if (socket != nullptr)
						{
							socket->Send(_content);
						}
						else
						{
							client_->Send(_content);
						}
					}
					catch (const std::exception& e)
					{
						printf("socket is not available. exception: %s\n", e.what());
					}
					catch (...)
					{
						printf("socket is not available.\n");
					}
				}
			}
		}
	}
	void TCPsocket::StartReceive()
	{
		auto ClientReceive = [&]()->bool {
			const size_t tenMeg = 10 * 1024 * 1024;
			std::vector<char> RcvBuffer(tenMeg);
			while (true)
			{
				int readCount = 0;
				if (is_protobuf_)
				{
					ProtoHeader _header;
					readCount = client_->Receive(RcvBuffer.data(), _header.ByteSizeLong(), true);
					if (readCount == _header.ByteSizeLong() && _header.ParseFromArray(RcvBuffer.data(), readCount) == true)
					{
						if (_header.size() > 0)
						{
							readCount = client_->Receive(RcvBuffer.data(), _header.size(), true);
							ProtoEvent _proto_event;
							if (readCount == _header.size() && _proto_event.ParseFromArray(RcvBuffer.data(), readCount) == true)
							{
								this->SendEvent(_proto_event.name(), _proto_event.payload());
							}
							else
							{
								printf("!!! ERROR! Invalid protobuf event: %s\n", RcvBuffer.data());
							}
						}
					}
				}
				else if (header_size_ > 0)
				{
					readCount = client_->Receive(RcvBuffer.data(), header_size_, true);
					if (readCount == header_size_)
					{
						String _header(RcvBuffer.data(), header_size_);
						String _hex_header;
						ASCIIToHex(_header, _hex_header);
						if (protocol_.count(RcvBuffer.data()) > 0 && protocol_[RcvBuffer.data()] > 0)
						{
							int _content_size = protocol_[RcvBuffer.data()];
							readCount = client_->Receive(RcvBuffer.data(), _content_size, true);
							if (readCount == _content_size)
							{
								RawEventReceived _raw_event;
								_raw_event.mutable_payload()->set_header(_hex_header);
								_raw_event.mutable_payload()->set_content(RcvBuffer.data(), _content_size);
								this->SendEvent(RawEventReceived::descriptor()->full_name(), _raw_event.SerializeAsString());
							}
							else
							{
								printf("!!! ERROR! Invalid protobuf event: %s\n", RcvBuffer.data());
							}
						}
					}
				}
				if (readCount == 0)
				{
					break;
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(YIELD));
				}
			}
			return 0;
		};

		// launch the receive in another thread to prevent server from
		// hanging when sending "many" bytes.
		//std::future<int> futClientReceive = std::async(std::launch::async, ClientReceive);
		std::packaged_task< bool(void) > packageReceive{ ClientReceive };
		std::future<bool> futReceive = packageReceive.get_future();
		receive_thread_ = Obj<std::thread>(new std::thread{ std::move(packageReceive) }); // pack. task is not copyable
		//ReceiveThread.join();
	}
	void TCPsocket::ASCIIToHex(const String& in, String& out)
	{
		static const char hex_digits[] = "0123456789ABCDEF";

		out.reserve(in.length() * 2);
		for (unsigned char c : in)
		{
			out.push_back(hex_digits[c >> 4]);
			out.push_back(hex_digits[c & 15]);
		}
	}

	void TCPsocket::HexToASCII(const String& in, String& out)
	{
		// C++98 guarantees that '0', '1', ... '9' are consecutive.
		// It only guarantees that 'a' ... 'f' and 'A' ... 'F' are
		// in increasing order, but the only two alternative encodings
		// of the basic source character set that are still used by
		// anyone today (ASCII and EBCDIC) make them consecutive.
		auto hexval = [](unsigned char c)
		{
			if ('0' <= c && c <= '9')
				return c - '0';
			else if ('a' <= c && c <= 'f')
				return c - 'a' + 10;
			else if ('A' <= c && c <= 'F')
				return c - 'A' + 10;
			else throw std::invalid_argument("invalid hex digit");
		};

		out.clear();
		out.reserve(in.length() / 2);
		for (std::string::const_iterator p = in.begin(); p != in.end(); p++)
		{
			unsigned char c = hexval(*p);
			p++;
			if (p == in.end()) break; // incomplete last digit - should report error
			c = (c << 4) + hexval(*p); // + takes precedence over <<
			out.push_back(c);
		}
	}
}
