#include "chat.h"
#include <iostream>
#include <sstream>
#include <algorithm>



extern MYSQL* mysql;
extern Logger logger;


void Chat::start() {
    _chatRunning = true;
    loadUsersFromFile();
    loadMessagesFromFile();
}

std::shared_ptr<User> Chat::getUserByLogin(const std::string& login) const {
    auto it = _users.find(login);
    return (it != _users.end()) ? it->second : nullptr;
}

std::shared_ptr<User> Chat::getUserByName(const std::string& name) const {
    for (const auto& user : _users) {
        if (user.second->getName() == name) {
            return user.second;
        }
    }
    return nullptr;
}

void Chat::loadUsersFromFile() {
    _users.clear();

    std::string query = "SELECT id, login, password, name FROM users";
    MYSQL_RES* res = getQueryResult(query);
    if (!res) return;

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        int id = std::stoi(row[0]);
        std::string login = row[1];
        std::string password = row[2];
        std::string name = row[3];

        auto user = std::make_shared<User>(login, password, name);
        user->setId(id);
        _users[login] = user;
    }

    mysql_free_result(res);
}

void Chat::saveUsersToFile() {
    for (const auto& user : _users) {
        char* escapedLogin = new char[user.second->getLogin().length() * 2 + 1];
        char* escapedPassword = new char[user.second->getPassword().length() * 2 + 1];
        char* escapedName = new char[user.second->getName().length() * 2 + 1];

        mysql_real_escape_string(mysql, escapedLogin, user.second->getLogin().c_str(), user.second->getLogin().length());
        mysql_real_escape_string(mysql, escapedPassword, user.second->getPassword().c_str(), user.second->getPassword().length());
        mysql_real_escape_string(mysql, escapedName, user.second->getName().c_str(), user.second->getName().length());

        std::string query = "INSERT INTO users (login, password, name) VALUES ('" +
            std::string(escapedLogin) + "', '" +
            std::string(escapedPassword) + "', '" +
            std::string(escapedName) + "') "
            "ON DUPLICATE KEY UPDATE password=VALUES(password), name=VALUES(name)";

        executeQuery(query);


        delete[] escapedLogin;
        delete[] escapedPassword;
        delete[] escapedName;
    }
}

void Chat::loadMessagesFromFile() {
    _messages.clear();

    std::string query = "SELECT sender_id, receiver_id, text FROM messages";
    MYSQL_RES* res = getQueryResult(query);
    if (!res) return;

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        int from = std::stoi(row[0]);
        int to = std::stoi(row[1]);
        std::string text = row[2];

        _messages.push_back(std::make_shared<Message>(from, to, text));
    }
    mysql_free_result(res);
}

void Chat::saveMessagesToFile() {
    for (const auto& message : _messages) {
        int from = message->getFrom();
        int to = message->getTo();
        std::string text = message->getText();


        char* escapedText = new char[text.length() * 2 + 1];
        mysql_real_escape_string(mysql, escapedText, text.c_str(), text.length());

        std::string checkQuery = "SELECT COUNT(*) FROM messages WHERE sender_id = " +
            std::to_string(from) + " AND receiver_id = " + std::to_string(to) +
            " AND text = '" + std::string(escapedText) + "'";

        MYSQL_RES* checkRes = getQueryResult(checkQuery);
        bool exists = false;
        if (checkRes) {
            MYSQL_ROW row = mysql_fetch_row(checkRes);
            exists = (atoi(row[0]) > 0);
            mysql_free_result(checkRes);
        }

        if (!exists) {
            std::string insertQuery = "INSERT INTO messages (sender_id, receiver_id, text) VALUES (" +
                std::to_string(from) + ", " + std::to_string(to) + ", '" +
                std::string(escapedText) + "')";
            executeQuery(insertQuery);
        }
        delete[] escapedText;
    }
}

bool Chat::saveSingleMessage(const std::shared_ptr<Message>& message) {
    int from = message->getFrom();
    int to = message->getTo();
    std::string text = message->getText();


    char* escapedText = new char[text.length() * 2 + 1];
    mysql_real_escape_string(mysql, escapedText, text.c_str(), text.length());

    std::string query = "INSERT INTO messages (sender_id, receiver_id, text) VALUES (" +
        std::to_string(from) + ", " + std::to_string(to) + ", '" +
        std::string(escapedText) + "')";

    bool success = executeQuery(query);
    delete[] escapedText;
    return success;
}

bool Chat::saveUserToDb(const std::shared_ptr<User>& user) {
    char* escapedLogin = new char[user->getLogin().length() * 2 + 1];
    char* escapedPassword = new char[user->getPassword().length() * 2 + 1];
    char* escapedName = new char[user->getName().length() * 2 + 1];

    mysql_real_escape_string(mysql, escapedLogin, user->getLogin().c_str(), user->getLogin().length());
    mysql_real_escape_string(mysql, escapedPassword, user->getPassword().c_str(), user->getPassword().length());
    mysql_real_escape_string(mysql, escapedName, user->getName().c_str(), user->getName().length());


    std::string query = "INSERT INTO users (login, password, name) VALUES ('" +
        std::string(escapedLogin) + "', '" +
        std::string(escapedPassword) + "', '" +
        std::string(escapedName) + "')";

    bool success = executeQuery(query);

    delete[] escapedLogin;
    delete[] escapedPassword;
    delete[] escapedName;

    return success;
}

