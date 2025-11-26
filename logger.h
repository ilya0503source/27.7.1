#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <shared_mutex>
#include <thread>

class Logger {
public:
    Logger();
    ~Logger();

    void writeLine(const std::string& line);
    void readLastLine() const;
    void asyncLogAndRead(const std::string& message);
    bool isFileOpen() const;

private:
    bool checkFileOpen() const;

    std::fstream fileStream;
    bool isOpen;
    const std::string logFilePath = "LOG/log.txt";
    mutable std::shared_mutex logMutex;
};

#endif // LOGGER_H