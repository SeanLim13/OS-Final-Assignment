#include "simdisk.h"
#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;

SimDisk::SimDisk() {
    superblock.totalBlocks = DISK_SIZE / BLOCK_SIZE;
    superblock.freeBlocks = superblock.totalBlocks;
    blockBitmap.resize(superblock.totalBlocks, true); // All blocks initially free
    dataBlocks.resize(superblock.totalBlocks, "");   // Initialize data blocks
    currentDir = "/";
    inodes["/"] = {"root", true, 0, {}};             // Create root directory
    cout << "SimDisk initialized.\n";
}

int SimDisk::allocateBlock() {
    for (int i = 0; i < blockBitmap.size(); ++i) {
        if (blockBitmap[i]) {
            blockBitmap[i] = false;
            superblock.freeBlocks--;
            return i;
        }
    }
    return -1; // No free blocks available
}

void SimDisk::freeBlock(int index) {
    if (index >= 0 && index < blockBitmap.size() && !blockBitmap[index]) {
        blockBitmap[index] = true;
        superblock.freeBlocks++;
    }
}

INode* SimDisk::findINode(const string& path) {
    auto it = inodes.find(path);
    return (it != inodes.end()) ? &it->second : nullptr;
}

void SimDisk::info(ostream& out) {
    out << "Disk Size: " << DISK_SIZE / (1024 * 1024) << " MB\n";
    out << "Block Size: " << BLOCK_SIZE << " Bytes\n";
    out << "Free Blocks: " << superblock.freeBlocks << '\n';
    out << "Used Blocks: " << superblock.totalBlocks - superblock.freeBlocks << '\n';
    out << "Root Directory: " << "/" << '\n';
}

void SimDisk::cd(const string& path, ostream& out) {
    if (inodes.find(path) != inodes.end() && inodes[path].isDirectory) {
        currentDir = path;
        out << "Changed directory to: " << path << '\n';
    } 
    else {
        out << "Error: Directory does not exist.\n";
    }
}

void SimDisk::md(const string& path, ostream& out) {
    string fullPath = (path[0] == '/') ? path : currentDir + path;
    if (inodes.find(fullPath) != inodes.end()) {
        out << "Error: Directory already exists.\n";
        return;
    }
    inodes[fullPath] = {fullPath, true, 0, {}};
    out << "Directory created: " << fullPath << '\n';
}

void SimDisk::dir(const string& path, ostream& out) {
    string dirPath = path.empty() ? currentDir : path;
    if (inodes.find(dirPath) == inodes.end() || !inodes[dirPath].isDirectory) {
        out << "Error: Directory does not exist.\n";
        return;
    }
    out << "Contents of " << dirPath << ":\n";
    for (const auto& pair : inodes) {
        const string& key = pair.first;
        const INode& inode = pair.second;
        if (key.find(dirPath + "/") == 0 && key != dirPath) {
            out << (inode.isDirectory ? "[DIR] " : "[FILE] ") << key << '\n';
        }
    }
}

void SimDisk::rd(const string& path, ostream& out) {
    auto it = inodes.find(path);
    if (it != inodes.end() && it->second.isDirectory) {
        deleteDirectoryContents(path);
        inodes.erase(it);
        out << "Directory deleted: " << path << '\n';
    } else {
        out << "Error: Directory does not exist or is not a directory.\n";
    }
}

void SimDisk::deleteDirectoryContents(const string& dirPath) {
    for (auto it = inodes.begin(); it != inodes.end();) {
        if (it->first.find(dirPath + "/") == 0) {
            if (!it->second.isDirectory) {
                for (int blockIndex : it->second.dataBlocks) {
                    freeBlock(blockIndex);
                }
            }
            it = inodes.erase(it);
        } 
        else {
            ++it;
        }
    }
}

