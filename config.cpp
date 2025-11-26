#include "config.h"
#include <fstream>
#include <sstream>
#include <stdexcept>



ConnectionSettings LoadConnectionSettings(const std::string& filename) {
    ConnectionSettings settings;
    std::ifstream file(filename);


    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (key == "ip") {
            settings.ip = value;
        }
        else if (key == "port") {
            try {
                settings.port = std::stoi(value);
                if (settings.port < 1 || settings.port > 65535) {
                    throw std::invalid_argument("Port must be between 1 and 65535");
                }
            }
            catch (const std::exception&) {
                throw std::invalid_argument("Invalid port value: " + value);
            }
        }
    }

    file.close();

    if (settings.ip.empty()) {
        throw std::invalid_argument("IP address is not specified in config file");
    }
    if (settings.port == 0) {
        throw std::invalid_argument("Port is not specified in config file");
    }

    return settings;
}