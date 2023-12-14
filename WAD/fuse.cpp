#define FUSE_USE_VERSION 26

#include "../libWad/Wad.h"
#include "../libWad/Wad.cpp"
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

using namespace std;

static int write_callback(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    return ((Wad *)fuse_get_context()->private_data)->writeToFile(path, buf, size, offset);
}

static int mkdir_callback(const char *path, mode_t mode)
{
    ((Wad *)fuse_get_context()->private_data)->createDirectory(path);
    return 0;
}

static int mknod_callback(const char *path, mode_t mode, dev_t rdev)
{
    // S_ISREG means if it is a regular file then proceed
    if (S_ISREG(mode))
    {
        ((Wad *)fuse_get_context()->private_data)->createFile(path);
        return 0;
    }
    else
    {
        return -EACCES;
    }
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *fi)
{
    (void)offset;
    (void)fi;

    vector<string> directoryContents;
    ((Wad *)fuse_get_context()->private_data)->getDirectory(path, &directoryContents);

    for (int i = 0; i < directoryContents.size(); i++)
    {
        filler(buf, directoryContents[i].c_str(), NULL, 0);
    }
    return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
    // if get contents does not equal zero then return the value else throw error
    if (((Wad *)fuse_get_context()->private_data)->getContents(path, buf, size, offset) > 0)
    {
        return ((Wad *)fuse_get_context()->private_data)->getContents(path, buf, size, offset);
    }
    else
    {
        return -ENOENT;
    }
}

static int getattr_callback(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));

    if (((Wad *)fuse_get_context()->private_data)->isDirectory(path))
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    if (((Wad *)fuse_get_context()->private_data)->isContent(path))
    {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = ((Wad *)fuse_get_context()->private_data)->getSize(path);
        return 0;
    }
    return -ENOENT;
}

static struct fuse_operations fuse_example_operations = {
    .getattr = getattr_callback,
    .mknod = mknod_callback,
    .mkdir = mkdir_callback,
    .read = read_callback,
    .write = write_callback,
    .readdir = readdir_callback,

};

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "not enough arguments" << endl;
        exit(EXIT_SUCCESS);
    }

    std::string wadPath = argv[argc - 2];

    if (wadPath.at(0) != '/')
    {
        wadPath = std::string(get_current_dir_name()) + "/" + wadPath;
    }

    Wad *myWad = Wad::loadWad(wadPath);

    // argv at 2 is equal to mount directory
    argv[argc - 2] = argv[argc - 1];
    argc--;
    return fuse_main(argc, argv, &fuse_example_operations, myWad);
}
