#pragma once

#include <string>
#include <unordered_map>

namespace RepoSync
{
    using SyncManifest = std::unordered_map<std::string, std::string>;

    SyncManifest LoadManifest();
    void SaveManifest(const SyncManifest& manifest);
}