#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

const int DISK_SIZE = 100 * 1024 * 1024; // 100MB
const int BLOCK_SIZE = 1024;            // 1KB
const int NUM_BLOCKS = DISK_SIZE / BLOCK_SIZE;

// Superblock structure
struct Superblock {
    int totalBlocks;
    int freeBlocks;
    vector<bool> blockBitmap; // Bitmap for free block management
};

// i-Node structure
struct INode {
    string name;
    bool isDirectory;
    int size;
    int protectionCode;
    vector<int> dataBlocks;  // Points to data blocks
};

// Simulated file system
class SimDisk {
private:
    Superblock superblock;
    map<string, INode> inodes; // Metadata for files and directories
    string currentDir;         // Current working directory
    vector<string> dataBlocks; // Simulated data blocks for file content

    // Helper to split paths
    vector<string> splitPath(const string& path) {
        stringstream ss(path);
        string part;
        vector<string> components;
        while (getline(ss, part, '/')) {
            if (!part.empty()) components.push_back(part);
        }
        return components;
    }

    // Helper to find an i-node
    INode* findINode(const string& path) {
        auto it = inodes.find(path);
        return it != inodes.end() ? &it->second : nullptr;
    }

    // Helper to create a new block
    int allocateBlock() {
        for (int i = 0; i < superblock.blockBitmap.size(); ++i) {
            if (superblock.blockBitmap[i]) {
                superblock.blockBitmap[i] = false;
                superblock.freeBlocks--;
                return i;
            }
        }
        return -1; // No free block available
    }

    // Helper to free a block
    void freeBlock(int blockIndex) {
        if (blockIndex >= 0 && blockIndex < superblock.blockBitmap.size()) {
            superblock.blockBitmap[blockIndex] = true;
            superblock.freeBlocks++;
        }
    }

