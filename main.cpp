#include "server.h"
#include <iostream>
#include <csignal>
#include <string>
#include "chat.h"

Logger logger;



void signalHandler(int signal) {
    std::cout << "A shutdown signal has been received (" << signal << ")" << std::endl;
    logger.asyncLogAndRead("A shutdown signal has been received");
    shouldExit = true;
    std::cout << shouldExit;
}

int main() {

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    StartServ();
    try {
        while (!shouldExit) {
            ListenServ();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "An error has occurred: " << e.what() << std::endl;
        logger.asyncLogAndRead("An error has occurred");
    }
    catch (...) {
        std::cerr << "An unknown error has occurred" << std::endl;
        logger.asyncLogAndRead("An unknown error has occurred");
    }

    GracefulShutdown();
    return 0;
}