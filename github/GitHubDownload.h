#pragma once

#include "GitHubClient.h"

#include <string>

namespace RepoSync
{
    std::string DownloadBlobContent(GitHubClient& client, const std::string& blobUrl);
}