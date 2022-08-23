#pragma once
#include <functional>

enum FileSystemChange
{
    FileSystemChangeNone,
    FileSystemChangeAdd,
    FileSystemChangeDelete,
    FileSystemChangeModify,
    FileSystemChangeRename
};
struct FileSystemEvent
{
    FileSystemChange change;
    fs::path path;
    fs::path oldPath;   //used for rename
};
using FileSystemObserver = std::function<void(const FileSystemEvent&)>;

bool ObserveDirectoryThread(const fs::path& directory, const FileSystemObserver& callback, bool watchSubDir = true);