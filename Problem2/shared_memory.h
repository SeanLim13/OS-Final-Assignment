#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

struct SharedMemory {
    char command[1024];  // Command buffer from the frontend
    char result[4096];   // Result buffer from the backend
    bool ready;          // Indicates the command is ready for processing
    bool done;           // Indicates the backend has finished processing
};

#endif // SHARED_MEMORY_H