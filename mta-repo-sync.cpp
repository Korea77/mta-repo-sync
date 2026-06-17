#include "mta/ILuaModuleManager.hpp"

#include "github/GitHubClient.h"
#include "github/GitHubDownload.h"
#include "sync/SyncManifest.h"
#include "sync/PathUtils.h"

#include <lua/lua.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <unordered_set>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#define MTAEXPORT extern "C" __declspec(dllexport)
#else
#define MTAEXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;


static ILuaModuleManager10* g_pModuleManager = nullptr;

struct GitHubConfig
{
    std::string owner;
    std::string repo;
    std::string branch = "main";
    std::string token;
};


static GitHubConfig g_config;
static std::mutex g_configMutex;

static std::atomic<bool> g_busy = false;
static std::atomic<bool> g_resultReady = false;

static std::mutex g_resultMutex;
static std::vector<std::string> g_changedResources;
static std::string g_status = "idle";

MTAEXPORT bool InitModule(
    void** pManager,
    char* szModuleName,
    char* szAuthor,
    float* fVersion
) {

    strcpy(szModuleName, "mta-repo-sync");
    strcpy(szAuthor, "Korea");
    *fVersion = 1.0f;
    g_pModuleManager = (ILuaModuleManager10*)pManager;


    return true;
}

MTAEXPORT bool ShutdownModule() {
    return true;
}



MTAEXPORT bool DoPulse()
{

    return true;
}

static int lua_github_connect(lua_State* luaVM)
{
    const char* owner = lua_tostring(luaVM, 1);
    const char* repo = lua_tostring(luaVM, 2);
    const char* branch = luaL_optstring(luaVM, 3, "main");
    const char* token = luaL_optstring(luaVM, 4, "");

    {
        std::lock_guard<std::mutex> lock(g_configMutex);

        g_config.owner = owner;
        g_config.repo = repo;
        g_config.branch = branch;
        g_config.token = token;
    }

    {
        std::lock_guard<std::mutex> lock(g_resultMutex);
        g_status = "connected";
    }

    lua_pushboolean(luaVM, true);
    return 1;
}


static int lua_github_sync(lua_State* luaVM)
{
    const char* remoteFolderArg = luaL_checkstring(luaVM, 1);
    const char* localFolderArg = luaL_optstring(luaVM, 2, "");

    if (g_busy.load())
    {
        lua_pushboolean(luaVM, false);
        lua_pushstring(luaVM, "sync already running");
        return 2;
    }

    GitHubConfig config;

    {
        std::lock_guard<std::mutex> lock(g_configMutex);
        config = g_config;
    }

    if (config.owner.empty() || config.repo.empty())
    {
        lua_pushboolean(luaVM, false);
        lua_pushstring(luaVM, "github_connect was not called");
        return 2;
    }

    std::string remoteFolder = RepoSync::NormalizeSlash(remoteFolderArg);
    std::string localFolder = RepoSync::NormalizeSlash(localFolderArg);

    g_busy = true;
    g_resultReady = false;

    {
        std::lock_guard<std::mutex> lock(g_resultMutex);
        g_changedResources.clear();
        g_status = "syncing";
    }

    std::thread([config, remoteFolder, localFolder]()
        {
            try
            {
                GitHubClient client(config.token);

                std::string treeUrl =
                    "https://api.github.com/repos/" + config.owner + "/" + config.repo +
                    "/git/trees/" + config.branch + "?recursive=1";

                std::string treeResponse = client.Get(treeUrl);
                json treeData = json::parse(treeResponse);

                if (!treeData.contains("tree") || !treeData["tree"].is_array())
                    throw std::runtime_error("Invalid GitHub tree response");

                auto manifest = RepoSync::LoadManifest();

                std::unordered_set<std::string> changedResources;

                fs::create_directories(RepoSync::GetResourcesRoot());

                for (const auto& item : treeData["tree"])
                {
                    if (!item.contains("type") || item["type"] != "blob")
                        continue;

                    std::string repoPath = RepoSync::NormalizeSlash(
                        item["path"].get<std::string>()
                    );

                    if (!RepoSync::StartsWithFolder(repoPath, remoteFolder))
                        continue;

                    std::string relativeFromRemote =
                        RepoSync::RemoveRemotePrefix(repoPath, remoteFolder);

                    if (relativeFromRemote.empty())
                        continue;

                    std::string localRelative;

                    if (localFolder.empty())
                        localRelative = relativeFromRemote;
                    else
                        localRelative = localFolder + "/" + relativeFromRemote;

                    localRelative = RepoSync::NormalizeSlash(localRelative);

                    fs::path safeRelativePath =
                        fs::path(localRelative).lexically_normal();

                    if (!RepoSync::IsSafeRelativePath(safeRelativePath))
                        throw std::runtime_error("Unsafe repo path: " + localRelative);

                    fs::path outputPath =
                        RepoSync::GetResourcesRoot() / safeRelativePath;

                    std::string sha = item["sha"].get<std::string>();

                    bool existsLocally = fs::exists(outputPath);

                    bool shaChanged =
                        !manifest.contains(localRelative) ||
                        manifest[localRelative] != sha;

                    if (!existsLocally || shaChanged)
                    {
                        std::string blobUrl = item["url"].get<std::string>();

                        std::string content =
                            RepoSync::DownloadBlobContent(client, blobUrl);

                        fs::create_directories(outputPath.parent_path());

                        std::ofstream output(
                            outputPath,
                            std::ios::binary | std::ios::trunc
                        );

                        if (!output.is_open())
                        {
                            throw std::runtime_error(
                                "Cannot write file: " + outputPath.string()
                            );
                        }

                        output.write(
                            content.data(),
                            static_cast<std::streamsize>(content.size())
                        );

                        output.close();

                        manifest[localRelative] = sha;

                        std::string resourceName =
                            RepoSync::GetResourceNameFromPath(localRelative);

                        if (!resourceName.empty())
                            changedResources.insert(resourceName);
                    }
                }

                RepoSync::SaveManifest(manifest);

                {
                    std::lock_guard<std::mutex> lock(g_resultMutex);

                    g_changedResources.assign(
                        changedResources.begin(),
                        changedResources.end()
                    );

                    g_status = "done";
                }

                g_resultReady = true;
            }
            catch (const std::exception& e)
            {
                {
                    std::lock_guard<std::mutex> lock(g_resultMutex);

                    g_changedResources.clear();
                    g_status = std::string("error: ") + e.what();
                }

                g_resultReady = true;
            }

            g_busy = false;
        }).detach();

    lua_pushboolean(luaVM, true);
    return 1;
}

