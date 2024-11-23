#include "simdisk.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <windows.h> // For CRITICAL_SECTION

using namespace std;

// Predefined users
vector<User> users = {
    {"admin", "admin123", Admin},
    {"editor", "edit123", Editor},
    {"viewer", "view123", Viewer}
};

// Authenticate user
User* login(const string& username, const string& password) {
    for (auto& user : users) {
        if (user.username == username && user.password == password) {
            return &user;
        }
    }
    return nullptr;
}

// SimDisk constructor
SimDisk::SimDisk() {
    superblock.totalBlocks = DISK_SIZE / BLOCK_SIZE;
    superblock.freeBlocks = superblock.totalBlocks;
    blockBitmap.resize(superblock.totalBlocks, true);
    dataBlocks.resize(superblock.totalBlocks, "");
    currentDir = "/";
    inodes["/"] = {"root", true, 0, {}};

    // Initialize CRITICAL_SECTION
    InitializeCriticalSection(&rwLock);
}

// SimDisk destructor
SimDisk::~SimDisk() {
    DeleteCriticalSection(&rwLock);
}

// Helper functions
int SimDisk::allocateBlock() {
    EnterCriticalSection(&rwLock);
    for (int i = 0; i < blockBitmap.size(); ++i) {
        if (blockBitmap[i]) {
            blockBitmap[i] = false;
            superblock.freeBlocks--;
            LeaveCriticalSection(&rwLock);
            return i;
        }
    }
    LeaveCriticalSection(&rwLock);
    return -1;
}

void SimDisk::freeBlock(int index) {
    EnterCriticalSection(&rwLock);
    if (index >= 0 && index < blockBitmap.size() && !blockBitmap[index]) {
        blockBitmap[index] = true;
        superblock.freeBlocks++;
    }
    LeaveCriticalSection(&rwLock);
}

INode* SimDisk::findINode(const string& path) {
    auto it = inodes.find(path);
    return (it != inodes.end()) ? &it->second : nullptr;
}

void SimDisk::deleteDirectoryContents(const string& dirPath) {
    for (auto it = inodes.begin(); it != inodes.end();) {
        // Check if the entry is within the directory being deleted
        if (it->first.find(dirPath + "/") == 0) {
            if (!it->second.isDirectory) {
                // Free the blocks allocated to the file
                for (int blockIndex : it->second.dataBlocks) {
                    freeBlock(blockIndex);
                }
            }
            it = inodes.erase(it); // Erase the entry
        } 
        else {
            ++it; // Move to the next entry
        }
    }
}


// Commands Implementation

void SimDisk::info(User* user, ostream& out) {
    if (user->role == Viewer || user->role == Editor || user->role == Admin) {
        EnterCriticalSection(&rwLock);
        out << "Disk Size: " << DISK_SIZE / (1024 * 1024) << " MB\n";
        out << "Block Size: " << BLOCK_SIZE << " Bytes\n";
        out << "Free Blocks: " << superblock.freeBlocks << '\n';
        out << "Used Blocks: " << superblock.totalBlocks - superblock.freeBlocks << '\n';
        out << "Root Directory: " << "/" << '\n';
        LeaveCriticalSection(&rwLock);
    } 
    else {
        out << "Error: Permission denied.\n";
    }
}

void SimDisk::cd(const string& path, User* user, ostream& out) {
    if (user->role == Viewer || user->role == Editor || user->role == Admin) {
        EnterCriticalSection(&rwLock);
        string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
        auto it = inodes.find(fullPath);

        if (it == inodes.end() || !it->second.isDirectory) {
            LeaveCriticalSection(&rwLock);
            out << "Error: Directory does not exist.\n";
            return;
        }

        currentDir = fullPath;
        LeaveCriticalSection(&rwLock);
        out << "Current directory changed to: " << currentDir << "\n";
    } 
    else {
        out << "Error: Permission denied.\n";
    }
}

void SimDisk::md(const string& path, User* user, ostream& out) {
    if (user->role == Viewer) {
        out << "Error: Permission denied.\n";
        return;
    }
    EnterCriticalSection(&rwLock);
    string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
    if (inodes.find(fullPath) != inodes.end()) {
        LeaveCriticalSection(&rwLock);
        out << "Error: Directory already exists.\n";
        return;
    }
    inodes[fullPath] = {fullPath, true, 0, {}};
    LeaveCriticalSection(&rwLock);
    out << "Directory created: " << fullPath << "\n";
}

