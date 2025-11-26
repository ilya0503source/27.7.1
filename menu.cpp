#define NOMINMAX
#include "menu.h"


void ShowAllMessages() {
    std::cout << std::endl << "All messages:" << std::endl;
    processServerResponse(socket_file_descriptor, "showchat;");

    bool flag3 = true;
    while (flag3) {
        std::cout << std::endl << "To exit, press - 0" << std::endl;
        int exit = 0;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin >> exit;

        if (exit == 0) {
            flag3 = false;
        }
        else {
            std::cout << "The command was not recognized" << std::endl;
        }
    }
}

std::string SendMessage() {
    std::string to, text;

    std::cout << std::endl << "Select the recipient or enter \"all\" to send the message to everyone: ";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, to);

    if (to.empty()) {
        std::cout << "Error: Addressee is empty!" << std::endl;
        return "";
    }

    std::cout << std::endl << "Write a message:" << std::endl;
    std::getline(std::cin, text);

    if (text.empty()) {
        std::cout << "Error: Message is empty!" << std::endl;
        return "";
    }

    std::string response = "sendmessage;" + to + ";" + text;
    return response;
}

void ShowUserMenu() {
    bool flag1 = true;
    while (flag1) {
        std::string response;
        std::cout << std::endl << "select an action: 1 - show all messages, 2 - send a message, 3 - go to the main menu" << std::endl;
        int usermenu = 0;
        std::cin >> usermenu;
        switch (usermenu) {
        case 1:
            ShowAllMessages();
            break;
        case 2:
            processServerResponse(socket_file_descriptor, "showusers;");
            response = SendMessage();
            SendResponse(response.c_str());
            processServerResponse(socket_file_descriptor, "");
            break;
        case 3:
            flag1 = false;
            break;
        default:
            std::cout << "the command was not recognized..." << std::endl;
        }
    }
}

std::string ShowLoginMenu() {
    std::string login, password;
    std::cout << "Enter your login and password" << std::endl;
    std::cout << "login:";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin >> login;
    std::cout << "password:";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin >> password;
    std::string response = "login;" + login + ";" + password;
    return response;
}

std::string ShowRegisterMenu() {
    std::string login, password, name;
    std::cout << "Enter your login, password and name" << std::endl;
    std::cout << "login:";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin >> login;
    std::cout << "password:";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin >> password;
    std::cout << "name:";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin >> name;
    std::string response = "signup;" + login + ";" + password + ";" + name;
    return response;
}

void ShowMainMenu() {
    bool flag2 = true;
    while (flag2) {
        std::string response, message, answer;
        std::cout << std::endl << "Select an action: 1 - login, 2 - register, 3 - exit" << std::endl;
        int mainmenu = 0;
        std::cin >> mainmenu;
        switch (mainmenu) {
        case 1:
            response = ShowLoginMenu();
            SendResponse(response);
            answer = processServerResponse(socket_file_descriptor, "");
            if (answer == "Authorization is successful!") {
                ShowUserMenu();
            }
            break;

        case 2:
            response = ShowRegisterMenu();
            SendResponse(response);
            answer = processServerResponse(socket_file_descriptor, "");
            break;
        case 3:
            GracefulDisconnect();
            std::cout << "Client Exit." << std::endl;
            flag2 = false;
            break;
        default:
            std::cout << "Invalid command!" << std::endl;
        }
    }
}