#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <cstring>
#include <string>
#include "simdisk.h" // Use the existing enum Role from simdisk.h

const int SHARED_MEMORY_KEY = 65; // Unique key for shared memory
const int COMMAND_SIZE = 256;     // Maximum size for commands
const int RESULT_SIZE = 1024;     // Maximum size for results

struct SharedMemory {
    char command[COMMAND_SIZE];
    char result[RESULT_SIZE];
    char username[32];
    Role role;                     // Use the Role enum from simdisk.h
    bool isCommandReady;
    bool isResultReady;

    SharedMemory() : isCommandReady(false), isResultReady(false), role(Viewer) {
        std::memset(command, 0, sizeof(command));
        std::memset(result, 0, sizeof(result));
        std::memset(username, 0, sizeof(username));
    }
};

#endif // SHARED_MEMORY_H