static int lua_github_is_busy(lua_State* luaVM)
{
    lua_pushboolean(luaVM, g_busy.load());
    return 1;
}

static int lua_github_get_status(lua_State* luaVM)
{
    std::lock_guard<std::mutex> lock(g_resultMutex);
    lua_pushstring(luaVM, g_status.c_str());
    return 1;
}

static int lua_github_has_result(lua_State* luaVM)
{
    lua_pushboolean(luaVM, g_resultReady.load());
    return 1;
}

static int lua_github_get_changed_resources(lua_State* luaVM)
{
    std::lock_guard<std::mutex> lock(g_resultMutex);

    lua_newtable(luaVM);

    int index = 1;

    for (const auto& resourceName : g_changedResources)
    {
        lua_pushinteger(luaVM, index++);
        lua_pushstring(luaVM, resourceName.c_str());
        lua_settable(luaVM, -3);
    }

    return 1;
}

static int lua_github_clear_result(lua_State* luaVM)
{
    {
        std::lock_guard<std::mutex> lock(g_resultMutex);

        g_changedResources.clear();

        if (!g_busy.load())
            g_status = "idle";
    }

    g_resultReady = false;

    lua_pushboolean(luaVM, true);
    return 1;
}


MTAEXPORT void RegisterFunctions(lua_State* luaVM)
{

    g_pModuleManager->RegisterFunction(luaVM, "github_connect", lua_github_connect);
    g_pModuleManager->RegisterFunction(luaVM, "github_sync", lua_github_sync);
    g_pModuleManager->RegisterFunction(luaVM, "github_has_result", lua_github_has_result);
    g_pModuleManager->RegisterFunction(luaVM, "github_get_changed_resources", lua_github_get_changed_resources);
    g_pModuleManager->RegisterFunction(luaVM, "github_clear_result", lua_github_clear_result);
    g_pModuleManager->RegisterFunction(luaVM, "github_is_busy", lua_github_is_busy);
    g_pModuleManager->RegisterFunction(luaVM, "github_get_status", lua_github_get_status);
}

MTAEXPORT void ResourceStopping(lua_State* luaVM) {
}

MTAEXPORT void ResourceStopped(lua_State* luaVM) {
}