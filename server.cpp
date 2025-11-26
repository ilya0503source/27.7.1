#include "server.h"
#include "chat.h"

bool shouldExit = false;
MYSQL* mysql = nullptr;

struct sockaddr_in serveraddress, client;
int length;
SOCKET socket_file_descriptor = INVALID_SOCKET;
SOCKET connection = INVALID_SOCKET;
char message[MESSAGE_LENGTH];
Chat chat;

extern Logger logger;

bool setupDatabase() {
    MYSQL* tmpMysql = mysql_init(NULL);
    if (!tmpMysql) {
        logger.asyncLogAndRead("MySQL init failed in setupDatabase");
        return false;
    }
    if (!mysql_real_connect(tmpMysql, "localhost", "root", "root", NULL, 3306, NULL, 0)) {
        const char* mysqlErr = mysql_error(tmpMysql);
        std::string errMsg(mysqlErr ? mysqlErr : "");
        logger.asyncLogAndRead(errMsg);
        mysql_close(tmpMysql);
        return false;
    }
    std::string checkDbQuery = "SHOW DATABASES LIKE 'chatdb'";
    if (mysql_query(tmpMysql, checkDbQuery.c_str())) {
        const char* mysqlErr = mysql_error(tmpMysql);
        std::string errMsg(mysqlErr ? mysqlErr : "");
        logger.asyncLogAndRead(errMsg);
        mysql_close(tmpMysql);
        return false;
    }
    MYSQL_RES* res = mysql_store_result(tmpMysql);
    bool dbExists = (res && mysql_num_rows(res) > 0);
    if (res) {
        mysql_free_result(res);
    }
    if (!dbExists) {
        std::string createDbQuery = "CREATE DATABASE chatdb";
        if (mysql_query(tmpMysql, createDbQuery.c_str())) {
            const char* mysqlErr = mysql_error(tmpMysql);
            std::string errMsg(mysqlErr ? mysqlErr : "");
            logger.asyncLogAndRead(errMsg);
            mysql_close(tmpMysql);
            return false;
        }
        logger.asyncLogAndRead("Database 'chatdb' created successfully");
    }

    if (mysql_select_db(tmpMysql, "chatdb")) {
        const char* mysqlErr = mysql_error(tmpMysql);
        std::string errMsg(mysqlErr ? mysqlErr : "");
        logger.asyncLogAndRead(errMsg);
        mysql_close(tmpMysql);
        return false;
    }

    std::string createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INT AUTO_INCREMENT PRIMARY KEY,
            login VARCHAR(50) NOT NULL UNIQUE,
            password VARCHAR(255) NOT NULL,
            name VARCHAR(50) NOT NULL
        )
    )";
    if (mysql_query(tmpMysql, createUsersTable.c_str())) {
        const char* mysqlErr = mysql_error(tmpMysql);
        std::string errMsg(mysqlErr ? mysqlErr : "");
        logger.asyncLogAndRead(errMsg);
        mysql_close(tmpMysql);
        return false;
    }

    std::string createMessagesTable = R"(
    CREATE TABLE IF NOT EXISTS messages (
        id INT AUTO_INCREMENT PRIMARY KEY,
        sender_id INT NOT NULL,
        receiver_id INT NOT NULL,
        text TEXT NOT NULL,
        CONSTRAINT fk_sender FOREIGN KEY (sender_id) REFERENCES users(id),
        CONSTRAINT fk_receiver FOREIGN KEY (receiver_id) REFERENCES users(id)
    )
)";

    if (mysql_query(tmpMysql, createMessagesTable.c_str())) {
        const char* mysqlErr = mysql_error(tmpMysql);
        std::string errMsg(mysqlErr ? mysqlErr : "");
        logger.asyncLogAndRead(errMsg);
        mysql_close(tmpMysql);
        return false;
    }

    std::string checkUserQuery = "SELECT id FROM users WHERE id = -1";
    if (mysql_query(tmpMysql, checkUserQuery.c_str())) {
        const char* mysqlErr = mysql_error(tmpMysql);
        std::string errMsg(mysqlErr ? mysqlErr : "");
        logger.asyncLogAndRead(errMsg);
        mysql_close(tmpMysql);
        return false;
    }

    res = mysql_store_result(tmpMysql);
    bool userExists = (res && mysql_num_rows(res) > 0);
    if (res) {
        mysql_free_result(res);
    }

    if (!userExists) {
        std::string insertUserQuery =
            "INSERT INTO users (id, login, password, name) VALUES (-1, 'all', '', 'all')";
        if (mysql_query(tmpMysql, insertUserQuery.c_str())) {
            const char* mysqlErr = mysql_error(tmpMysql);
            std::string errMsg(mysqlErr ? mysqlErr : "");
            logger.asyncLogAndRead(errMsg);
            mysql_close(tmpMysql);
            return false;
        }
        logger.asyncLogAndRead("Virtual user 'all' (id=-1) added to users table");
    }

    mysql_close(tmpMysql);
    return true;
}

