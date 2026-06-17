#include "GitHubTree.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

GitHubTree::GitHubTree(
    GitHubClient& client,
    std::string owner,
    std::string repo,
    std::string branch
)
    : m_client(client),
    m_owner(std::move(owner)),
    m_repo(std::move(repo)),
    m_branch(std::move(branch))
{
}

std::vector<GitHubFile> GitHubTree::GetLuaFiles()
{
    std::string url =
        "https://api.github.com/repos/" + m_owner + "/" + m_repo +
        "/git/trees/" + m_branch + "?recursive=1";

    std::string response = m_client.Get(url);
    json data = json::parse(response);

    std::vector<GitHubFile> files;

    for (const auto& item : data["tree"])
    {
        if (!item.contains("type") || item["type"] != "blob")
            continue;

        std::string path = item["path"];

        if (path.size() >= 4 && path.ends_with(".lua"))
        {
            files.push_back({
                path,
                item["sha"],
                item.value("url", "")
                });
        }
    }

    return files;
}