#include <iostream>
#include <sstream>
#include <cstring>
#include <windows.h> // For Windows shared memory
#include "shared_memory.h"
#include "simdisk.h"

using namespace std;

int main() {
    // Create shared memory
    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(SharedMemory), "SharedMemory");
    if (hMapFile == nullptr) {
        cerr << "Error: Could not create shared memory. Error Code: " << GetLastError() << "\n";
        return 1;
    }

    // Map shared memory
    SharedMemory* shm = static_cast<SharedMemory*>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory)));
    if (shm == nullptr) {
        cerr << "Error: Could not map shared memory. Error Code: " << GetLastError() << "\n";
        CloseHandle(hMapFile);
        return 1;
    }

    // Initialize the SimDisk
    SimDisk simdisk;

    cout << "Backend is running and ready to process commands...\n";

    while (true) {
        // Wait for a command from the frontend
        if (shm->isCommandReady) {
            string command(shm->command);
            string username(shm->username);
            Role role = shm->role;

            // Create user for backend processing
            User user = {username, "", role};

            cout << "Processing command: " << command << " (User: " << username << ")\n";

            // Process the command
            stringstream result;
            stringstream ss(command);
            string cmd, arg1, arg2;

            ss >> cmd;

            if (cmd == "info") {
                simdisk.info(&user, result);
            } 
            else if (cmd == "cd") {
                ss >> arg1;
                simdisk.cd(arg1, &user, result);
            } 
            else if (cmd == "md") {
                ss >> arg1;
                simdisk.md(arg1, &user, result);
            } 
            else if (cmd == "dir") {
                ss >> arg1;
                simdisk.dir(arg1, &user, result);
            } 
            else if (cmd == "rd") {
                ostringstream response;
                ss >> arg1;

                // Process the directory deletion directly
                simdisk.rd(arg1, &user, response);

                // Send the final result back to the frontend
                strncpy(shm->result, response.str().c_str(), RESULT_SIZE - 1); // Prevent buffer overflow
                shm->result[RESULT_SIZE - 1] = '\0'; // Null-terminate the string

                shm->isResultReady = true;
                shm->isCommandReady = false;
            }
            else if (cmd == "newfile") {
                ss >> arg1;
                simdisk.newfile(arg1, &user, result);
            } 
            else if (cmd == "cat") {
                ss >> arg1;
                simdisk.cat(arg1, &user, result);
            } 
            else if (cmd == "copy") {
                ss >> arg1 >> arg2;
                simdisk.copy(arg1, arg2, &user, result);
            } 
            else if (cmd == "del") {
                ss >> arg1;
                simdisk.del(arg1, &user, result);
            } 
            else if (cmd == "check") {
                simdisk.check(&user, result);
            } 
            else if (cmd == "exit") {
                strncpy(shm->result, result.str().c_str(), RESULT_SIZE);
                shm->isResultReady = true;
                break;
            } 
            else {
                result << "Error: Unknown command.\n";
            }

            // Send the result back to the frontend
            strncpy(shm->result, result.str().c_str(), RESULT_SIZE);
            shm->isResultReady = true;

            // Reset command flag
            shm->isCommandReady = false;
        }
    }

    // Clean up
    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);

    cout << "Backend shutting down.\n";

    return 0;
}