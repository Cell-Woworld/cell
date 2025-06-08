#pragma once
#include "AcceptedSocket.h"
#include "RNA.h"

USING_BIO_NAMESPACE

class CTCPClient;
class CTCPServer;


namespace TCPsocket
{

class TCPsocket : public RNA
{
	static const bool SSL = true;
	static bool ssl_;
#ifdef _WIN32
	#ifdef _WIN64
		typedef unsigned long long address_t;
	#else
		typedef unsigned long address_t;
	#endif
#else
	typedef unsigned long long address_t;
#endif
public:
	PUBLIC_API TCPsocket(IBiomolecule* owner);
	PUBLIC_API virtual ~TCPsocket();

protected:
	virtual void OnEvent(const DynaArray& name);

private:
	const int YIELD;
	bool is_protobuf_;
	int header_size_;
	Map<String, int> protocol_;
	Obj<CTCPClient> client_;
	Obj<CTCPServer> server_;

	typedef AcceptedSocket T_TCPsocket;
	static Map<T_TCPsocket*, String> socket_uuid_map_;
#ifdef OPENSSL
	typedef AcceptedSSLSocket T_SSLsocket;
	static Map<T_SSLsocket*, String> ssl_uuid_map_;
#endif
	static Set<void*> ignored_socket_set_;
	static Mutex socket_uuid_map_mutex_;
	Obj<std::thread> receive_thread_;

private:
	void StartListen(const String& port, const String& cert_filename = "", const String& key_filename = "");
	void Send(const String& name, T_TCPsocket* socket = nullptr);
	void StartReceive();
	void ASCIIToHex(const String& in, String& out);
	void HexToASCII(const String& in, String& out);
};

}