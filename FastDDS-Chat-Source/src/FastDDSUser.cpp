// FastDDSUser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "UserChatPublisher.hpp"
#include "UserChatSubscriber.hpp"
#include "Globals.hpp"

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <fstream>

// For colors
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <cstdio>
#endif

#undef max

void setTextColor(Color color) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, static_cast<int>(color));
#else
    std::cout << "\033[" << static_cast<int>(color) << "m";
#endif
}

void resetTextColor() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 7);
#else
    std::cout << "\033[0m";
#endif
}

//std::vector<std::string> endThreadSignal = {};  // Lets threads know to end
std::vector<std::string> curr_chat_tab = {};    // Tells which tabbed user is currently being talked to (option 3)

// Class for representing a Subscriber
class sub_thread {
private:
    UserChatSubscriber* user_sub;
    std::string sub_topic;
    std::thread st;
    std::vector<std::string>* curr_history;
    std::vector<std::string>* curr_tab;

public:
    sub_thread(std::string sub_topic, std::vector<std::string>& history, std::vector<std::string>& tab) : curr_history(&history), curr_tab(&tab) {
        this->sub_topic = sub_topic;
        user_sub = new UserChatSubscriber(sub_topic, curr_history, curr_tab);
        user_sub->init();
        st = std::thread(&sub_thread::run, this, sub_topic);
    }

    void run(std::string topic) {
        user_sub->run(topic);
    }

    UserChatSubscriber* getSub() {
        return user_sub;
    }

    std::thread* getThread() {
        return &st;
    }

    std::vector<std::string>* getHistory() {
        return curr_history;
    }
};

// Class for representing a Publisher
class pub_thread {
private:
    UserChatPublisher* user_pub;
    std::string pub_topic;
    std::thread pt;
    std::vector<std::string>* curr_history;

public:
    pub_thread(std::string pub_topic, std::string name, std::vector<std::string>& history) : curr_history(&history) {
        this->pub_topic = pub_topic;
        user_pub = new UserChatPublisher(pub_topic, name, curr_history);
        user_pub->init();
        pt = std::thread(&pub_thread::run, this);
    }

    void run() {
        user_pub->run();
    }

    UserChatPublisher* getPub() {
        return user_pub;
    }

    std::thread* getThread() {
        return &pt;
    }

    std::vector<std::string>* getHistory() {
        return curr_history;
    }
};

// Find index of element in vector
int findIndex(std::vector<std::string> vector, std::string search) {
    auto f = std::find(vector.begin(), vector.end(), search);

    if (f != vector.end()) {
        return f - vector.begin();
    }
    else {
        return -1;
    }
}

// View users currently added
void viewUsers(std::vector<std::string>& threaded_usernames, std::vector<pub_thread>& pubs) {
    std::cout << std::endl << "These are the users you are currently connected to:" << std::endl;

    int i = 0;
    for (std::string& user : threaded_usernames) {
        bool curr_status = pubs.at(i).getPub()->getStatus();
        std::string str = "";

        if (curr_status) str = "online";
        else str = "offline";

        std::cout << "  " + user + " (" + str + ")" << std::endl;

        i++;
    }
}

// Add new user
void addUser(std::vector<pub_thread>& pubs, std::vector<sub_thread>& subs, std::vector<std::string>& threaded_usernames, std::string username, std::vector<std::vector<std::string>>& chat_histories) {
    std::string new_user = "";

    while (true) {
        std::cout << std::endl << "Enter new user: ";
        std::getline(std::cin, new_user);

        if (new_user.find(" ") != std::string::npos) {
            std::cout << "Enter a valid username." << std::endl;

            new_user = "";
        }
        else {
            if (new_user == username) {
                std::cout << "You can't add yourself. Try again." << std::endl;
            }
            else if (findIndex(threaded_usernames, new_user) != -1) {
                std::cout << "You can't add a user you already added." << std::endl;
            }
            else {
                break;
            }
        }
    }

    std::vector<std::string> temp_history = {};
    chat_histories.push_back(temp_history);

    pub_thread pub(username + "_" + new_user, username, chat_histories.at(chat_histories.size()-1));
    sub_thread sub(new_user + "_" + username, chat_histories.at(chat_histories.size() - 1), curr_chat_tab);

    pubs.push_back(std::move(pub));
    subs.push_back(std::move(sub));

    threaded_usernames.push_back(new_user);

    std::cout << "Successfully added " + new_user + "." << std::endl;
}

