#pragma once
#include <string>
#include <memory>
#include "logger.h"

class User {
private:
    int _id = 0;
    const std::string _login;
    std::string _password;
    std::string _name;

public:
    User(const std::string& login, const std::string& password, const std::string& name)
        : _login(login), _password(password), _name(name) {
    }

    int getId() const { return _id; }
    void setId(int id) { _id = id; }

    const std::string& getLogin() const { return _login; }
    const std::string& getPassword() const { return _password; }
    void setPassword(const std::string& password) { _password = password; }
    const std::string& getName() const { return _name; }
    void setName(const std::string& name) { _name = name; }
};