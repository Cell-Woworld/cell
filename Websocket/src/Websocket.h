#pragma once
#include "RNA.h"
#include "App.h"

USING_BIO_NAMESPACE

namespace Websocket
{

class Websocket : public RNA
{
	static const bool SSL = true;
	static bool ssl_;
public:
	PUBLIC_API Websocket(IBiomolecule* owner);
	PUBLIC_API virtual ~Websocket();

protected:
	virtual void OnEvent(const DynaArray& name);

private:
	void StartListen(int port, const String& cert_filename = "", const String& key_filename = "");

private:
	typedef uWS::WebSocket<!SSL, true> T_WebSocket;
	typedef uWS::WebSocket<SSL, true> T_WSS;
	static Map<T_WebSocket*, String> ws_uuid_map_;
	static Map<T_WSS*, String> wss_uuid_map_;
	static Set<void*> ignored_ws_set_;
	static Mutex ws_uuid_map_mutex_;
};

}