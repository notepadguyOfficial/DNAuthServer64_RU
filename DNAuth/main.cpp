#include "DNAuth.h"
#include <iostream>

int main(int argc, const char** argv[])
{
	try
	{
		std::string cFile = "Config\\DNAuth.ini";
		DNAuthServer::ServerInfo sInfo = DNAuthServer::DNAuth::LoadServerInfo(cFile);
		DNAuthServer::DBInfo dInfo = DNAuthServer::DNAuth::LoadDBInfo(cFile);

		DNAuthServer::DNAuth Auth(sInfo, dInfo);
		Auth.Start();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
	
	return 0;
}