#include "GitHubDownload.h"
#include "../utils/Base64.h"

#include <nlohmann/json.hpp>
#include <stdexcept>

namespace RepoSync
{
    using json = nlohmann::json;

    std::string DownloadBlobContent(GitHubClient& client, const std::string& blobUrl)
    {
        std::string response = client.Get(blobUrl);

        json data = json::parse(response);

        if (!data.contains("content"))
            throw std::runtime_error("GitHub blob has no content");

        std::string encoding = data.value("encoding", "");

        if (encoding != "base64")
            throw std::runtime_error("Unsupported GitHub blob encoding: " + encoding);

        return Base64Decode(data["content"].get<std::string>());
    }
}