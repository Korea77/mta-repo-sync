#include "GitHubClient.h"
#include <curl/curl.h>
#include <stdexcept>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

GitHubClient::GitHubClient(std::string token)
    : m_token(std::move(token))
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

std::string GitHubClient::Get(const std::string& url)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("curl_easy_init failed");

    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: mta-repo-sync");
    headers = curl_slist_append(headers, "Accept: application/vnd.github+json");

    std::string authHeader;
    if (!m_token.empty())
    {
        authHeader = "Authorization: Bearer " + m_token;
        headers = curl_slist_append(headers, authHeader.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode result = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(result));

    if (httpCode < 200 || httpCode >= 300)
        throw std::runtime_error("GitHub HTTP error: " + std::to_string(httpCode));

    return response;
}