// PacketRcvr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

SOCKET m_ServerSocket;
SOCKET m_ClientSocket;

void InitializeWinSocket()
{
	WSADATA m_WSADATA;
	int result = WSAStartup(MAKEWORD(2, 2), &m_WSADATA);

	if (result != 0)
		throw std::runtime_error("WSAStartup failed with error: " + std::to_string(result));
}

SOCKET CreateSocket(const std::string& p_Host, int p_Port, bool m_IsListener)
{
	SOCKET m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_Socket == INVALID_SOCKET)
		throw std::runtime_error("Socket creation failed with error: " + std::to_string(WSAGetLastError()));

	sockaddr_in m_Address = {};
	m_Address.sin_family = AF_INET;
	m_Address.sin_port = htons(p_Port);
	inet_pton(AF_INET, p_Host.c_str(), &m_Address.sin_addr);

	if (m_IsListener)
	{
		if (bind(m_Socket, reinterpret_cast<sockaddr*>(&m_Address), sizeof(m_Address)) == SOCKET_ERROR)
		{
			closesocket(m_Socket);
			throw std::runtime_error("Socket bind failed with error: " + std::to_string(WSAGetLastError()));
		}

		if (listen(m_Socket, SOMAXCONN) == SOCKET_ERROR)
		{
			closesocket(m_Socket);
			throw std::runtime_error("Socket listen failed with error: " + std::to_string(WSAGetLastError()));
		}
	}
	else
	{
		if (connect(m_Socket, reinterpret_cast<sockaddr*>(&m_Address), sizeof(m_Address)) == SOCKET_ERROR)
		{
			closesocket(m_Socket);
			throw std::runtime_error("Socket connect failed with error: " + std::to_string(WSAGetLastError()));
		}
	}

	return m_Socket;
}

void SetupSockets()
{
	m_ServerSocket = CreateSocket("127.0.0.1", 6001, true);
	m_ClientSocket = CreateSocket("127.0.0.1", 5412, false);
}

void Run() {
	char buffer[20480];
	while (true) {
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(m_ServerSocket, &readSet);

		timeval timeout = { 1, 0 }; // 1 second timeout
		int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
		if (selectResult > 0 && FD_ISSET(m_ServerSocket, &readSet)) {
			sockaddr_in clientAddr;
			int addrLen = sizeof(clientAddr);
			SOCKET clientConn = accept(m_ServerSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
			if (clientConn == INVALID_SOCKET) {
				std::cerr << "Failed to accept client connection: " << WSAGetLastError() << std::endl;
				continue;
			}

			int bytesReceived = recv(clientConn, buffer, sizeof(buffer), 0);
			if (bytesReceived > 0) {
				std::cout << "Received from client: " << std::string(buffer, bytesReceived) << std::endl;
			}
			closesocket(clientConn);
		}
	}
}

int main()
{
	SetupSockets();
	Run();

	return 0;
}