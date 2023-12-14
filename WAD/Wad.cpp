#include "Wad.h"
#include <unordered_map>

// wad private constructor
Wad::Wad(const string &path)
    : file(path, ios::binary | ios::in | ios::out)
{
    if (!file.is_open())
    {
        cerr << "could not open file " << path << endl;
        return;
    }
    wadPath = path;
    // read all wad file specific things
    file.read(magic, sizeof(magic));
    file.read(reinterpret_cast<char *>(&numDescriptor), sizeof(numDescriptor));
    file.read(reinterpret_cast<char *>(&descriptorOffset), sizeof(descriptorOffset));

    file.seekg(descriptorOffset, std::ios::beg);
    vector<Descriptor> descriptors;
    // read in all the descriptors
    for (int i = 0; i < numDescriptor; i++)
    {
        Descriptor descriptor;

        file.read(reinterpret_cast<char *>(&descriptor.offset), sizeof(descriptor.offset));
        file.read(reinterpret_cast<char *>(&descriptor.length), sizeof(descriptor.length));
        file.read(descriptor.name, sizeof(descriptor.name));
        descriptor.isContent = true;
        if (descriptor.offset == 0 && descriptor.length == 0)
        {
            descriptor.isContent = false;
        }
        descriptors.push_back(descriptor);
    }

    // call build tree to build the tree
    tree.buildTree(descriptors);
    file.close();
}

Wad *Wad::loadWad(const std::string &path)
{
    // dynamically allocate wad and call constructor
    Wad *wad = new Wad(path);
    return wad;
}

string Wad::getMagic()
{
    magic[4] = '\0';
    return string(magic);
}

bool Wad::isContent(const string &path)
{
    return tree.findContent(path);
}

bool Wad::isDirectory(const string &path)
{
    return tree.findDirectory(path);
}

int Wad::getSize(const string &path)
{
    return tree.getSize(path);
}

int Wad::getContents(const string &path, char *buffer, int length, int offset)
{
    if (!isContent(path))
    {
        return -1;
    }
    else
    {

        file.open(wadPath, std::ios::binary | std::ios::in);
        // find file offset and then seek the contents from the offset given
        int fileOffset = tree.returnDescriptor(path);
        file.seekg(fileOffset + offset, ios::beg);
        // get remaining bytes and then read that many bytes, get file count close file and then return
        int remainingBytes = tree.getSize(path) - offset;
        int bytesToRead = min(length, remainingBytes);
        file.read(buffer, bytesToRead);
        int count = file.gcount();
        file.close();
        return count;
    }
}

int Wad::getDirectory(const string &path, vector<string> *directory)
{
    if (!isDirectory(path))
    {
        return -1;
    }
    else
    {
        // use return directory to get vector size and then return
        *directory = tree.returnDirectory(path);
        return directory->size();
    }
}

