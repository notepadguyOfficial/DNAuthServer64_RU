# DNAuthServer

DNAuthServer is a lightweight authentication server designed to handle secure connections between a client application and a remote server. It features a modular design for managing socket communication and database interactions. The project is built in C++ and leverages the Winsock API for network operations and ODBC for database connectivity.

## Configuration File
Create a file named `DNAuth.ini` in the `Config` directory with the following structure:

```ini
[Connection]
LocalHost=127.0.0.1
LocalPort=8080
RemoteHost=192.168.1.100
RemotePort=9090

[DB_DNMembership]
DBIP=127.0.0.1
DBPort=1433
DBName=DNAuthDB
DBID=sa
DBPassword=your_password
```