// Remove user
void removeUser(std::vector<pub_thread>& pubs, std::vector<sub_thread>& subs, std::vector<std::string>& threaded_usernames, std::string removed_user, std::string username, std::vector<std::vector<std::string>>& chat_histories) {
    int index = findIndex(threaded_usernames, removed_user);

    if (index == -1) {
        std::cout << "Error: User was not found." << std::endl;
        return;
    }

    threaded_usernames.erase(threaded_usernames.begin() + index);

    std::string temp_pub_topic = username + "_" + removed_user;
    std::string temp_sub_topic = removed_user + "_" + username;

    endThreadSignal.push_back(temp_pub_topic);
    endThreadSignal.push_back(temp_sub_topic);

    if (pubs.at(index).getThread()->joinable()) {
        pubs.at(index).getThread()->join();
    }

    if (subs.at(index).getThread()->joinable()) {
        subs.at(index).getThread()->join();
    }

    delete pubs.at(index).getPub();
    delete subs.at(index).getSub();

    pubs.erase(pubs.begin() + index);
    subs.erase(subs.begin() + index);

    chat_histories.erase(chat_histories.begin() + index);

    endThreadSignal.clear();

    std::cout << removed_user + " has been successfully removed." << std::endl;
}

// Home Menu
void printHomeMenu() {
    std::cout << std::endl << "Choose an option from below:" << std::endl;
    std::cout << "  1. View list of currently connected users." << std::endl;
    std::cout << "  2. Add a new user." << std::endl;
    std::cout << "  3. Chat with a user." << std::endl;
    std::cout << "  4. Remove a user." << std::endl;
    std::cout << "  5. Save chat log." << std::endl;
    std::cout << "  6. Change color of text." << std::endl;
    std::cout << "  7. Exit the program." << std::endl << std::endl;
}

// Get login info
void getCredentials(std::string& username) {
    std::cout << "Please enter your login info below:" << std::endl << std::endl;

    while (true) {
        std::cout << "Username: ";
        std::cin >> username;
        std::cin.ignore();
        std::cout << std::endl;

        if (username == "Notes") {
            std::cout << "\"Notes\" can't be used as a name in order to share functionality with the GUI version. Try again." << std::endl << std::endl;
        }
        else if (username.length() < 3) {
            std::cout << "Your username should be at least 3 characters long. Try again." << std::endl << std::endl;
        }
        else if (username.length() > 32) {
            std::cout << "Your username should be at most 32 characters long. Try again." << std::endl << std::endl;
        }
        else  break;
    }

    std::cout << std::endl;
}

void chatUser(std::string username, std::string other_user, std::vector<std::string> threaded_usernames, std::vector<pub_thread>& pubs, std::vector<sub_thread>& subs, std::vector<std::vector<std::string>>& chat_histories) {
    int index = findIndex(threaded_usernames, other_user);

    if (index == -1) {
        std::cout << "Invalid username." << std::endl;
        return;
    }

    std::cout << std::endl << "Here's your current history with " + other_user + ":" << std::endl;

    std::vector<std::string>* temp_history = pubs.at(index).getHistory();

    if (!temp_history->empty()) {
        for (std::string& str : *temp_history) {
            std::cout << str << std::endl;
        }
    }
    else {
        std::cout << "This is the start of your history with " + other_user + "." << std::endl;
    }

    std::cout << std::endl;

    curr_chat_tab.at(0) = "in";
    curr_chat_tab.at(1) = other_user + "_" + username;
    pubs.at(index).getPub()->setActive(true);   // Allows typing messages in Publisher
    while (pubs.at(index).getPub()->getActive() == true) {
    }

    curr_chat_tab.at(0) = "";
    curr_chat_tab.at(1) = "";

    std::cout << "Leaving chat with " + other_user + "." << std::endl;
}

