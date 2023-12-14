#pragma once
#include <fstream>
#include <istream>
#include <ostream>
#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <stack>
#include <cstdint>
#include <iostream>
#include <cstring>
using namespace std;

struct Descriptor
{
    uint32_t offset;
    uint32_t length;
    char name[8];
    bool isContent = true;
    Descriptor()
    {
        std::strncpy(this->name, name, sizeof(this->name));
        this->name[sizeof(this->name)] = '\0';
    }
};
struct Tree
{
    struct TreeNode
    {
        // treenode must have descriptor components
        uint32_t offset;
        uint32_t length;
        char name[8];
        bool content;
        int order;
        // each node has a parent
        TreeNode *parent;
        // node must have directories and children
        std::vector<TreeNode *> directories;
        std::vector<TreeNode *> children;

        // initializing a treenode set each node to an initial value and increment order, also do the nul terminator to name
        TreeNode(const char name[8], uint32_t offset, uint32_t length, bool content, int order)
            : offset(offset), length(length), content(content), parent(nullptr), order(order)
        {
            std::strncpy(this->name, name, sizeof(this->name));
            this->name[sizeof(this->name)] = '\0';
        }

        // add child and add directory just push back vector
        void addChild(TreeNode *child)
        {
            children.push_back(child);
        }
        void addDirectory(TreeNode *directory)
        {
            directories.push_back(directory);
        }

        // destructor destroy each ofthe vlaues in its vector
        ~TreeNode()
        {
            // Recursively delete child nodes in 'children'
            for (TreeNode *child : children)
            {
                delete child;
            }
            for (TreeNode *directory : directories)
            {
                delete directory;
            }

            // Do not delete parent as it doesn't own this node

            // Clear vectors to avoid double deletion in case they were deleted elsewhere
        }
    };

private:
    // tree has a root and keeps track of order and descriptors.
    TreeNode *root;
    int ord = 0;

    vector<Descriptor> desc;

public:
    Tree() : root(nullptr) {}

    void buildTree(vector<Descriptor> &descriptors)
    {
        bool mapMarker = false;
        int count = 0;

        root = new TreeNode("/", 0, 0, false, ord);
        ord++;
        // stack to keep track of nodes
        std::stack<TreeNode *> nodeStack;
        nodeStack.push(root);

        // iterate vector passed in
        for (int i = 0; i < descriptors.size(); i++)
        {

            const string &name = descriptors[i].name;
            // name is namespace
            if (name.find("_START") != std::string::npos)
            {
                int under = name.find("_");
                string nodeName = name.substr(0, under);
                TreeNode *currentNode = new TreeNode(nodeName.c_str(), descriptors[i].offset, descriptors[i].length, descriptors[i].isContent, ord);
                currentNode->parent = nodeStack.top();
                nodeStack.top()->addDirectory(currentNode);
                nodeStack.push(currentNode);
                ord++;
                continue;
            }
            // namespace end
            if (name.find("_END") != std::string::npos)
            {
                nodeStack.pop();
                continue;
            }

            // mapmarker
            if (name.size() == 4 && name[0] == 'E' && name[2] == 'M')
            {
                TreeNode *currentNode = new TreeNode(descriptors[i].name, descriptors[i].offset, descriptors[i].length, descriptors[i].isContent, ord);
                currentNode->parent = nodeStack.top();
                nodeStack.top()->addDirectory(currentNode);
                nodeStack.push(currentNode);
                mapMarker = true;
                ord++;
                continue;
            }
            // mapmarker less than 9
            if (mapMarker == true && count < 9)
            {
                TreeNode *currentNode = new TreeNode(descriptors[i].name, descriptors[i].offset, descriptors[i].length, descriptors[i].isContent, ord);
                nodeStack.top()->addChild(currentNode); // Link the new node to its parent
                count++;
                ord++;
                continue;
            }
            // mapmarker last node
            if (mapMarker == true && count == 9)
            {
                TreeNode *currentNode = new TreeNode(descriptors[i].name, descriptors[i].offset, descriptors[i].length, descriptors[i].isContent, ord);
                nodeStack.top()->addChild(currentNode); // Link the new node to its parent
                nodeStack.pop();
                mapMarker = false;
                count = 0;
                ord++;
                continue;
            }
            // add child if these arent true
            TreeNode *currentNode = new TreeNode(descriptors[i].name, descriptors[i].offset, descriptors[i].length, descriptors[i].isContent, ord);
            nodeStack.top()->addChild(currentNode); // Link the new node to its parent
            ord++;
        }
    }

    void printTree(TreeNode *current, int depth = 0)
    {
        if (current != nullptr)
        {

            cout << string(depth * 2, ' ') << current->name << endl;

            // print children recursively
            for (int i = 0; i < current->children.size(); i++)
            {
                printTree(current->children[i], depth + 1);
            }

            for (int i = 0; i < current->directories.size(); i++)
            {
                printTree(current->directories[i], depth + 1);
            }
        }
    }

