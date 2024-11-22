#ifndef SIMDISK_H
#define SIMDISK_H

#include <string>
#include <vector>
#include <map>
#include <ostream>
#include <windows.h> // For CRITICAL_SECTION (synchronization)

using namespace std;

// Constants for disk size and block size
const int DISK_SIZE = 100 * 1024 * 1024; // 100 MB
const int BLOCK_SIZE = 1024;             // 1 KB

// Roles and permissions
enum Role { Admin, Editor, Viewer };

struct User {
    string username;
    string password;
    Role role;
};

// Superblock structure
struct Superblock {
    int totalBlocks;
    int freeBlocks;
};

// i-Node structure
struct INode {
    string name;
    bool isDirectory;
    int size;
    vector<int> dataBlocks;
};

// SimDisk class
class SimDisk {
public:
    SimDisk();
    ~SimDisk(); // Destructor to clean up resources

    // Commands
    void info(User* user, ostream& out);
    void cd(const string& path, User* user, ostream& out);
    void md(const string& path, User* user, ostream& out);
    void dir(const string& path, User* user, ostream& out);
    void rd(const string& path, User* user, ostream& out);
    void newfile(const string& path, User* user, ostream& out);
    void cat(const string& path, User* user, ostream& out);
    void copy(const string& src, const string& dest, User* user, ostream& out);
    void del(const string& path, User* user, ostream& out);
    void check(User* user, ostream& out);

private:
    Superblock superblock;
    map<string, INode> inodes;
    vector<string> dataBlocks;
    vector<bool> blockBitmap;
    string currentDir;

    // Windows synchronization primitive
    CRITICAL_SECTION rwLock;

    // Helper functions
    int allocateBlock();
    void freeBlock(int index);
    INode* findINode(const string& path);
    void deleteDirectoryContents(const string& dirPath);
};

// User login function
User* login(const string& username, const string& password);

#endif // SIMDISK_H
