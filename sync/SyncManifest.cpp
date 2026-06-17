#include "SyncManifest.h"
#include "PathUtils.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace RepoSync
{
    namespace fs = std::filesystem;
    using json = nlohmann::json;

    SyncManifest LoadManifest()
    {
        SyncManifest manifest;

        fs::path path = GetManifestPath();

        if (!fs::exists(path))
            return manifest;

        std::ifstream file(path);
        if (!file.is_open())
            return manifest;

        json data;
        file >> data;

        if (!data.contains("files") || !data["files"].is_object())
            return manifest;

        for (auto& [filePath, sha] : data["files"].items())
        {
            manifest[filePath] = sha.get<std::string>();
        }

        return manifest;
    }

    void SaveManifest(const SyncManifest& manifest)
    {
        fs::create_directories(GetResourcesRoot());

        json data;
        data["files"] = json::object();

        for (const auto& [filePath, sha] : manifest)
        {
            data["files"][filePath] = sha;
        }

        std::ofstream file(GetManifestPath(), std::ios::trunc);
        file << data.dump(4);
    }
}