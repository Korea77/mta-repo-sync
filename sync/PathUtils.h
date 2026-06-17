#pragma once

#include <filesystem>
#include <string>

namespace RepoSync
{
    namespace fs = std::filesystem;

    fs::path GetResourcesRoot();
    fs::path GetManifestPath();

    bool IsSafeRelativePath(const fs::path& path);

    std::string NormalizeSlash(std::string path);
    bool StartsWithFolder(const std::string& path, const std::string& folder);
    std::string RemoveRemotePrefix(const std::string& path, const std::string& remoteFolder);
    std::string GetResourceNameFromPath(const std::string& localRelativePath);
}