void saveChat(std::string username, std::vector<std::string> threaded_usernames, std::vector<std::vector<std::string>> chat_histories) {
    std::string saveUser;
    std::cout << std::endl << "Which ongoing chat would you like to save?: ";

    std::cin >> saveUser;
    std::cin.ignore();

    int index = findIndex(threaded_usernames, saveUser);

    if (index == -1) {
        std::cout << "You aren't currently chatting with a user named " << saveUser << "." << std::endl;
        return;
    }

    std::vector<std::string> curr_history = chat_histories.at(index);

    if (curr_history.empty()) {
        std::cout << "You've added " << saveUser << ", but you haven't chatted with them yet." << std::endl;
        return;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* timeInfo = std::localtime(&currentTime);

    std::ostringstream dateStream;
    dateStream << std::put_time(timeInfo, "%m-%d-%y");
    std::string date = dateStream.str();
    
    std::ostringstream timeStream;
    timeStream << std::put_time(timeInfo, "%H-%M-%S");
    std::string time = timeStream.str();

    // Format - Username_ChattedUser_MM-DD-YY_HH-MM-SS.txt
    std::string filename = "./ChatLogs/" + username + "_" + saveUser + "_" + date + "_" + time + ".txt";
    
    std::ofstream chatLog(filename);

    if (!chatLog) {
        std::cerr << "Error creating chat log file." << std::endl;
        return;
    }

    for (std::string& str : curr_history) {
        chatLog << str << std::endl;
    }

    chatLog.close();

    std::cout << "File " << filename << " was created." << std::endl;
}

void changeColor() {
    std::cout << std::endl << "Which color would you like to choose?" << std::endl;

    setTextColor(Color::DEFAULT_WHITE);
    std::cout << "  1. Default White" << std::endl;
    //setTextColor(Color::BLACK);
    //std::cout << "  2. Black" << std::endl;
    setTextColor(Color::BLUE);
    std::cout << "  2. Blue" << std::endl;
    setTextColor(Color::GREEN);
    std::cout << "  3. Green" << std::endl;
    setTextColor(Color::CYAN);
    std::cout << "  4. Cyan" << std::endl;
    setTextColor(Color::RED);
    std::cout << "  5. Red" << std::endl;
    setTextColor(Color::MAGENTA);
    std::cout << "  6. Magenta" << std::endl;
    setTextColor(Color::YELLOW);
    std::cout << "  7. Yellow" << std::endl;
    setTextColor(Color::WHITE);
    std::cout << "  8. White" << std::endl;

    resetTextColor();

    int option = 0;
    std::cout << std::endl << "Select an option: ";
    std::cin >> option;
    std::cin.ignore();

    std::cout << std::endl;

    switch (option) {
    case 1:
        setTextColor(Color::DEFAULT_WHITE);
        std::cout << "You're display is now set to Default White" << std::endl;
        break;
    case 2:
        setTextColor(Color::BLUE);
        std::cout << "You're display is now set to Blue" << std::endl;
        break;
    case 3:
        setTextColor(Color::GREEN);
        std::cout << "You're display is now set to Green" << std::endl;
        break;
    case 4:
        setTextColor(Color::CYAN);
        std::cout << "You're display is now set to Cyan" << std::endl;
        break;
    case 5:
        setTextColor(Color::RED);
        std::cout << "You're display is now set to Red" << std::endl;
        break;
    case 6:
        setTextColor(Color::MAGENTA);
        std::cout << "You're display is now set to Magenta" << std::endl;
        break;
    case 7:
        setTextColor(Color::YELLOW);
        std::cout << "You're display is now set to Yellow" << std::endl;
        break;
    case 8:
        setTextColor(Color::WHITE);
        std::cout << "You're display is now set to White" << std::endl;
        break;
    default:
        std::cout << "That's not a color." << std::endl;
    }
}

int main()
{
    curr_chat_tab.push_back("");
    curr_chat_tab.push_back("");

    std::vector<pub_thread> pubs = {};
    std::vector<sub_thread> subs = {};
    std::vector<std::vector<std::string>> chat_histories = {};

    std::vector<std::string> threaded_usernames = {};

    std::string username = "";

    std::cout << "Fast-DDS Chat" << std::endl;
    std::cout << "----------------------------" << std::endl;

    getCredentials(username);

    std::cout << "----------------------------" << std::endl << std::endl;

    std::cout << "Welcome, " + username + "." << std::endl;

    while (true) {
        int option = -1;
        printHomeMenu();

        while (true) {
            std::cin >> option;
            std::cin.ignore();

            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input. Please try again: ";
            }
            else {
                break;
            }
        }

        if (option == 1) {
            if (!threaded_usernames.empty()) {
                viewUsers(threaded_usernames, pubs);
            }
            else {
                std::cout << std::endl << "You have no Users added yet." << std::endl;
            }
        }
        else if (option == 2) {
            addUser(pubs, subs, threaded_usernames, username, chat_histories);
        }
        else if (option == 3) {
            std::string to_chat = "";

            std::cout << std::endl << "What user would you like to chat to: ";
            std::cin >> to_chat;
            std::cin.ignore();

            if (to_chat == username) {
                std::cout << "Invalid username: can't talk to yourself." << std::endl;
            }
            else {
                chatUser(username, to_chat, threaded_usernames, pubs, subs, chat_histories);
            }
        }
        else if (option == 4) {
            std::string to_remove = "";

            std::cout << std::endl << "What user would you like to remove: ";
            std::cin >> to_remove;
            std::cin.ignore();

            removeUser(pubs, subs, threaded_usernames, to_remove, username, chat_histories);
        }
        else if (option == 5) saveChat(username, threaded_usernames, chat_histories);
        else if (option == 6) changeColor();
        else if (option == 7) break;
        else {
            std::cout << std::endl << "That's not an option. Try again. (1-5)" << std::endl;
        }
    }

    // Clean up threads
    for (int i = 0; i < pubs.size(); i++) {
        std::string removed_user = threaded_usernames.at(i);

        std::string temp_pub_topic = username + "_" + removed_user;
        std::string temp_sub_topic = removed_user + "_" + username;

        endThreadSignal.push_back(temp_pub_topic);
        endThreadSignal.push_back(temp_sub_topic);

        pubs.at(i).getThread()->join();
        subs.at(i).getThread()->join();
    }

    std::cout << std::endl << "Thanks for chatting." << std::endl;

    resetTextColor();
}