void SimDisk::newfile(const string& path, ostream& out) {
    string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
    if (inodes.find(fullPath) != inodes.end()) {
        out << "Error: File already exists.\n";
        return;
    }
    int blockIndex = allocateBlock();
    if (blockIndex == -1) {
        out << "Error: No free blocks available.\n";
        return;
    }
    inodes[fullPath] = {fullPath, false, BLOCK_SIZE, {blockIndex}};
    out << "File created: " << fullPath << '\n';
}

void SimDisk::cat(const string& path, ostream& out) {
    INode* fileNode = findINode(path);
    if (fileNode && !fileNode->isDirectory) {
        out << "File content:\n";
        for (int blockIndex : fileNode->dataBlocks) {
            out << dataBlocks[blockIndex];
        }
        out << '\n';
    } 
    else {
        out << "Error: File does not exist or is a directory.\n";
    }
}

void SimDisk::copy(const string& src, const string& dest, ostream& out) {
    if (src.rfind("<host>", 0) == 0) {
        string hostPath = src.substr(6); // Remove "<host>" prefix
        ifstream inFile(hostPath, ios::binary);
        if (!inFile.is_open()) {
            out << "Error: Source file not found or cannot be opened in host system: " << hostPath << '\n';
            return;
        }

        string fullDest = (dest[0] == '/') ? dest : currentDir + "/" + dest;
        if (inodes.find(fullDest) != inodes.end()) {
            out << "Error: Destination file already exists in simulated file system: " << fullDest << '\n';
            return;
        }

        vector<int> allocatedBlocks;
        char buffer[BLOCK_SIZE];
        while (inFile.read(buffer, BLOCK_SIZE) || inFile.gcount() > 0) {
            int blockIndex = allocateBlock();
            if (blockIndex == -1) {
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
        out << "File successfully copied to simulated file system: " << fullDest << '\n';
    } 
    else if (dest.rfind("<host>", 0) == 0) {
        string fullSrc = (src[0] == '/') ? src : currentDir + "/" + src;
        INode* fileNode = findINode(fullSrc);
        if (!fileNode || fileNode->isDirectory) {
            out << "Error: Source file does not exist or is a directory in simulated file system: " << fullSrc << '\n';
            return;
        }

        string hostPath = dest.substr(6);
        ofstream outFile(hostPath, ios::binary);
        if (!outFile) {
            out << "Error: Unable to create destination file in host system: " << hostPath << '\n';
            return;
        }

        for (int blockIndex : fileNode->dataBlocks) {
            outFile.write(dataBlocks[blockIndex].c_str(), dataBlocks[blockIndex].size());
        }

        out << "File successfully copied to host file system: " << hostPath << '\n';
    } 
    else {
        string fullSrc = (src[0] == '/') ? src : currentDir + "/" + src;
        INode* srcNode = findINode(fullSrc);
        if (!srcNode || srcNode->isDirectory) {
            out << "Error: Source file does not exist or is a directory in simulated file system: " << fullSrc << '\n';
            return;
        }

        string fullDest = (dest[0] == '/') ? dest : currentDir + "/" + dest;
        if (inodes.find(fullDest) != inodes.end()) {
            out << "Error: Destination file already exists in simulated file system: " << fullDest << '\n';
            return;
        }

        vector<int> allocatedBlocks;
        for (int blockIndex : srcNode->dataBlocks) {
            int newBlock = allocateBlock();
            if (newBlock == -1) {
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
        out << "File successfully copied within simulated file system: " << fullDest << '\n';
    }
}


void SimDisk::del(const string& path, ostream& out) {
    INode* fileNode = findINode(path);
    if (fileNode && !fileNode->isDirectory) {
        for (int blockIndex : fileNode->dataBlocks) {
            freeBlock(blockIndex);
        }
        inodes.erase(path);
        out << "File deleted: " << path << '\n';
    } else {
        out << "Error: File does not exist or is a directory.\n";
    }
}

void SimDisk::check(ostream& out) {
    out << "File system check complete.\n";
    out << "Used Blocks: " << superblock.totalBlocks - superblock.freeBlocks;
    out << ",Free Blocks: " << superblock.freeBlocks << '\n';
    
}
