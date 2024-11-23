#include <iostream>
#include <sstream>
#include <cstring>
#include <windows.h> // For Windows shared memory
#include "shared_memory.h"
#include <limits>

using namespace std;

// Function to login a user
bool login(SharedMemory* shm) {
    cout << "Enter username: ";
    string username;
    cin >> username;

    cout << "Enter password: ";
    string password;
    cin >> password;

    cin.ignore(1000, '\n');

    // Hardcoded users (should match backend definitions)
    if (username == "admin" && password == "admin123") {
        shm->role = Admin;
    } 
    else if (username == "editor" && password == "edit123") {
        shm->role = Editor;
    } 
    else if (username == "viewer" && password == "view123") {
        shm->role = Viewer;
    } 
    else {
        cout << "Error: Invalid username or password.\n";
        return false;
    }

    strncpy(shm->username, username.c_str(), sizeof(shm->username));
    cout << "Login successful! Role: "
         << (shm->role == Admin ? "Admin" : shm->role == Editor ? "Editor" : "Viewer") << "\n";

    return true;
}

int main() {
    // Open shared memory
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "SharedMemory");
    if (hMapFile == nullptr) {
        cerr << "Error: Could not open shared memory. Error Code: " << GetLastError() << "\n";
        return 1;
    }

    // Map shared memory
    SharedMemory* shm = static_cast<SharedMemory*>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory)));
    if (shm == nullptr) {
        cerr << "Error: Could not map shared memory. Error Code: " << GetLastError() << "\n";
        CloseHandle(hMapFile);
        return 1;
    }

    cout << "Frontend is running...\n";

    // User login
    while (!login(shm)) {
        cout << "Please try again.\n";
    }

    // Command loop
    while (true) {
        // Display prompt and get user input
        cout << "simdisk> ";
        string command;
        getline(cin, command);

        // Write the command to shared memory
        strncpy(shm->command, command.c_str(), COMMAND_SIZE);
        shm->isCommandReady = true;

        // Wait for the backend to process the command
        while (!shm->isResultReady) {
            // Busy wait
        }

        // Display the result from the backend
        cout << shm->result;

        // Reset result flag
        shm->isResultReady = false;

        // Exit the frontend if the "exit" command was issued
        if (command == "exit") {
            break;
        }
    }


    // Clean up
    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);

    cout << "Frontend shutting down.\n";

    return 0;
}