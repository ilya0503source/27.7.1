#include "client.h"
#include "menu.h"
#include <iostream>
#include <cstdlib>


int main() {
    try {
        StartClient();
        try {
            ShowMainMenu();
            return 0;
        }
        catch (...) {
            std::cout << "error in main loop" << std::endl;
            GracefulDisconnect();
            return EXIT_FAILURE;
        }
    }
    catch (const std::exception& e) {
        std::cout << "error: " << e.what() << std::endl;
        GracefulDisconnect();
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cout << "Unknown error" << std::endl;
        GracefulDisconnect();
        return EXIT_FAILURE;
    }
}