void Wad::createDirectory(const string &path)
{
    string parentParent;
    string parentName;
    string newDirectoryName;
    vector<string> parents;
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
    // get parent name and directory name from the components of the elements of the path
    if (start < path.size() && path[start] != '/')
    {
        components.push_back(path.substr(start));
    }

    if (components.size() == 2)
    {
        parentName = components[0];
        newDirectoryName = components[1];
    }
    if (components.size() > 2)
    {
        parentName = components[components.size() - 2];
        newDirectoryName = components[components.size() - 1];
    }
    string temp;
    components.pop_back();
    for (int i = 0; i < components.size(); i++)
    {
        if (i > 0)
        {
            temp = temp + components[i] + "/";
        }
        else
        {
            temp = temp + components[i];
        }
    }
    // check if parent path is a directory
    if (!isDirectory(temp))
    {
        cout << "is not a directory" << endl;
        return;
    }
    // path of parent exists
    if (isDirectory(temp))
    {

        if (newDirectoryName.size() > 2)
        {
            cout << "name exceeded 2 " << newDirectoryName << endl;
            return;
        }
        // add to data structure
        tree.insertDirectory(path);

        // create descriptors
        Descriptor startDescriptor;
        startDescriptor.offset = 0;
        startDescriptor.length = 0;
        startDescriptor.isContent = false;
        std::strcpy(startDescriptor.name, (newDirectoryName + "_START").c_str());
        Descriptor endDescriptor;
        endDescriptor.offset = 0;
        endDescriptor.length = 0;
        endDescriptor.isContent = false;
        std::strcpy(endDescriptor.name, (newDirectoryName + "_END").c_str());
        // open file and read magic to get the descriptor and update wad file
        file.open(wadPath, std::ios::binary | std::ios::in | std::ios::out);
        file.read(magic, sizeof(magic));
        int temp = numDescriptor;
        numDescriptor += 2;
        file.clear();
        file.write(reinterpret_cast<char *>(&numDescriptor), sizeof(numDescriptor));
        vector<Descriptor> tempVec;
        map<string, int> map;
        file.flush();

        // read in the descriptors so i can insert directory
        file.seekg(descriptorOffset, ios::beg);
        string nam;
        int found = 0;
        for (int i = 0; i < temp; i++)
        {
            Descriptor descriptor;

            file.read(reinterpret_cast<char *>(&descriptor.offset), sizeof(descriptor.offset));
            file.read(reinterpret_cast<char *>(&descriptor.length), sizeof(descriptor.length));
            file.read(descriptor.name, sizeof(descriptor.name));
            if (map.find(descriptor.name) != map.end())
            {
                nam = descriptor.name;
            }
            map[descriptor.name] = 0;
            tempVec.push_back(descriptor);
        }

        // if parent name == name_end then insert at that positon (i)
        for (int i = 0; i < tempVec.size(); i++)
        {
            const string &name = tempVec[i].name;

            if (name == parentName + "_END")
            {
                tempVec.insert(tempVec.begin() + i, startDescriptor);
                tempVec.insert(tempVec.begin() + i + 1, endDescriptor);
                break;
            }
            if (parentName == "/")
            {
                tempVec.push_back(startDescriptor);
                tempVec.push_back(endDescriptor);
                break;
            }
        }
        file.clear();
        file.seekp(descriptorOffset, ios::beg);
        // rewrite everything to the file
        for (int i = 0; i < numDescriptor; i++)
        {
            file.write(reinterpret_cast<char *>(&tempVec[i].offset), 4);
            file.write(reinterpret_cast<char *>(&tempVec[i].length), 4);
            file.write(tempVec[i].name, 8);
        }

        file.close();
        return;
    }
}

// same concept as before except only one descriptor
void Wad::createFile(const string &path)
{
    string parentparent;
    string parentName;
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

    if (components.size() == 2)
    {
        parentName = components[0];
        newChildName = components[1];
    }
    if (components.size() > 2)
    {
        parentName = components[components.size() - 2];
        newChildName = components[components.size() - 1];
    }
    parentparent = "/";
    if (components.size() >= 3)
    {

        parentparent = components[components.size() - 3];
        parentName = components[components.size() - 2];
        newChildName = components[components.size() - 1];
    }
    string temp;
    components.pop_back();
    for (int i = 0; i < components.size(); i++)
    {
        if (i > 0)
        {
            temp = temp + components[i] + "/";
        }
        else
        {
            temp = temp + components[i];
        }
    }
    if (!isDirectory(temp))
    {
        cout << "parent directory doesnt exist" << endl;
        return;
    }

    // path of parent exists
    if (isDirectory(temp))
    {
        if (newChildName.find('_') != string::npos)
        {
            cout << "has _start or _end in name" << endl;
            return;
        }
        if (newChildName.size() == 4 && newChildName[0] == 'E' && newChildName[2] == 'M')
        {
            cout << "E#M# child" << endl;
            return;
        }
        if (parentName.size() == 4 && parentName[0] == 'E' && parentName[2] == 'M')
        {
            cout << "parent is E#M#" << endl;
            return;
        }
        if (newChildName.size() > 8)
        {
            cout << "too big pause" << endl;
            return;
        }
        tree.insertFile(path);

        Descriptor fileDescriptor;
        fileDescriptor.offset = 0;
        fileDescriptor.length = 0;
        fileDescriptor.isContent = true;
        std::strcpy(fileDescriptor.name, (newChildName).c_str());

        file.open(wadPath, std::ios::binary | std::ios::in | std::ios::out);
        file.read(magic, sizeof(magic));
        int temp = numDescriptor;
        numDescriptor++;
        file.clear();
        file.write(reinterpret_cast<char *>(&numDescriptor), sizeof(numDescriptor));
        vector<Descriptor> tempVec;
        unordered_map<string, int> map;
        file.flush();
        file.seekg(descriptorOffset, ios::beg);
        string nam;
        int found = 0;
        for (int i = 0; i < temp; i++)
        {
            Descriptor descriptor;

            file.read(reinterpret_cast<char *>(&descriptor.offset), sizeof(descriptor.offset));
            file.read(reinterpret_cast<char *>(&descriptor.length), sizeof(descriptor.length));
            file.read(descriptor.name, sizeof(descriptor.name));
            const string &name = descriptor.name;
            map[name]++;
            tempVec.push_back(descriptor);
        }

        for (int i = 0; i < tempVec.size(); i++)
        {
            const string &name = tempVec[i].name;

            if (name == parentName + "_END")
            {

                tempVec.insert(tempVec.begin() + i, fileDescriptor);
                break;
            }
            if (parentName == "/")
            {
                tempVec.push_back(fileDescriptor);
                break;
            }
        }
        file.clear();
        file.seekp(descriptorOffset, ios::beg);

        for (int i = 0; i < numDescriptor; i++)
        {
            file.write(reinterpret_cast<char *>(&tempVec[i].offset), 4);
            file.write(reinterpret_cast<char *>(&tempVec[i].length), 4);
            file.write(tempVec[i].name, 8);
        }
        file.close();
        return;
    }
}