bool connectToMySQL() {
    mysql = mysql_init(NULL);
    if (!mysql) {
        logger.asyncLogAndRead("MySQL init failed");
        return false;
    }

    if (!mysql_real_connect(mysql, "localhost", "root", "root", "chatdb", 3306, NULL, 0)) {
        const char* mysqlErr = mysql_error(mysql);
        std::string errMsg(mysqlErr ? mysqlErr : "");
        logger.asyncLogAndRead(errMsg);
        mysql_close(mysql);
        return false;
    }

    std::string checkQuery = "SELECT id FROM users WHERE id = -1 AND login = 'all'";
    if (mysql_query(mysql, checkQuery.c_str())) {
        const char* mysqlErr = mysql_error(mysql);
        std::string errMsg(mysqlErr ? mysqlErr : "");
        logger.asyncLogAndRead(errMsg);
        return false;
    }

    MYSQL_RES* res = mysql_store_result(mysql);
    if (res && mysql_num_rows(res) == 0) {
        std::string insertQuery = "INSERT INTO users (id, login, password, name) VALUES (-1, 'all', '', 'all')";
        if (mysql_query(mysql, insertQuery.c_str())) {
            const char* mysqlErr = mysql_error(mysql);
            std::string errMsg(mysqlErr ? mysqlErr : "");
            logger.asyncLogAndRead(errMsg);
            mysql_free_result(res);
            return false;
        }
        logger.asyncLogAndRead("Virtual user 'all' created with id = -1");
    }
    if (res) {
        mysql_free_result(res);
    }

    return true;
}

void StartServ() {
    if (!setupDatabase()) {
        exit(1);
    }

    if (!connectToMySQL()) {
        exit(1);
    }

    chat.start();
    logger.asyncLogAndRead("Chat initialized");

    WSADATA wsaData;
    int wsaErr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaErr != 0) {
        logger.asyncLogAndRead("WSAStartup failed");
        exit(1);
    }

    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_file_descriptor == INVALID_SOCKET) {
        logger.asyncLogAndRead("Socket creation failed");
        WSACleanup();
        exit(1);
    }

    serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddress.sin_port = htons(PORT);
    serveraddress.sin_family = AF_INET;

    int bind_status = bind(socket_file_descriptor, (struct sockaddr*)&serveraddress, sizeof(serveraddress));
    if (bind_status == SOCKET_ERROR) {
        logger.asyncLogAndRead("Bind failed");
        closesocket(socket_file_descriptor);
        WSACleanup();
        exit(1);
    }

    int connection_status = listen(socket_file_descriptor, 5);
    if (connection_status == SOCKET_ERROR) {
        logger.asyncLogAndRead("Listen failed");
        closesocket(socket_file_descriptor);
        WSACleanup();
        exit(1);
    }
    else {
        logger.asyncLogAndRead("Server is listening for new connections");
    }

    length = sizeof(client);
}

