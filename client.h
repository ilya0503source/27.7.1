#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "config.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

const int MESSAGE_LENGTH = 1024;
extern ConnectionSettings g_connectionSettings;

extern SOCKET socket_file_descriptor;
extern SOCKET connection;
extern struct sockaddr_in serveraddress;
extern char message[MESSAGE_LENGTH];

std::string processServerResponse(SOCKET socket_file_descriptor, const std::string& command = "");
void SendResponse(std::string response);
void StartClient();
void Cleanup();
void GracefulDisconnect();