#ifndef SIMDISK_H
#define SIMDISK_H

#include <string>
#include <vector>
#include <map>
#include <ostream>

using namespace std;

// Constants for disk and block size
const int DISK_SIZE = 100 * 1024 * 1024; // 100 MB
const int BLOCK_SIZE = 1024;             // 1 KB

// Superblock structure
struct Superblock {
    int totalBlocks;  // Total number of blocks
    int freeBlocks;   // Number of free blocks
};

// i-Node structure
struct INode {
    string name;          // File or directory name
    bool isDirectory;     // True if it’s a directory
    int size;             // Size of the file
    vector<int> dataBlocks; // Indices of data blocks
};

// SimDisk class
class SimDisk {
public:
    SimDisk();  // Constructor

    // Commands
    void info(ostream& out);                // Display file system information
    void cd(const string& path, ostream& out); // Change current directory
    void md(const string& path, ostream& out); // Make directory
    void dir(const string& path, ostream& out); // List directory contents
    void rd(const string& path, ostream& out);  // Remove directory
    void newfile(const string& path, ostream& out); // Create new file
    void cat(const string& path, ostream& out);     // View file content
    void copy(const string& src, const string& dest, ostream& out); // Copy files
    void del(const string& path, ostream& out);     // Delete a file
    void check(ostream& out);                       // Check file system integrity

private:
    Superblock superblock;            // Superblock structure
    map<string, INode> inodes;        // Metadata for files and directories
    vector<string> dataBlocks;        // Simulated data blocks
    vector<bool> blockBitmap;         // Bitmap to manage free blocks
    string currentDir;                // Current working directory

    // Helper functions
    int allocateBlock();              // Allocate a free block
    void freeBlock(int index);        // Free a block
    INode* findINode(const string& path); // Find an i-Node by path
    void deleteDirectoryContents(const string& dirPath); // Helper to delete directory
};

#endif // SIMDISK_H
