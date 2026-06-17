#include "PathUtils.h"

namespace RepoSync
{
    fs::path GetResourcesRoot()
    {
        return fs::path("mods") / "deathmatch" / "resources";
    }

    fs::path GetManifestPath()
    {
        return GetResourcesRoot() / ".mta-repo-sync.json";
    }

    bool IsSafeRelativePath(const fs::path& path)
    {
        if (path.empty())
            return false;

        if (path.is_absolute())
            return false;

        for (const auto& part : path)
        {
            if (part == "..")
                return false;
        }

        return true;
    }

    std::string NormalizeSlash(std::string path)
    {
        for (char& c : path)
        {
            if (c == '\\')
                c = '/';
        }

        while (!path.empty() && path.front() == '/')
            path.erase(path.begin());

        while (!path.empty() && path.back() == '/')
            path.pop_back();

        return path;
    }

    bool StartsWithFolder(const std::string& path, const std::string& folder)
    {
        if (folder.empty())
            return true;

        return path == folder || path.rfind(folder + "/", 0) == 0;
    }

    std::string RemoveRemotePrefix(const std::string& path, const std::string& remoteFolder)
    {
        if (remoteFolder.empty())
            return path;

        if (path == remoteFolder)
            return "";

        return path.substr(remoteFolder.size() + 1);
    }

    std::string GetResourceNameFromPath(const std::string& localRelativePath)
    {
        std::string normalized = NormalizeSlash(localRelativePath);

        size_t slash = normalized.find('/');
        if (slash == std::string::npos)
            return normalized;

        return normalized.substr(0, slash);
    }
}