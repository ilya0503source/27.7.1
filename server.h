#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mysql.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib")

#define MESSAGE_LENGTH 1024
#define PORT 7777

extern bool shouldExit;
extern MYSQL* mysql;

void StartServ();
bool sendMessageToClient(SOCKET connection, const std::string& message);
void parseMessage(SOCKET connection, char* message);
void ListenServ();
void HandleClient(SOCKET clientSocket);
void GracefulShutdown();
bool connectToMySQL();
bool setupDatabase();

#endif // SERVER_H