bool Chat::fetchUserId(const std::shared_ptr<User>& user) {
    std::string query = "SELECT id FROM users WHERE login = '" + user->getLogin() + "'";
    MYSQL_RES* res = getQueryResult(query);
    if (!res) return false;

    if (MYSQL_ROW row = mysql_fetch_row(res)) {
        user->setId(std::stoi(row[0]));
        mysql_free_result(res);
        return true;
    }

    mysql_free_result(res);
    return false;
}

std::string Chat::getLoginById(int id) {
    std::string query = "SELECT login FROM users WHERE id = " + std::to_string(id);
    MYSQL_RES* res = getQueryResult(query);
    if (!res) return "";

    if (MYSQL_ROW row = mysql_fetch_row(res)) {
        mysql_free_result(res);
        return row[0];
    }

    mysql_free_result(res);
    return "";
}

bool Chat::executeQuery(const std::string& query) {
    if (mysql_query(mysql, query.c_str())) {
        const char* mysqlErr = mysql_error(mysql);
        std::string errMsg(mysqlErr ? mysqlErr : "");
        logger.asyncLogAndRead(errMsg);
        return false;
    }
    return true;
}

MYSQL_RES* Chat::getQueryResult(const std::string& query) {
    if (!executeQuery(query)) {
        return nullptr;
    }
    return mysql_store_result(mysql);
}

std::string Chat::signUp(const std::string& login, const std::string& password, const std::string& name) {
    if (getUserByLogin(login)) {
        logger.asyncLogAndRead("The login already exists!");
        return "The login already exists!";
    }
    if (name == "all") {
        logger.asyncLogAndRead("The name cannot be all!");
        return "The name cannot be all!";
    }

    auto newUser = std::make_shared<User>(login, password, name);

    if (!saveUserToDb(newUser)) {
        logger.asyncLogAndRead("Failed to save user to database");
        return "Registration failed";
    }

    if (!fetchUserId(newUser)) {
        logger.asyncLogAndRead("Failed to retrieve user id");
        return "Registration failed";
    }

    _users[login] = newUser;
    _currentUser = newUser;

    logger.asyncLogAndRead(login + " successfully registered!");
    return "You have successfully registered!";
}

std::string Chat::Flogin(const std::string& login, const std::string& password) {
    auto user = getUserByLogin(login);
    if (!user) {
        logger.asyncLogAndRead("User not found!");
        return "User not found!";
    }

    if (user->getPassword() != password) {
        logger.asyncLogAndRead("Wrong password!");
        return "Wrong password!";
    }

    _currentUser = user;
    logger.asyncLogAndRead("Authorization is successful!");
    return "Authorization is successful!";
}

std::string Chat::sendMessage(const std::string& to, const std::string& text) {
    std::shared_ptr<User> recipient = nullptr;

    if (!(to == "all" || (recipient = getUserByName(to)))) {
        logger.asyncLogAndRead("The user with the name " + to + " does not exist");
        return "name error";
    }

    int senderId = _currentUser->getId();
    int receiverId = (to == "all") ? -1 : recipient->getId();

    auto newMessage = std::make_shared<Message>(senderId, receiverId, text);
    _messages.push_back(newMessage);

    if (saveSingleMessage(newMessage)) {
        logger.asyncLogAndRead("Message delivered!");
        return "Message delivered!";
    }
    else {
        logger.asyncLogAndRead("Failed to save message!");
        return "Failed to save message!";
    }
}

std::string Chat::showAllUsersName() {
    loadUsersFromFile();
    std::stringstream message;
    message << "showusers;";

    bool isFirst = true;

    for (const auto& user : _users) {
        if (!isFirst) {
            message << "; ";
        }

        std::string login = user.second->getLogin();

        if (_currentUser && _currentUser->getLogin() == login) {
            login += " (You)";
        }

        message << login;
        isFirst = false;
    }

    return message.str();
}

std::string Chat::showChat() {
    std::stringstream message;
    loadMessagesFromFile();

    message << "showchat;";

    bool isFirst = true;

    for (const auto& chatMessage : _messages) {
        int senderId = chatMessage->getFrom();
        int receiverId = chatMessage->getTo();

        std::string fromLogin = getLoginById(senderId);
        std::string toLogin = (receiverId == -1) ? "all" : getLoginById(receiverId);

        std::string fromDisplay;
        std::string toDisplay;

        if (_currentUser && _currentUser->getId() == senderId) {
            fromDisplay = "You";
        }
        else {
            fromDisplay = fromLogin;
        }

        if (receiverId == -1) {
            toDisplay = "All";
        }
        else if (_currentUser && _currentUser->getId() == receiverId) {
            toDisplay = "You";
        }
        else {
            toDisplay = toLogin;
        }

        bool isSender = (_currentUser && _currentUser->getId() == senderId);
        bool isReceiver = (_currentUser && _currentUser->getId() == receiverId);
        bool isForAll = (receiverId == -1);

        if (!(isSender || isReceiver || isForAll)) {
            continue;
        }

        std::string msg = fromDisplay + " -> " + toDisplay + ": " + chatMessage->getText();

        if (!isFirst) {
            message << "; ";
        }
        message << msg;
        isFirst = false;
    }

    return message.str();
}

void Chat::saveDataToFile() {
    saveUsersToFile();
    saveMessagesToFile();
}

std::string Chat::getUserNameById(int id) {
    std::string query = "SELECT name FROM users WHERE id = " + std::to_string(id);
    MYSQL_RES* res = getQueryResult(query);
    if (!res) return "";

    if (MYSQL_ROW row = mysql_fetch_row(res)) {
        mysql_free_result(res);
        return row[0];
    }

    mysql_free_result(res);
    return "";
}