    void printTree()
    {
        printTree(root);
    }

    bool findContent(const string &path)
    {
        // use components to find path elements
        vector<string> components;
        size_t start = 0, end;

        while ((end = path.find('/', start)) != string::npos)
        {
            if (end > start)
            {
                components.push_back(path.substr(start, end - start));
            }
            start = end + 1;
        }
        if (start < path.size())
        {
            components.push_back(path.substr(start));
        }

        TreeNode *currentNode = root;
        bool found = false;
        for (const string &component : components)
        {

            // find directory
            for (int i = 0; i < currentNode->directories.size(); i++)
            {
                if (strncmp(currentNode->directories[i]->name, component.c_str(), sizeof(currentNode->directories[i]->name)) == 0)
                {
                    currentNode = currentNode->directories[i];
                }
            }
            // find child
            for (int i = 0; i < currentNode->children.size(); i++)
            {
                if (strncmp(currentNode->children[i]->name, component.c_str(), sizeof(currentNode->children[i]->name)) == 0)
                {

                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            return false;
        }

        return true;
    }

    bool findDirectory(const string &path)
    {
        // components
        vector<string> components;
        size_t start = 0, end;

        while ((end = path.find('/', start)) != string::npos)
        {
            if (end > start)
            {
                components.push_back(path.substr(start, end - start));
            }
            start = end + 1;
        }
        if (start < path.size())
        {
            components.push_back(path.substr(start));
        }

        TreeNode *currentNode = root;
        bool found = false;
        int count = 0;
        for (const string &component : components)
        {

            // iterate directories
            for (int i = 0; i < currentNode->directories.size(); i++)
            {
                if (strncmp(currentNode->directories[i]->name, component.c_str(), sizeof(currentNode->directories[i]->name)) == 0)
                {
                    currentNode = currentNode->directories[i];
                    count++;
                }
            }
        }
        // if count is equal to the number of elemnts and the path is not empty then its true
        if (count == components.size() && path != "")
        {
            return true;
        }
        return false;
    }

    int getSize(const string &path)
    {

        vector<string> components;
        size_t start = 0, end;

        while ((end = path.find('/', start)) != string::npos)
        {
            if (end > start)
            {
                components.push_back(path.substr(start, end - start));
            }
            start = end + 1;
        }
        if (start < path.size())
        {
            components.push_back(path.substr(start));
        }

        TreeNode *currentNode = root;

        for (const string &component : components)
        {

            // find directory
            for (int i = 0; i < currentNode->directories.size(); i++)
            {
                if (strncmp(currentNode->directories[i]->name, component.c_str(), sizeof(currentNode->directories[i]->name)) == 0)
                {
                    currentNode = currentNode->directories[i];
                }
            }
            // find child and return length
            for (int i = 0; i < currentNode->children.size(); i++)
            {
                if (strncmp(currentNode->children[i]->name, component.c_str(), sizeof(currentNode->children[i]->name)) == 0)
                {

                    return currentNode->children[i]->length;
                }
            }
        }

        return -1;
    }
    int returnDescriptor(const string &path)
    {
        // return file offset (location)
        vector<string> components;
        size_t start = 0, end;

        while ((end = path.find('/', start)) != string::npos)
        {
            if (end > start)
            {
                components.push_back(path.substr(start, end - start));
            }
            start = end + 1;
        }
        if (start < path.size())
        {
            components.push_back(path.substr(start));
        }

        TreeNode *currentNode = root;
        for (const string &component : components)
        {

            // check directories
            for (int i = 0; i < currentNode->directories.size(); i++)
            {
                if (strncmp(currentNode->directories[i]->name, component.c_str(), sizeof(currentNode->directories[i]->name)) == 0)
                {
                    currentNode = currentNode->directories[i];
                }
            }
            // find child and return offset
            for (int i = 0; i < currentNode->children.size(); i++)
            {
                if (strncmp(currentNode->children[i]->name, component.c_str(), sizeof(currentNode->children[i]->name)) == 0)
                {

                    return currentNode->children[i]->offset;
                }
            }
        }
        return -1;
    }

    vector<string> returnDirectory(const string &path)
    {

        vector<string> components;
        size_t start = 0, end;

        while ((end = path.find('/', start)) != string::npos)
        {
            if (end > start)
            {
                components.push_back(path.substr(start, end - start));
            }
            start = end + 1;
        }

        if (start < path.size() && path[start] != '/')
        {
            components.push_back(path.substr(start));
        }

        vector<string> returnDirectories;
        TreeNode *currentNode = root;
        vector<TreeNode *> allNodes;
        // find directories
        for (const string &component : components)
        {

            for (int i = 0; i < currentNode->directories.size(); i++)
            {
                if (strncmp(currentNode->directories[i]->name, component.c_str(), sizeof(currentNode->directories[i]->name)) == 0)
                {
                    currentNode = currentNode->directories[i];
                }
            }
        }
        // insert all nodes
        allNodes.insert(allNodes.begin(), currentNode->directories.begin(), currentNode->directories.end());
        allNodes.insert(allNodes.begin(), currentNode->children.begin(), currentNode->children.end());

        // sort all the nodes
        std::sort(allNodes.begin(), allNodes.end(),
                  [](const TreeNode *a, const TreeNode *b)
                  {
                      return a->order < b->order;
                  });
        // pushback all thendoes in the new sorted vector and then return
        for (const TreeNode *node : allNodes)
        {
            returnDirectories.push_back(node->name);
        }
        return returnDirectories;
    }

    void insertDirectory(const string &path)
    {
        string newDirectoryName;
        vector<string> components;
        size_t start = 0, end;
        components.push_back("/");
        while ((end = path.find('/', start)) != string::npos)
        {
            if (end > start)
            {
                components.push_back(path.substr(start, end - start));
            }
            start = end + 1;
        }

        if (start < path.size() && path[start] != '/')
        {
            components.push_back(path.substr(start));
        }

        newDirectoryName = components.back();
        TreeNode *currentNode = root;
        // find directory that matches
        for (const string &component : components)
        {

            for (int i = 0; i < currentNode->directories.size(); i++)
            {

                if ((strncmp(currentNode->directories[i]->name, component.c_str(), sizeof(currentNode->directories[i]->name)) == 0))
                {
                    currentNode = currentNode->directories[i];
                }
            }
        }

        // pair the directories by using add function
        const char *newDirectoryNameCStr = newDirectoryName.c_str();
        TreeNode *newDirectory = new TreeNode(newDirectoryNameCStr, 0, 0, false, ord);
        currentNode->addDirectory(newDirectory);

        ord++;
    }

    void insertFile(const string &path)
    {
        string newChildName;
        vector<string> components;
        size_t start = 0, end;
        components.push_back("/");
        while ((end = path.find('/', start)) != string::npos)
        {
            if (end > start)
            {
                components.push_back(path.substr(start, end - start));
            }
            start = end + 1;
        }

        if (start < path.size() && path[start] != '/')
        {
            components.push_back(path.substr(start));
        }

        newChildName = components.back();

        TreeNode *currentNode = root;
        for (const string &component : components)
        {

            for (int i = 0; i < currentNode->directories.size(); i++)
            {
                if (strncmp(currentNode->directories[i]->name, component.c_str(), sizeof(currentNode->directories[i]->name)) == 0)
                {
                    currentNode = currentNode->directories[i];
                }
            }
        }

        // find directory and then insert child
        const string &name = currentNode->name;
        const char *newChildNameCStr = newChildName.c_str();
        TreeNode *newChild = new TreeNode(newChildNameCStr, 0, 0, true, ord);
        currentNode->addChild(newChild);

        ord++;
    }

    void updateFile(const string &path, int length, int offset)
    {

        vector<string> components;
        size_t start = 0, end;

        while ((end = path.find('/', start)) != string::npos)
        {
            if (end > start)
            {
                components.push_back(path.substr(start, end - start));
            }
            start = end + 1;
        }
        if (start < path.size())
        {
            components.push_back(path.substr(start));
        }

        TreeNode *currentNode = root;

        for (const string &component : components)
        {

            // cehck direcotires
            for (int i = 0; i < currentNode->directories.size(); i++)
            {
                if (strncmp(currentNode->directories[i]->name, component.c_str(), sizeof(currentNode->directories[i]->name)) == 0)
                {
                    currentNode = currentNode->directories[i];
                }
            }
            // find child and reqrite the offset and length
            for (int i = 0; i < currentNode->children.size(); i++)
            {
                if (strncmp(currentNode->children[i]->name, component.c_str(), sizeof(currentNode->children[i]->name)) == 0)
                {
                    currentNode->children[i]->length = length;
                    currentNode->children[i]->offset = offset;
                }
            }
        }

        return;
    }
    ~Tree()
    {
        // triggers recrusive deletion of all the nodes
        delete root;
    }
};
struct Wad
{
private:
    // magic num desc and desc offset
    char magic[4];
    int numDescriptor;
    int descriptorOffset;
    // tree instance and file instance
    Tree tree;
    fstream file;
    Wad(const string &path);
    string wadPath;

public:
    static Wad *loadWad(const std::string &path);
    std::string getMagic();
    bool isContent(const std::string &path);
    bool isDirectory(const std::string &path);
    int getSize(const std::string &path);
    int getContents(const std::string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const std::string &path, std::vector<std::string> *directory);
    void createDirectory(const std::string &path);
    void createFile(const std::string &path);
    int writeToFile(const std::string &path, const char *buffer, int length, int offset = 0);
    ~Wad();
};
