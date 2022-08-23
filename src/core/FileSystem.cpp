#include "pch.h"
#include "FileSystem.h"

struct ObserveParams
{
    fs::path directory;
    FileSystemObserver callback;
    bool watchSubDirs;
};

static DWORD ObserveDirectory(void* inParams)
{
    ObserveParams* params = static_cast<ObserveParams*>(inParams);
    uint8 buffer[1024] = {};
    //FILE_FLAG_BACKUP_SEMANTICS must set this flag to obtain a handle to a directory.
    //When subsequent I/O operations are completed on this handle, the event specified in the OVERLAPPED structure will be set to the signaled state
    HANDLE directoryHandle = CreateFileW(
        params->directory.c_str(),
        GENERIC_READ | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);
    if(directoryHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "ObserveDirectory failed.\n";
        return 1;
    }
    OVERLAPPED overlapped;
    overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    ResetEvent(overlapped.hEvent);
    DWORD dwNotifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE |
    FILE_NOTIFY_CHANGE_FILE_NAME;
    DWORD byteReturned;
    fs::path lastWritePath = "";
    fs::file_time_type lastChangedTimeStamp;

    while(true) {
        //the event specified in the OVERLAPPED structure has been set to the signaled state 
        DWORD result = ReadDirectoryChangesW(directoryHandle, buffer, sizeof(buffer),
                                             params->watchSubDirs, dwNotifyFilter, &byteReturned, &overlapped, nullptr);
        if(!result) {
            std::cerr << "ObserveDirectory Read directory changes failed\n";
            break;
        }
        //Wait until subsequent I/O operations are completed on this handle, then event specified in the OVERLAPPED structure will be set to the signaled state
        WaitForSingleObject(overlapped.hEvent, INFINITE);

        DWORD dw;
        if(!GetOverlappedResult(directoryHandle, &overlapped, &dw, FALSE) || dw == 0) {
            std::cerr << "ObserveDirectory Get overlapped result failed.\n";
            break;
        }

        FILE_NOTIFY_INFORMATION* fileNotify;
        DWORD offset = 0;
        fs::path oldPath;

        do {
            // ReadDirectoryChangesW
            fileNotify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[offset]);
            FileSystemChange change = FileSystemChangeNone;
            switch(fileNotify -> Action) {
            case FILE_ACTION_ADDED: { change = FileSystemChangeAdd; }
                break;
            case FILE_ACTION_REMOVED: { change = FileSystemChangeDelete; }
                break;
            case FILE_ACTION_MODIFIED: { change = FileSystemChangeModify; }
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                {
                    uint32 filenameLength = fileNotify->FileNameLength / sizeof(WCHAR);
                    oldPath = (params->directory / std::wstring(fileNotify->FileName, filenameLength)).lexically_normal();
                }
                break;
            case FILE_ACTION_RENAMED_NEW_NAME: { change = FileSystemChangeRename; }
                break;
            }

            if (change != FileSystemChangeNone)
            {
                uint32 filenameLength = fileNotify->FileNameLength / sizeof(WCHAR);
                fs::path path = (params->directory / std::wstring(fileNotify->FileName, filenameLength)).lexically_normal();

                if (change == FileSystemChangeModify)
                {
                    auto writeTime = fs::last_write_time(path);

                    // The filesystem usually sends multiple notifications for changed files, since the file is first written, then metadata is changed etc.
                    // This check prevents these notifications if they are too close together in time.
                    // This is a pretty crude fix. In this setup files should not change at the same time, since we only ever track one file.
                    if (path == lastWritePath
                        && std::chrono::duration_cast<std::chrono::milliseconds>(writeTime - lastChangedTimeStamp).count() < 200)
                    {
                        lastWritePath = path;
                        lastChangedTimeStamp = writeTime;
                        break;
                    }

                    lastWritePath = path;
                    lastChangedTimeStamp = writeTime;
                }

                FileSystemEvent e;
                e.change = change;
                e.path = std::move(path);
                if (change == FileSystemChangeRename)
                {
                    e.oldPath = std::move(oldPath);
                }

                params->callback(e);
            }

            offset += fileNotify->NextEntryOffset;
        } while (fileNotify->NextEntryOffset != 0);

        if (!ResetEvent(overlapped.hEvent))
        {
            std::cerr << "ObserveDirectory: Reset event failed.\n";
        }
    }
    CloseHandle(directoryHandle);

    delete params;

    return 0;
}

bool ObserveDirectoryThread(const fs::path& directory, const FileSystemObserver& callback, bool watchSubDir)
{
    ObserveParams* params = new ObserveParams;
    params->directory = directory;
    params->callback = callback;
    params->watchSubDirs = watchSubDir;
    HANDLE handle = CreateThread(nullptr, 0, ObserveDirectory, params, 0, nullptr);
    bool result = handle != INVALID_HANDLE_VALUE;
    CloseHandle(handle);
    return result;
}