void SimDisk::dir(const string& path, User* user, ostream& out) {
    if (user->role == Viewer || user->role == Editor || user->role == Admin) {
        EnterCriticalSection(&rwLock);
        string dirPath = path.empty() ? currentDir : path;
        if (inodes.find(dirPath) == inodes.end() || !inodes[dirPath].isDirectory) {
            LeaveCriticalSection(&rwLock);
            out << "Error: Directory does not exist.\n";
            return;
        }
        out << "Contents of " << dirPath << ":\n";
        for (const auto& pair : inodes) {
            if (pair.first.find(dirPath + "/") == 0 && pair.first != dirPath) {
                out << (pair.second.isDirectory ? "[DIR] " : "[FILE] ") << pair.first << "\n";
            }
        }
        LeaveCriticalSection(&rwLock);
    } 
    else {
        out << "Error: Permission denied.\n";
    }
}



void SimDisk::rd(const string& path, User* user, ostream& out) {
    // Check if the user is an Admin
    if (user->role != Admin) {
        out << "Error: Only admin users can delete directories.\n";
        return;
    }

    EnterCriticalSection(&rwLock); // Enter critical section
    string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;

    // Check if the directory exists
    auto it = inodes.find(fullPath);
    if (it == inodes.end() || !it->second.isDirectory) {
        out << "Error: Directory does not exist or is not a directory.\n";
        LeaveCriticalSection(&rwLock); // Exit critical section
        return;
    }

    // Delete the directory and its contents
    deleteDirectoryContents(fullPath);
    inodes.erase(it); // Remove the directory itself

    LeaveCriticalSection(&rwLock); // Exit critical section
    out << "Directory deleted: " << fullPath << '\n';
}

void SimDisk::newfile(const string& path, User* user, ostream& out) {
    if (user->role == Viewer) {
        out << "Error: Permission denied.\n";
        return;
    }
    EnterCriticalSection(&rwLock);
    string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
    if (inodes.find(fullPath) != inodes.end()) {
        LeaveCriticalSection(&rwLock);
        out << "Error: File already exists.\n";
        return;
    }
    int blockIndex = allocateBlock();
    if (blockIndex == -1) {
        LeaveCriticalSection(&rwLock);
        out << "Error: No free blocks available.\n";
        return;
    }
    inodes[fullPath] = {fullPath, false, BLOCK_SIZE, {blockIndex}};
    LeaveCriticalSection(&rwLock);
    out << "File created: " << fullPath << "\n";
}

void SimDisk::cat(const string& path, User* user, ostream& out) {
    if (user->role == Viewer || user->role == Editor || user->role == Admin) {
        EnterCriticalSection(&rwLock);
        string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
        INode* file = findINode(fullPath);
        if (!file || file->isDirectory) {
            LeaveCriticalSection(&rwLock);
            out << "Error: File does not exist or is a directory.\n";
            return;
        }
        out << "File content:\n";
        for (int blockIndex : file->dataBlocks) {
            out << dataBlocks[blockIndex];
        }
        out << "\n";
        LeaveCriticalSection(&rwLock);
    } 
    else {
        out << "Error: Permission denied.\n";
    }
}

