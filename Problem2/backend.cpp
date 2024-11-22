#include "simdisk.h"
#include "shared_memory.h"
#include <windows.h>
#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;

int main() {
    const char* shmName = "SimDiskSharedMemory";
    const size_t shmSize = sizeof(SharedMemory);

    // Create shared memory
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, shmSize, shmName);

    if (!hMapFile) {
        cerr << "Error: Could not create shared memory: " << GetLastError() << '\n';
        return 1;
    }

    SharedMemory* shm = (SharedMemory*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, shmSize);

    if (!shm) {
        cerr << "Error: Could not map shared memory: " << GetLastError() << '\n';
        CloseHandle(hMapFile);
        return 1;
    }

    SimDisk disk;
    shm->ready = false;
    shm->done = false;

    cout << "Backend started. Waiting for commands...\n";

    while (true) {
        if (!shm->ready) {
            Sleep(100); // Wait for the frontend
            continue;
        }

        string command(shm->command);
        stringstream response;
        string cmd, arg1, arg2;
        istringstream ss(command);
        ss >> cmd >> arg1 >> ws;
        getline(ss, arg2);

        // Process commands
        if (cmd == "info") {
            disk.info(response);
        } 
        else if (cmd == "cd") {
            disk.cd(arg1, response);
        } 
        else if (cmd == "dir") {
            disk.dir(arg1, response);
        } 
        else if (cmd == "md") {
            disk.md(arg1, response);
        } 
        else if (cmd == "rd") {
            disk.rd(arg1, response);
        } 
        else if (cmd == "newfile") {
            disk.newfile(arg1, response);
        } 
        else if (cmd == "cat") {
            disk.cat(arg1, response);
        } 
        else if (cmd == "copy") {
            disk.copy(arg1, arg2, response);
        }   
        else if (cmd == "del") {
            disk.del(arg1, response);
        } 
        else if (cmd == "check") {
            disk.check(response);
        } 
        else if (cmd == "exit") {
            response << "Exiting backend.\n";
            strncpy(shm->result, response.str().c_str(), sizeof(shm->result));
            cout << response.str(); // Print to backend terminal
            shm->done = true;
            break;
        } 
        else {
            response << "Unknown command: " << cmd << "\n";
        }

        // Write result back to shared memory
        strncpy(shm->result, response.str().c_str(), sizeof(shm->result));
        cout << response.str(); // Print to backend terminal
        shm->done = true;
        shm->ready = false;
    }
}
