#pragma once
#include<iostream>
#include "message.h"
#include <fstream>
#include <unordered_map>
#include <cstring>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include "user.h"
#include "server.h"
#include "logger.h"
#include <mysql.h>


class Chat {
private:
    bool _chatRunning = false;
    std::unordered_map<std::string, std::shared_ptr<User>> _users;
    std::vector<std::shared_ptr<Message>> _messages;
    std::shared_ptr<User> _currentUser = nullptr;
    std::shared_ptr<User> getUserByLogin(const std::string& login) const;
    std::shared_ptr<User> getUserByName(const std::string& name) const;
    const std::string USERS_FILE = "users.dat";
    const std::string MESSAGES_FILE = "messages.dat";
    void loadUsersFromFile();
    void saveUsersToFile();
    void loadMessagesFromFile();
    void saveMessagesToFile();
    bool saveSingleMessage(const std::shared_ptr<Message>& message);

    bool saveUserToDb(const std::shared_ptr<User>& user);
    bool fetchUserId(const std::shared_ptr<User>& user);
    std::string getLoginById(int id);
    std::string getUserNameById(int id);

    bool executeQuery(const std::string& query);
    MYSQL_RES* getQueryResult(const std::string& query);

public:
    void start();
    bool isChatRunning() const { return _chatRunning; }
    std::shared_ptr<User> getCurrentUser() const { return _currentUser; }

    std::string signUp(const std::string& login, const std::string& password, const std::string& name);
    std::string Flogin(const std::string& login, const std::string& password);
    std::string sendMessage(const std::string& to, const std::string& text);

    std::string showAllUsersName();
    std::string showChat();

    void saveDataToFile();

    ~Chat() {
        for (auto& user : _users) {
            user.second.reset();
        }
        for (auto& message : _messages) {
            message.reset();
        }
    }
};