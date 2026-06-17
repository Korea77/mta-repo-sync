#pragma once
#include "GitHubClient.h"
#include <string>
#include <vector>

struct GitHubFile
{
    std::string path;
    std::string sha;
    std::string url;
};

class GitHubTree
{
public:
    GitHubTree(GitHubClient& client, std::string owner, std::string repo, std::string branch);

    std::vector<GitHubFile> GetLuaFiles();

private:
    GitHubClient& m_client;
    std::string m_owner;
    std::string m_repo;
    std::string m_branch;
};