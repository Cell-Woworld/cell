#pragma once
#include "AcceptedSocket.h"

namespace TCPsocket
{
	int AcceptedSocket::Receive(char* pData,
		const size_t uSize,
		bool bReadFully /*= true*/) const {
		if (socket_ == 0 || !pData || !uSize)
			return -1;

#ifdef WINDOWS
		int tries = 0;
#endif

		int total = 0;
		do {
			int nRecvd = recv(socket_, pData + total, (int)uSize - total, 0);

			if (nRecvd == 0) {
				// peer shut down
				break;
			}

#ifdef WINDOWS
			if ((nRecvd < 0) && (WSAGetLastError() == WSAENOBUFS))
			{
				// On long messages, Windows recv sometimes fails with WSAENOBUFS, but
				// will work if you try again.
				if ((tries++ < 1000))
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}

				//if (m_eSettingsFlags & ENABLE_LOG)
				//	m_oLog("[TCPServer][Error] Socket error in call to recv.");

				break;
			}
#endif

			total += nRecvd;

		} while (bReadFully && (total < uSize));

		return (int)total;
	}

	bool AcceptedSocket::Send(const char* pData, size_t uSize) const {
		if (socket_ < 0 || !pData || !uSize)
			return false;

		int total = 0;
		do {
			const int flags = 0;
			int nSent;

			nSent = send(socket_, pData + total, (int)uSize - total, flags);

			if (nSent < 0) {
				//if (m_eSettingsFlags & ENABLE_LOG)
				//	m_oLog("[TCPServer][Error] Socket error in call to send.");

				return false;
			}
			total += nSent;
		} while (total < uSize);

		return true;
	}

	bool AcceptedSocket::Send(const std::string& strData) const {
		return Send(strData.c_str(), strData.length());
	}

	bool AcceptedSocket::Send(const std::vector<char>& Data) const {
		return Send(Data.data(), Data.size());
	}

	bool AcceptedSocket::Disconnect() const {
#ifdef WINDOWS
		// The shutdown function disables sends or receives on a socket.
		int iResult = shutdown(socket_, SD_RECEIVE);

		if (iResult == SOCKET_ERROR)
		{
			//if (m_eSettingsFlags & ENABLE_LOG)
			//	m_oLog(StringFormat("[TCPServer][Error] shutdown failed : %d", WSAGetLastError()));

			return false;
		}

		closesocket(socket_);
#else

		close(socket_);

#endif
		return true;
	}
}