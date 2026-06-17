#pragma once
#include <string>

class GitHubClient
{
public:
    explicit GitHubClient(std::string token = "");

    std::string Get(const std::string& url);

private:
    std::string m_token;
};