#include "logger.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <shared_mutex>
#include <thread>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

Logger::Logger() {
    const std::string logDir = "LOG";

    if (!std::filesystem::exists(logDir)) {
        try {
            std::filesystem::create_directory(logDir);
            std::cout << "Log directory created: " << logDir << std::endl;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cout << "Error creating directory: " << e.what() << std::endl;
            isOpen = false;
            return;
        }
    }

    fileStream.open(logFilePath, std::ios::app);
    if (fileStream.is_open()) {
        isOpen = true;
        writeLine("Logger initialized. Log file: " + logFilePath);
    }
    else {
        isOpen = false;
        std::cout << "Error: Could not open log file: " << logFilePath << std::endl;
    }
}

Logger::~Logger() {
    if (isOpen) {
        writeLine("Logger shutting down.");
        fileStream.close();
    }
}

bool Logger::checkFileOpen() const {
    if (!isOpen) {
        std::cout << "Error: Log file is not open." << std::endl;
        return false;
    }
    return true;
}

void Logger::writeLine(const std::string& line) {
    if (!checkFileOpen()) return;

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::tm tm = {};
    if (localtime_s(&tm, &time_t) != 0) {
        fileStream << "????-??-?? ??:??:?? " << line << std::endl;
        fileStream.flush();
        return;
    }

    std::ostringstream timeStream;
    timeStream << std::setfill('0')
        << (tm.tm_year + 1900) << "-"
        << std::setw(2) << (tm.tm_mon + 1) << "-"
        << std::setw(2) << tm.tm_mday << " "
        << std::setw(2) << tm.tm_hour << ":"
        << std::setw(2) << tm.tm_min << ":"
        << std::setw(2) << tm.tm_sec;

    fileStream << timeStream.str() << " " << line << std::endl;
    fileStream.flush();
}

void Logger::readLastLine() const {
    if (!checkFileOpen()) return;

    std::ifstream input(logFilePath);
    if (!input.is_open()) {
        std::cout << "Error: Could not open file to read last line." << std::endl;
        return;
    }

    std::string line;
    std::string lastLine;

    while (std::getline(input, line)) {
        if (!line.empty()) {
            lastLine = line;
        }
    }

    input.close();

    if (!lastLine.empty()) {
        std::cout << lastLine << std::endl;
    }
    else {
        std::cout << "Log file is empty." << std::endl;
    }
}

void Logger::asyncLogAndRead(const std::string& message) {

    std::thread writer([&, msg = message]() {
        logMutex.lock();
        writeLine(msg);
        logMutex.unlock();
        });

    writer.join();

    std::thread reader([&]() {
        logMutex.lock_shared();
        readLastLine();
        logMutex.unlock_shared();
        });

    reader.join();
}

bool Logger::isFileOpen() const {
    return isOpen;
}