int Wad::writeToFile(const string &path, const char *buffer, int length, int offset)
{
    string parentName;
    string fileName;
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

    if (components.size() == 2)
    {
        parentName = components[0];
        fileName = components[1];
    }
    if (components.size() > 2)
    {
        parentName = components[components.size() - 2];
        fileName = components[components.size() - 1];
    }

    if (!isContent(path))
    {
        cout << "is not content" << endl;
        return -1;
    }
    if (tree.returnDescriptor(path) != 0)
    {
        cout << "retunr desc not zero" << endl;
        return 0;
    }

    // if the file is content and the size is zero whichmenas its empty, write to it
    if (isContent(path) && tree.getSize(path) == 0)
    {
        file.open(wadPath, std::ios::binary | std::ios::in | std::ios::out);

        file.seekg(descriptorOffset, std::ios::beg);

        vector<Descriptor> tempVec;
        for (int i = 0; i < numDescriptor; i++)
        {
            Descriptor descriptor;

            file.read(reinterpret_cast<char *>(&descriptor.offset), sizeof(descriptor.offset));
            file.read(reinterpret_cast<char *>(&descriptor.length), sizeof(descriptor.length));
            file.read(descriptor.name, sizeof(descriptor.name));
            tempVec.push_back(descriptor);
        }

        int remainingBytes = length - offset;

        // get remaining bytes from offset and write the content
        for (int i = 0; i < tempVec.size(); i++)
        {

            const string &name = tempVec[i].name;
            if (name == fileName)
            {
                tempVec[i].offset = descriptorOffset;
                tempVec[i].length = remainingBytes;
                break;
            }
        }
        file.clear();
        file.seekp(descriptorOffset, ios::beg);

        int init = file.tellp();
        // write content buffer length size
        file.write(buffer, length);
        int fin = file.tellp();

        int bytesCopied = fin - init;

        for (int i = 0; i < numDescriptor; i++)
        {
            file.write(reinterpret_cast<char *>(&tempVec[i].offset), 4);
            file.write(reinterpret_cast<char *>(&tempVec[i].length), 4);
            file.write(tempVec[i].name, 8);
        }
        // update descriptor offset, update file in tree and return bytes copied
        file.seekp(sizeof(magic) + sizeof(numDescriptor), std::ios::beg);
        descriptorOffset += length;
        file.write(reinterpret_cast<char *>(&descriptorOffset), sizeof(descriptorOffset));
        tree.updateFile(path, remainingBytes, init);
        file.close();
        return bytesCopied;
    }
    return 0;
}
// destructor
Wad::~Wad()
{
    file.close();
}
