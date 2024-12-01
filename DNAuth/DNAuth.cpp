#include "DNAuth.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <regex>
#include <numeric>
#include "MD5.h"
#include "CIniFile.h"

namespace DNAuthServer {
	DNAuth::DNAuth(const ServerInfo& p_ServerInfo, const DBInfo& p_DBInfo)
		: m_ServerInfo(p_ServerInfo), m_DBInfo(p_DBInfo)
	{
		InitializeWinSocket();
	}

	DNAuth::~DNAuth()
	{
		CleanUp();
	}

	void DNAuth::InitializeWinSocket()
	{
		WSADATA m_WSADATA;
		if (WSAStartup(MAKEWORD(2, 2), &m_WSADATA) != 0)
			throw std::runtime_error("WSAStartup failed!");
	}

	void DNAuth::SetupSockets()
	{
		m_ServerSocket = CreateSocket(m_ServerInfo.m_LocalHost, m_ServerInfo.m_LocalPort, true);
		m_ClientSocket = CreateSocket(m_ServerInfo.m_RemoteHost, m_ServerInfo.m_RemotePort, false);
	}

	void DNAuth::SetupDatabase()
	{
		std::wstringstream _temp;
		
		_temp << L"Driver={SQL Server};"
                     << L"Server=" << std::wstring(m_DBInfo.m_DBHostName.begin(), m_DBInfo.m_DBHostName.end())
                     << L"," << m_DBInfo.m_DBPort << L";"
                     << L"Database=" << std::wstring(m_DBInfo.m_DBName.begin(), m_DBInfo.m_DBName.end()) << L";"
                     << L"Uid=" << std::wstring(m_DBInfo.m_DBUsername.begin(), m_DBInfo.m_DBUsername.end()) << L";"
                     << L"Pwd=" << std::wstring(m_DBInfo.m_DBPassword.begin(), m_DBInfo.m_DBPassword.end()) << L";";

		std::wstring m_DBConStr = _temp.str();
		
		if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_sqlEnv) != SQL_SUCCESS) {
			throw std::runtime_error("Failed to allocate SQL environment.");
		}
		if (SQLSetEnvAttr(m_sqlEnv, SQL_ATTR_ODBC_VERSION, reinterpret_cast<void*>(SQL_OV_ODBC3), 0) != SQL_SUCCESS) {
			throw std::runtime_error("Failed to set ODBC version.");
		}
		if (SQLAllocHandle(SQL_HANDLE_DBC, m_sqlEnv, &m_sqlCon) != SQL_SUCCESS) {
			throw std::runtime_error("Failed to allocate SQL connection.");
		}
		if (SQLDriverConnect(m_sqlCon, NULL, (SQLWCHAR*)m_DBConStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE) != SQL_SUCCESS) {
			throw std::runtime_error("Failed to connect to the database.");
		}
	}

	SOCKET DNAuth::CreateSocket(const std::string& p_Host, int p_Port, bool m_IsListener)
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
			if (bind(m_Socket, reinterpret_cast<sockaddr*>(&m_Address), sizeof(m_Address)) == SOCKET_ERROR || listen(m_Socket, SOMAXCONN) == SOCKET_ERROR)
			{
				closesocket(m_Socket);
				throw std::runtime_error("Failed to bind or listen on socket. Error: " + std::to_string(WSAGetLastError()));
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

	void DNAuth::Run()
	{
		char buffer[20480];

		while (true)
		{
			fd_set m_ReadSet;
			FD_ZERO(&m_ReadSet);
			FD_SET(m_ServerSocket, &m_ReadSet);

			timeval timeout = { 1, 0 };
			if (select(0, &m_ReadSet, nullptr, nullptr, &timeout) > 0)
			{
				sockaddr_in p_ClientAddr;
				int p_AddrLen = sizeof(p_ClientAddr);
				SOCKET m_ClientConnection = accept(m_ServerSocket, (sockaddr*)&p_ClientAddr, &p_AddrLen);
				int m_BytesRecieved = recv(m_ClientConnection, buffer, sizeof(buffer), 0);
				
				if (m_BytesRecieved > 0)
				{
					std::string m_ProcessedMessage = ProcessData(std::string(buffer, m_BytesRecieved));
					send(m_ClientSocket, m_ProcessedMessage.c_str(), m_ProcessedMessage.size(), 0);
					int m_BytesFromServer = recv(m_ClientSocket, buffer, sizeof(buffer), 0);

					if (m_BytesFromServer > 0)
					{
						send(m_ClientConnection, buffer, m_BytesFromServer, 0);
					}
				}

				closesocket(m_ClientConnection);
			}
		}
	}

	void DNAuth::CleanUp()
	{
		if (m_ServerSocket != INVALID_SOCKET) closesocket(m_ServerSocket);
		if (m_ClientSocket != INVALID_SOCKET) closesocket(m_ClientSocket);
		if (m_sqlCon)
		{
			SQLDisconnect(m_sqlCon);
			SQLFreeHandle(SQL_HANDLE_DBC, m_sqlCon);
		}
		if (m_sqlEnv) SQLFreeHandle(SQL_HANDLE_ENV, m_sqlEnv);
		WSACleanup();
	}

	void DNAuth::Start()
	{
		SetupSockets();
		SetupDatabase();
		Run();
	}

	ServerInfo DNAuth::LoadServerInfo(const std::string& m_Path)
	{
		CIniFile iniFile(std::wstring(m_Path.begin(), m_Path.end()));
		ServerInfo info;
		info.m_LocalHost = iniFile.GetValue(L"Connection", L"LocalHost");
		info.m_LocalPort = iniFile.GetIntValue(L"Connection", L"LocalPort");
		info.m_RemoteHost = iniFile.GetValue(L"Connection", L"RemoteHost");
		info.m_RemotePort = iniFile.GetIntValue(L"Connection", L"RemotePort");
		return info;
	}

	DBInfo DNAuth::LoadDBInfo(const std::string& m_Path)
	{
		CIniFile iniFile(std::wstring(m_Path.begin(), m_Path.end()));
		DBInfo info;
		info.m_DBHostName = iniFile.GetValue(L"DB_DNMembership", L"DBIP");
		info.m_DBPort = iniFile.GetIntValue(L"DB_DNMembership", L"DBPort");
		info.m_DBName = iniFile.GetValue(L"DB_DNMembership", L"DBName");
		info.m_DBUsername = iniFile.GetValue(L"DB_DNMembership", L"DBID");
		info.m_DBPassword = iniFile.GetValue(L"DB_DNMembership", L"DBPassword");
		return info;
	}

	std::string DNAuth::ProcessData(const std::string& message)
	{
		std::vector<std::string> parts;
		std::regex re("'");
		std::copy(std::sregex_token_iterator(message.begin(), message.end(), re, -1),
			std::sregex_token_iterator(), std::back_inserter(parts));

		if (parts.size() == 10 && parts[5] == "I")
		{
			SQLHSTMT m_SQLHSTMT;
			SQLAllocHandle(SQL_HANDLE_STMT, m_sqlCon, &m_SQLHSTMT);

			std::string query = "SELECT * FROM Accounts WHERE AccountName = ? AND RLKTPassword = ?";
			SQLPrepare(m_SQLHSTMT, (SQLWCHAR*)query.c_str(), SQL_NTS);

			std::string password = md5(parts[3]);
			SQLBindParameter(m_SQLHSTMT, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, parts[2].size(), 0, (SQLCHAR*)parts[2].c_str(), 0, NULL);
			SQLBindParameter(m_SQLHSTMT, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, password.size(), 0, (SQLCHAR*)password.c_str(), 0, NULL);
		
			if (SQLExecute(m_SQLHSTMT) == SQL_SUCCESS && SQLFetch(m_SQLHSTMT) == SQL_SUCCESS) {
				parts[3] = "0";
			}
			else {
				parts[3] = "1";
			}

			SQLFreeHandle(SQL_HANDLE_STMT, m_SQLHSTMT);
		}

		return "'" + std::accumulate(parts.begin(), parts.end(), std::string(),
			[](const std::string& a, const std::string& b) { return a + "'" + b; });
	}
}