bool sendMessageToClient(SOCKET connection, const std::string& message) {
    int bytes = send(connection, message.c_str(), static_cast<int>(message.size()) + 1, 0);
    
    if (bytes != SOCKET_ERROR) {
        logger.asyncLogAndRead("Data successfully sent to the client");
        return true;
    }
    else {
        logger.asyncLogAndRead("Error sending data");
        return false;
    }
}

void parseMessage(SOCKET connection, char* message) {
    if (!message || strlen(message) == 0) {
        return;
    }

    std::stringstream ss(message);
    std::string token;
    std::vector<std::string> values;

    while (std::getline(ss, token, ';')) {
        if (!token.empty()) {
            values.push_back(token);
        }
    }

    if (values.empty()) {
        return;
    }

    if (values[0] == "signup" && values.size() >= 4) {
        std::string login = values[1];
        std::string password = values[2];
        std::string name = values[3];
        logger.asyncLogAndRead("The command was requested: signup");
        std::string response = chat.signUp(login, password, name);
        sendMessageToClient(connection, response);
    }
    else if (values[0] == "login" && values.size() >= 3) {
        std::string login = values[1];
        std::string password = values[2];
        logger.asyncLogAndRead("The command was requested: login");
        std::string response = chat.Flogin(login, password);
        sendMessageToClient(connection, response);
    }
    else if (values[0] == "sendmessage" && values.size() >= 3) {
        std::string to = values[1];
        std::string text = values[2];
        logger.asyncLogAndRead("The command was requested: sendmessage");
        std::string response = chat.sendMessage(to, text);
        sendMessageToClient(connection, response);
    }
    else if (values[0] == "showusers") {
        logger.asyncLogAndRead("The command was requested: showusers");
        std::string response = chat.showAllUsersName();
        sendMessageToClient(connection, response);
    }
    else if (values[0] == "showchat") {
        logger.asyncLogAndRead("The command was requested: showchat");
        std::string response = chat.showChat();
        sendMessageToClient(connection, response);
    }
}

void HandleClient(SOCKET clientSocket) {
    char message[MESSAGE_LENGTH];

    while (!shouldExit) {
        memset(message, 0, MESSAGE_LENGTH);

        int bytesRead = recv(clientSocket, message, sizeof(message), 0);

        if (bytesRead <= 0) {
            std::cout << "Client disconnected" << std::endl;
            logger.asyncLogAndRead("Client disconnected");
            break;
        }

        if (strncmp(message, "end;", 4) == 0) {
            logger.asyncLogAndRead("Received end command from client");
            sendMessageToClient(clientSocket, "Session terminated successfully");
            break;
        }

        parseMessage(clientSocket, message);
    }

    closesocket(clientSocket);
}

void ListenServ() {
    while (!shouldExit) {
        SOCKET clientSocket = accept(socket_file_descriptor, (struct sockaddr*)&client, &length);

        if (clientSocket == INVALID_SOCKET) {
            int lastError = WSAGetLastError();
            if (lastError == WSAECONNABORTED || lastError == WSAEINTR) {
                continue;
            }
            std::cout << "Accept failed: " << lastError << std::endl;
            logger.asyncLogAndRead("Accept failed");
            continue;
        }
        logger.asyncLogAndRead("New connection");
        try {
            HandleClient(clientSocket);
        }
        catch (const std::exception& e) {
            logger.asyncLogAndRead("Exception in client processing");
        }
        catch (...) {
            logger.asyncLogAndRead("Unknown exception in client processing");
        }

        if (shouldExit) {
            break;
        }
    }
}

void GracefulShutdown() {
    try {
        chat.saveDataToFile();
        logger.asyncLogAndRead("Data saved successfully");
    }
    catch (const std::exception& e) {
        logger.asyncLogAndRead("Error saving data");
    }

    if (mysql) {
        mysql_close(mysql);
    }

    if (socket_file_descriptor != INVALID_SOCKET) {
        closesocket(socket_file_descriptor);
        socket_file_descriptor = INVALID_SOCKET;
    }

    WSACleanup();
    logger.asyncLogAndRead("The server is stopped");
}