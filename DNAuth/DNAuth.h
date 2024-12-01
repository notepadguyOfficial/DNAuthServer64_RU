#pragma once
#ifndef DNAUTH_H
#define DNAUTH_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>       // Includes basic Winsock2 API
#include <ws2tcpip.h>       // For TCP/IP extensions like getaddrinfo
#include <string>           // C++ standard library headers
#include <unordered_map>
#include <vector>
#include <sql.h>            // ODBC headers
#include <sqlext.h>

#pragma comment(lib, "Ws2_32.lib")

namespace DNAuthServer {
	
	struct ServerInfo {
		std::string m_LocalHost;
		int m_LocalPort;
		std::string m_RemoteHost;
		int m_RemotePort;
	};

	struct DBInfo {
		std::string m_DBHostName;
		int m_DBPort;
		std::string m_DBName;
		std::string m_DBUsername;
		std::string m_DBPassword;
	};

	class DNAuth {
	public:
		DNAuth(const ServerInfo& p_ServerInfo, const DBInfo& p_DBInfo);
		~DNAuth();

		void Start();
		static ServerInfo LoadServerInfo(const std::string& m_Path);
		static DBInfo LoadDBInfo(const std::string& m_Path);

	private:
		ServerInfo m_ServerInfo;
		DBInfo m_DBInfo;
		
		SOCKET m_ServerSocket;
		SOCKET m_ClientSocket;
		
		SQLHENV m_sqlEnv = nullptr;
		SQLHDBC m_sqlCon = nullptr;

		void InitializeWinSocket();
		void SetupSockets();
		void SetupDatabase();
		SOCKET CreateSocket(const std::string& p_Host, int p_Port, bool m_IsListener);
		void Run();
		void CleanUp();
		std::string ProcessData(const std::string& message);
	};
}

#endif // 