void SimDisk::copy(const string& src, const string& dest, User* user, ostream& out) {
    if (user->role != Editor && user->role != Admin) {
        out << "Error: Only editors or admins can copy files.\n";
        return;
    }

    EnterCriticalSection(&rwLock);

    if (src.rfind("<host>", 0) == 0) {
        // Copy from host file system to simulated file system
        string hostPath = src.substr(6); // Remove "<host>" prefix
        ifstream inFile(hostPath, ios::binary);
        if (!inFile.is_open()) {
            LeaveCriticalSection(&rwLock);
            out << "Error: Source file not found or cannot be opened in host system: " << hostPath << '\n';
            return;
        }

        string fullDest = (dest[0] == '/') ? dest : currentDir + "/" + dest;
        if (inodes.find(fullDest) != inodes.end()) {
            LeaveCriticalSection(&rwLock);
            out << "Error: Destination file already exists in simulated file system: " << fullDest << '\n';
            return;
        }

        vector<int> allocatedBlocks;
        char buffer[BLOCK_SIZE];
        while (inFile.read(buffer, BLOCK_SIZE) || inFile.gcount() > 0) {
            int blockIndex = allocateBlock();
            if (blockIndex == -1) {
                LeaveCriticalSection(&rwLock);
                out << "Error: Not enough space in simulated file system.\n";
                for (int idx : allocatedBlocks) {
                    freeBlock(idx);
                }
                return;
            }
            allocatedBlocks.push_back(blockIndex);
            dataBlocks[blockIndex] = string(buffer, inFile.gcount());
        }

        inodes[fullDest] = {fullDest, false, (int)allocatedBlocks.size() * BLOCK_SIZE, allocatedBlocks};
        LeaveCriticalSection(&rwLock);
        out << "File successfully copied to simulated file system: " << fullDest << '\n';
    } 
    else if (dest.rfind("<host>", 0) == 0) {
        // Copy from simulated file system to host file system
        string fullSrc = (src[0] == '/') ? src : currentDir + "/" + src;
        INode* fileNode = findINode(fullSrc);
        if (!fileNode || fileNode->isDirectory) {
            LeaveCriticalSection(&rwLock);
            out << "Error: Source file does not exist or is a directory in simulated file system: " << fullSrc << '\n';
            return;
        }

        string hostPath = dest.substr(6); // Remove "<host>" prefix
        ofstream outFile(hostPath, ios::binary);
        if (!outFile) {
            LeaveCriticalSection(&rwLock);
            out << "Error: Unable to create destination file in host system: " << hostPath << '\n';
            return;
        }

        for (int blockIndex : fileNode->dataBlocks) {
            outFile.write(dataBlocks[blockIndex].c_str(), dataBlocks[blockIndex].size());
        }

        LeaveCriticalSection(&rwLock);
        out << "File successfully copied to host file system: " << hostPath << '\n';
    } 
    else {
        // Copy within simulated file system
        string fullSrc = (src[0] == '/') ? src : currentDir + "/" + src;
        INode* srcNode = findINode(fullSrc);
        if (!srcNode || srcNode->isDirectory) {
            LeaveCriticalSection(&rwLock);
            out << "Error: Source file does not exist or is a directory in simulated file system: " << fullSrc << '\n';
            return;
        }

        string fullDest = (dest[0] == '/') ? dest : currentDir + "/" + dest;
        if (inodes.find(fullDest) != inodes.end()) {
            LeaveCriticalSection(&rwLock);
            out << "Error: Destination file already exists in simulated file system: " << fullDest << '\n';
            return;
        }

        vector<int> allocatedBlocks;
        for (int blockIndex : srcNode->dataBlocks) {
            int newBlock = allocateBlock();
            if (newBlock == -1) {
                LeaveCriticalSection(&rwLock);
                out << "Error: Not enough space in simulated file system.\n";
                for (int idx : allocatedBlocks) {
                    freeBlock(idx);
                }
                return;
            }
            allocatedBlocks.push_back(newBlock);
            dataBlocks[newBlock] = dataBlocks[blockIndex];
        }

        inodes[fullDest] = {fullDest, false, srcNode->size, allocatedBlocks};
        LeaveCriticalSection(&rwLock);
        out << "File successfully copied within simulated file system: " << fullDest << '\n';
    }
}

void SimDisk::del(const string& path, User* user, ostream& out) {
    if (user->role != Admin) {
        out << "Error: Only editors or admins can delete files.\n";
        return;
    }

    EnterCriticalSection(&rwLock);
    string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;

    auto it = inodes.find(fullPath);
    if (it == inodes.end() || it->second.isDirectory) {
        LeaveCriticalSection(&rwLock);
        out << "Error: File does not exist or is a directory.\n";
        return;
    }

    for (int blockIndex : it->second.dataBlocks) {
        freeBlock(blockIndex);
    }
    inodes.erase(it);

    LeaveCriticalSection(&rwLock);
    out << "File deleted: " << fullPath << "\n";
}

void SimDisk::check(User* user, ostream& out) {
    if (user->role != Admin) {
        out << "Error: Only admin users can perform file system checks.\n";
        return;
    }
    EnterCriticalSection(&rwLock);
    out << "File system check complete.\n";
    out << "Used blocks: " << superblock.totalBlocks - superblock.freeBlocks << "\n";
    out << "Free blocks: " << superblock.freeBlocks << "\n";
    LeaveCriticalSection(&rwLock);
}
