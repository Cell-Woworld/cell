#pragma once
#include "Socket.h"
#include "SecureSocket.h"
#include <thread>

namespace TCPsocket
{
	class AcceptedSocket
	{
	private:
		ASocket::Socket socket_;
		bool Disconnect() const;
	public:
		AcceptedSocket():socket_(0){
		};
		~AcceptedSocket() { Disconnect(); };
		ASocket::Socket& socket() {
			return socket_;
		}
		/* ret > 0   : bytes received
		 * ret == 0  : connection closed
		 * ret < 0   : recv failed
		 */
		int Receive(char* pData,
			const size_t uSize,
			bool bReadFully /*= true*/) const;

		bool Send(const char* pData, size_t uSize) const;
		bool Send(const std::string& strData) const;
		bool Send(const std::vector<char>& Data) const;
	};
}