#include "client.h"
#include "menu.h"
#include "config.h"
#include <shlwapi.h>

ConnectionSettings g_connectionSettings;
SOCKET socket_file_descriptor = INVALID_SOCKET;
SOCKET connection = INVALID_SOCKET;
struct sockaddr_in serveraddress;
char message[MESSAGE_LENGTH] = { 0 };




void Cleanup() {
    if (socket_file_descriptor != INVALID_SOCKET) {
        closesocket(socket_file_descriptor);
    }
    if (connection != INVALID_SOCKET) {
        closesocket(connection);
    }
    WSACleanup();
}
void Reconnect() {
    GracefulDisconnect();
    try {
        StartClient();
        std::cout << "Reconnected to the server successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Failed to reconnect: " << e.what() << std::endl;
    }
}

void GracefulDisconnect() {
    try {
        SendResponse("end;");
        Cleanup();
    }
    catch (...) {
        Cleanup();
    }
}

std::string processServerResponse(SOCKET socket_file_descriptor, const std::string& command) {
    char message[MESSAGE_LENGTH];
    memset(message, 0, MESSAGE_LENGTH);

    if (!command.empty()) {
        SendResponse(command);
    }

    int bytes_read = recv(socket_file_descriptor, message, MESSAGE_LENGTH - 1, 0);
    if (bytes_read <= 0) {
        std::cout << "Receive failed: " << WSAGetLastError() << std::endl;
        Cleanup();
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    message[bytes_read] = '\0';
    char* end = message + strlen(message);
    while (end > message && (*end == '\n' || *end == '\r' || *end == ' ')) {
        *end = '\0';
        end--;
    }
    std::string response = message;


    if (command == "showchat;") {

        if (response.substr(0, 9) != "showchat;") {
            std::cout << "Error: An incorrect message was received from the server" << std::endl;
            return "";
        }
        response = response.substr(9);
        std::vector<std::string> messages;
        std::stringstream ss(response);
        std::string token;

        while (getline(ss, token, ';')) {
            if (!token.empty()) {
                messages.push_back(token);
            }
        }
        for (const std::string& msg : messages) {
            std::cout << msg << std::endl;
        }
    }

    else if (command == "showusers;") {
        if (response.substr(0, 10) != "showusers;") {
            std::cout << "Error: An incorrect message was received from the server" << std::endl;
            return "";
        }
        response = response.substr(10);

        std::vector<std::string> users;
        std::stringstream ss(response);
        std::string token;

        while (getline(ss, token, ';')) {
            if (!token.empty()) {
                users.push_back(token);
            }
        }
        std::cout << std::endl << "addressees' names:" << std::endl;
        for (const std::string& user : users) {
            std::cout << user << std::endl;
        }
    }
    else {
        if (response == "You have successfully registered!") {
            std::cout << "You have successfully registered!" << std::endl;
        }
        else if (response == "The name cannot be all!") {
            std::cout << "The name cannot be all!" << std::endl;
        }
        else if (response == "The login already exists!") {
            std::cout << "The login already exists!" << std::endl;
        }
        else if (response == "Authorization is successful!") {
            std::cout << "Authorization is successful!" << std::endl;
        }
        else if (response == "User not found!") {
            std::cout << "User not found!" << std::endl;
        }
        else if (response == "Wrong password!") {
            std::cout << "Wrong password!" << std::endl;
        }
        else if (response == "Message delivered!") {
            std::cout << "Message delivered!" << std::endl;
        }
        else if (response == "name error!") {
            std::cout << "There is no user with that name" << std::endl;
        }
        else {
            std::cout << "Unknown response from server: " << response << std::endl;
        }
    }

    memset(message, 0, MESSAGE_LENGTH);
    return response;
}

void SendResponse(std::string response) {
    if (socket_file_descriptor == INVALID_SOCKET) {
        std::cout << "Socket is not initialized!" << std::endl;
        return;
    }

    int bytes = send(socket_file_descriptor, response.c_str(), static_cast<int>(response.length()), 0);
    if (bytes == SOCKET_ERROR) {
        std::cout << "Send failed: " << WSAGetLastError() << std::endl;
    }
    else {
        if (response == "end;") {
            std::cout << "Session terminated successfully." << std::endl;
        }
    }
}


std::string GetExePath() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    return std::string(path);
}

void StartClient() {
    try {
        std::string configPath = GetExePath() + "\\connection.ini";
        g_connectionSettings = LoadConnectionSettings(configPath);
    }
    catch (const std::exception& e) {
        std::cout << "Error loading config: " << e.what() << std::endl;
        Cleanup();
        exit(1);
    }

    WSADATA wsaData;
    int wsaErr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaErr != 0) {
        std::cout << "WSAStartup failed: " << wsaErr << std::endl;
        Cleanup();
        exit(1);
    }

    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_file_descriptor == INVALID_SOCKET) {
        std::cout << "Creation of Socket failed!" << std::endl;
        Cleanup();
        exit(1);
    }

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(g_connectionSettings.port);

    int result = inet_pton(AF_INET, g_connectionSettings.ip.c_str(), &serveraddress.sin_addr);
    if (result != 1) {
        std::cout << "Invalid IP address format: " << g_connectionSettings.ip << std::endl;
        Cleanup();
        exit(1);
    }

    connection = connect(socket_file_descriptor, (struct sockaddr*)&serveraddress, sizeof(serveraddress));
    if (connection == SOCKET_ERROR) {
        std::cout << "Connection with the server failed!" << std::endl;
        Cleanup();
        exit(1);
    }
}