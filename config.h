#pragma once
#include <string>

struct ConnectionSettings {
    std::string ip;
    int port;
};

ConnectionSettings LoadConnectionSettings(const std::string& filename);
