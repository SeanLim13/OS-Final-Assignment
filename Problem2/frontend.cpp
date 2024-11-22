#include "shared_memory.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <cstring>

using namespace std;

int main() {
    const char* shmName = "SimDiskSharedMemory";
    const size_t shmSize = sizeof(SharedMemory);

    // Open shared memory
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, shmName);

    if (!hMapFile) {
        cerr << "Error: Could not open shared memory: " << GetLastError() << endl;
        return 1;
    }

    SharedMemory* shm = (SharedMemory*)MapViewOfFile(
        hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, shmSize);

    if (!shm) {
        cerr << "Error: Could not map shared memory: " << GetLastError() << endl;
        CloseHandle(hMapFile);
        return 1;
    }

    cout << "Frontend shell started. Type commands to interact with the backend.\n";

    while (true) {
        cout << "simdisk> ";
        string command;
        getline(cin, command);

        if (command.empty()) continue;

        strncpy(shm->command, command.c_str(), sizeof(shm->command));
        shm->ready = true;
        shm->done = false;

        while (!shm->done) {
            Sleep(100); // Wait for the backend to process the command
        }

        if (command == "exit"){
            cout << "Exiting frontend.\n";
            break;
        }
        
        shm->ready = false;
        shm->done = false;
    }

    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);

    return 0;
}