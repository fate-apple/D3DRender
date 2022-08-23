#include "pch.h"
#include "FileRegistry.h"
#include "Asset.h"
#include <unordered_map>
#include <yaml-cpp/node/convert.h>
#include <yaml-cpp/node/parse.h>

#include "FileSystem.h"
#include "Log.h"
#include "Yaml.h"

using PathToHandle =  std::unordered_map<fs::path, AssetHandle>;
using HandleToPath =  std::unordered_map<AssetHandle, fs::path>;
static PathToHandle pathToHandle;
static HandleToPath handleToPath;

static const fs::path registryPath = fs::path(L"resources/files.yaml").lexically_normal();
static std::mutex mutex;

// file path to unique id
static PathToHandle LoadRegistryFromDisk()
{
    PathToHandle result;
    std::ifstream stream(registryPath);
    YAML::Node root = YAML::Load(stream);
    for(auto entryNode : root) {
        AssetHandle handle{0};
        fs::path path;
        YAML_LOAD(entryNode, handle, "Handle");
        YAML_LOAD(entryNode, path, "Path");
        if(handle) {
            result[path] = handle;
        }
    }
    return result;
}

/**
 * \brief   recursive read file in path to initialize loadedRegistry: If already known, use the handle, otherwise generate one.
 * \param path              asset file directory
 * \param loadedRegistry    path to Handle map
 */
static void ReadDirectory(const fs::path& path, const PathToHandle& loadedRegistry)
{
    for (const auto& dirEntry : fs::directory_iterator(path))
    {
        const auto& path = dirEntry.path();
        if (dirEntry.is_directory())
        {
            ReadDirectory(path, loadedRegistry);
        }
        else
        {
            auto it = loadedRegistry.find(path);
            AssetHandle handle = (it != loadedRegistry.end()) ? it->second : AssetHandle::Generate();
            pathToHandle.insert({ path, handle });
            handleToPath.insert({ handle, path });
        }
    }
}

static void WriteRegistryToDisk()
{
    YAML::Node out;

    for (const auto& [path, handle] : pathToHandle)
    {
        YAML::Node n;
        n["Handle"] = handle;
        n["Path"] = path;
        out.push_back(n);
    }

    fs::create_directories(registryPath.parent_path());
    std::ofstream fout(registryPath);
    fout << out;
}


static void HandleAssetChange(const FileSystemEvent& e)
{
    if (!fs::is_directory(e.path) && e.path != registryPath)
    {
        mutex.lock();
        switch (e.change)
        {
        case FileSystemChangeAdd:
            {
                LOG_MESSAGE("Asset '%ws' added", e.path.c_str());

                assert(pathToHandle.find(e.path) == pathToHandle.end());

                AssetHandle handle = AssetHandle::Generate();
                pathToHandle.insert({ e.path, handle });
                handleToPath.insert({ handle, e.path });
            } break;

        case FileSystemChangeDelete:
            {
                LOG_MESSAGE("Asset '%ws' deleted", e.path.c_str());

                auto it = pathToHandle.find(e.path);

                assert(it != pathToHandle.end());

                AssetHandle handle = it->second;
                pathToHandle.erase(it);
                handleToPath.erase(handle);
            } break;

        case FileSystemChangeModify:
            {
                LOG_MESSAGE("Asset '%ws' modified", e.path.c_str());
            } break;

        case FileSystemChangeRename:
            {
                LOG_MESSAGE("Asset renamed from '%ws' to '%ws'", e.oldPath.c_str(), e.path.c_str());

                auto oldIt = pathToHandle.find(e.oldPath);

                assert(oldIt != pathToHandle.end()); // Old path exists.
                assert(pathToHandle.find(e.path) == pathToHandle.end()); // New path does not exist.

                AssetHandle handle = oldIt->second;
                pathToHandle.erase(oldIt);
                pathToHandle.insert({ e.path, handle });
                handleToPath[handle] = e.path; // Replace.
            } break;
        }
        mutex.unlock();

        // During runtime the registry is only written to in this function, so no need to protect the read with mutex.
        LOG_MESSAGE("Rewriting file registry");
        WriteRegistryToDisk();
    }
}

void InitializeFileRegistry()
{
    auto loadedRegistry = LoadRegistryFromDisk();
    ReadDirectory(L"assets", loadedRegistry);
    WriteRegistryToDisk();
    ObserveDirectoryThread(L"assets", HandleAssetChange);
}


AssetHandle GetAssetHandleFromPath(const fs::path& path)
{
    const std::lock_guard<std::mutex> lock(mutex);

    auto it = pathToHandle.find(path);
    if (it == pathToHandle.end())
    {
        return {};
    }
    return it->second;
}

fs::path GetPathFromAssetHandle(AssetHandle handle)
{
    const std::lock_guard<std::mutex> lock(mutex);

    auto it = handleToPath.find(handle);
    if (it == handleToPath.end())
    {
        return {};
    }
    return it->second;
}