    // Recursively delete all files and subdirectories in a directory
    void deleteDirectoryContents(const string& dirPath) {
        for (auto it = inodes.begin(); it != inodes.end();) {
            if (it->first.find(dirPath + "/") == 0) {
                if (it->second.isDirectory) {
                    // Recursively delete subdirectory contents
                    deleteDirectoryContents(it->first);
                } 
                else {
                    // Delete file
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

public:
    SimDisk() {
        currentDir = "/";
        superblock.totalBlocks = NUM_BLOCKS;
        superblock.freeBlocks = NUM_BLOCKS;
        superblock.blockBitmap.resize(NUM_BLOCKS, true); // All blocks free initially
        dataBlocks.resize(NUM_BLOCKS, ""); // Initialize data blocks as empty

        // Create root directory
        inodes["/"] = {"root", true, 0, 777, {}};
    }

    // Command: info
    void info() {
        cout << "Disk Size: " << DISK_SIZE / (1024 * 1024) << " MB\n";
        cout << "Block Size: " << BLOCK_SIZE << " Bytes\n";
        cout << "Free Blocks: " << superblock.freeBlocks << '\n';
        cout << "Used Blocks: " << superblock.totalBlocks - superblock.freeBlocks << '\n';
        cout << "Root Directory: " << "/" << '\n';
    }

    // Command: cd
    void cd(const string& path) {
        string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
        if (inodes.find(fullPath) != inodes.end() && inodes[fullPath].isDirectory) {
            currentDir = fullPath;
        } 
        else {
            cerr << "Error: Directory does not exist.\n";
        }
    }

    // Command: md
    void md(const string& path) {
        string fullPath = (path[0] == '/') ? path : currentDir + path;
        if (inodes.find(fullPath) != inodes.end()) {
            cerr << "Error: Directory already exists.\n";
            return;
        }
        inodes[fullPath] = {path, true, 0, 777, {}};
        cout << "Directory created: " << fullPath << '\n';
    }

    // Command: dir
    void dir(const string& path) {
        string fullPath = (path.empty()) ? currentDir : ((path[0] == '/') ? path : currentDir + "/" + path);
        if (inodes.find(fullPath) != inodes.end() && inodes[fullPath].isDirectory) {
            cout << "Contents of " << fullPath << ":\n";
            for (map<string, INode>::iterator it = inodes.begin(); it != inodes.end(); ++it) {
                if (it->first.find(fullPath) == 0 && it->first != fullPath) {
                    cout << (it->second.isDirectory ? "[DIR] " : "[FILE] ") << it->first << '\n';
                }
            }
        } 
        else {
            cerr << "Error: Directory does not exist.\n";
        }
    }

    // Command: rd
    void rd(const string& path) {
        string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
        auto it = inodes.find(fullPath);
        if (it != inodes.end() && it->second.isDirectory) {
            cout << "Are you sure you want to delete this directory and its contents? (yes/no): ";
            string response;
            cin >> response;

            // Flush input buffer to prevent leftover newline
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (response == "yes") {
                deleteDirectoryContents(fullPath); // Delete all contents
                inodes.erase(it);                 // Delete the directory itself
                cout << "Directory deleted: " << fullPath << "\n";
            } 
            else {
                cout << "Operation canceled.\n";
            }
        } 
        else {
            cerr << "Error: Directory does not exist or is not a directory.\n";
        }
    }

    // Command: newfile
    void newfile(const string& path) {
        string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
        if (inodes.find(fullPath) != inodes.end()) {
            cerr << "Error: File already exists.\n";
            return;
        }
        int blockIndex = allocateBlock();
        if (blockIndex == -1) {
            cerr << "Error: No free blocks available.\n";
            return;
        }
        inodes[fullPath] = {fullPath, false, BLOCK_SIZE, 777, {blockIndex}};
        cout << "File created: " << fullPath << '\n';
    }

    // Command: cat
    void cat(const string& path) {
        string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
        INode* file = findINode(fullPath);
        if (file && !file->isDirectory) {
            cout << "File content:\n";
            for (int blockIndex : file->dataBlocks) {
                cout << dataBlocks[blockIndex];
            }
            cout << '\n';
        } 
        else {
            cerr << "Error: File does not exist or is a directory.\n";
        }
    }

    // Command: copy
    void copy(const string& src, const string& dest) {
        if (src.rfind("<host>", 0) == 0) {
            string hostPath = src.substr(6); // Remove "<host>" prefix
            ifstream inFile(hostPath, ios::binary);
            if (!inFile.is_open()) {
                cerr << "Error: Source file not found or cannot be opened in host system: " << hostPath << '\n';
                return;
            }

            string fullDest = (dest[0] == '/') ? dest : currentDir + dest;
            if (inodes.find(fullDest) != inodes.end()) {
                cerr << "Error: Destination file already exists in simulated file system: " << fullDest << '\n';
                return;
            }

            vector<int> allocatedBlocks;
            char buffer[BLOCK_SIZE];
            while (inFile.read(buffer, BLOCK_SIZE) || inFile.gcount() > 0) {
                int blockIndex = allocateBlock();
                if (blockIndex == -1) {
                    cerr << "Error: Not enough space in simulated file system.\n";
                    for (int idx : allocatedBlocks) {
                        freeBlock(idx);
                    }
                    return;
                }
                allocatedBlocks.push_back(blockIndex);
                dataBlocks[blockIndex] = string(buffer, inFile.gcount());
            }

            inodes[fullDest] = {fullDest, false, (int)allocatedBlocks.size() * BLOCK_SIZE, 777, allocatedBlocks};
            cout << "File successfully copied to simulated file system: " << fullDest << '\n';
        } 
        else if (dest.rfind("<host>", 0) == 0) {
            string fullSrc = (src[0] == '/') ? src : currentDir + src;
            INode* fileNode = findINode(fullSrc);
            if (!fileNode || fileNode->isDirectory) {
                cerr << "Error: Source file does not exist or is a directory in simulated file system: " << fullSrc << '\n';
                return;
            }

            string hostPath = dest.substr(6);
            ofstream outFile(hostPath, ios::binary);
            if (!outFile) {
                cerr << "Error: Unable to create destination file in host system: " << hostPath << '\n';
                return;
            }

            for (int blockIndex : fileNode->dataBlocks) {
                outFile.write(dataBlocks[blockIndex].c_str(), dataBlocks[blockIndex].size());
            }

            cout << "File successfully copied to host file system: " << hostPath << '\n';
        } 
        else {
            string fullSrc = (src[0] == '/') ? src : currentDir + src;
            INode* srcNode = findINode(fullSrc);
            if (!srcNode || srcNode->isDirectory) {
                cerr << "Error: Source file does not exist or is a directory in simulated file system: " << fullSrc << '\n';
                return;
            }

            string fullDest = (dest[0] == '/') ? dest : currentDir + dest;
            if (inodes.find(fullDest) != inodes.end()) {
                cerr << "Error: Destination file already exists in simulated file system: " << fullDest << '\n';
                return;
            }

            vector<int> allocatedBlocks;
            for (int blockIndex : srcNode->dataBlocks) {
                int newBlock = allocateBlock();
                if (newBlock == -1) {
                    cerr << "Error: Not enough space in simulated file system.\n";
                    for (int idx : allocatedBlocks) {
                        freeBlock(idx);
                    }
                    return;
                }
                allocatedBlocks.push_back(newBlock);
                dataBlocks[newBlock] = dataBlocks[blockIndex];
            }

            inodes[fullDest] = {fullDest, false, srcNode->size, 777, allocatedBlocks};
            cout << "File successfully copied within simulated file system: " << fullDest << '\n';
        }
    }

    // Command: del
    void del(const string& path) {
        string fullPath = (path[0] == '/') ? path : currentDir + "/" + path;
        auto it = inodes.find(fullPath);
        if (it != inodes.end() && !it->second.isDirectory) {
            for (int blockIndex : it->second.dataBlocks) {
                freeBlock(blockIndex);
            }
            inodes.erase(it);
            cout << "File deleted: " << fullPath << '\n';
        } 
        else {
            cerr << "Error: File does not exist or is a directory.\n";
        }
    }

    // Command: check
    void check() {
        int usedBlocks = 0;
        for (auto& pair : inodes) {
            for (int blockIndex : pair.second.dataBlocks) {
                if (blockIndex >= 0 && blockIndex < NUM_BLOCKS && !superblock.blockBitmap[blockIndex]) {
                    usedBlocks++;
                } 
                else {
                    cerr << "Error: Corrupted block in file: " << pair.first << '\n';
                }
            }
        }
        cout << "File system check complete.\n";
        cout << "Used blocks: " << usedBlocks << ", Free blocks: " << superblock.freeBlocks << '\n';
    }

    // Command: execute
    void execute() {
        string command;
        while (true) {
            cout << "simdisk> ";
            getline(cin, command);

            stringstream ss(command);
            string cmd, arg;
            ss >> cmd;

            if (cmd == "info") {
                info();
            } 
            else if (cmd == "cd") {
                ss >> arg;
                cd(arg);
            } 
            else if (cmd == "dir") {
                ss >> arg;
                dir(arg);
            } 
            else if (cmd == "md") {
                ss >> arg;
                md(arg);
            } 
            else if (cmd == "rd") {
                ss >> arg;
                rd(arg);
            } 
            else if (cmd == "newfile") {
                ss >> arg;
                if (arg.empty()) {
                    cerr << "Error: Missing file name.\n";
                } else {
                    newfile(arg);
                }
            } 
            else if (cmd == "cat") {
                ss >> arg;
                if (arg.empty()) {
                    cerr << "Error: Missing file name.\n";
                } else {
                    cat(arg);
                }
            } 
            else if (cmd == "copy") {
                string src, dest;
                ss >> src >> dest;
                if (src.empty() || dest.empty()) {
                    cerr << "Error: Missing source or destination.\n";
                } else {
                    copy(src, dest);
                }
            } 
            else if (cmd == "del") {
                ss >> arg;
                if (arg.empty()) {
                    cerr << "Error: Missing file name.\n";
                } else {
                    del(arg);
                }
            } 
            else if (cmd == "check") {
                check();
            } 
            else if (cmd == "exit") {
                break;
            } 
            else {
                cerr << "Error: Unknown command.\n";
            }
        }
    }
};

int main() {
    SimDisk disk;
    disk.execute();
    return 0;
}