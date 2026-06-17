# mta-repo-sync

`mta-repo-sync` is a module for **Multi Theft Auto: San Andreas** that downloads and synchronizes server resource files from a GitHub repository.
## Features

* Synchronize MTA Server resources from a GitHub repository
* Supports public and private repositories
* Uses GitHub tree/blob API
* Downloads only missing or changed files
* Stores local SHA manifest to avoid unnecessary downloads
* Works asynchronously, allowing Lua main thread to continue executing while the sync is in progress
* Supports Windows `.dll` and Linux `.so` builds
* Linux build can statically link dependencies such as curl, Lua, OpenSSL, and zlib

## How it works

The Lua resource calls:

```lua
github_connect("OWNER", "REPO", "master", "TOKEN")
github_sync("resources", "")
```

This module then:

1. The module connecting to the repository on GitHub.
2. The repository tree being downloaded.
3. Checking files in the selected remote directory.
4. Comparing GitHub files with the manifest using their SHAs.
5. Downloading any changed or missing files.
6. Writing the files to:

```text
mods/deathmatch/resources/
```

7. Returns a list of changed resource names to Lua.
8. Lua can then restart those resources manually.

## Example repository structure

Remote GitHub repository:

```text
resources/
├── my_resource/
│   ├── meta.xml
│   └── server.lua
└── another_resource/
    ├── meta.xml
    └── client.lua
```

Local MTA server after sync:

```text
mods/deathmatch/resources/
├── my_resource/
│   ├── meta.xml
│   └── server.lua
└── another_resource/
    ├── meta.xml
    └── client.lua
```

## Lua API

### `github_connect(owner, repo, branch, token)`

Configures the GitHub repository.

```lua
local ok = github_connect("OWNER", "REPO", "master", "")
```

Arguments:

| Argument | Description                                     |
| -------- | ----------------------------------------------- |
| `owner`  | GitHub username or organization                 |
| `repo`   | Repository name                                 |
| `branch` | Branch name, for example `master` or `main`     |
| `token`  | GitHub token, required for private repositories |

For public repositories, the token can be an empty string.

```lua
github_connect("OWNER", "REPO", "master", "")
```

For private repositories:

```lua
github_connect("OWNER", "REPO", "master", "github_pat_xxxxx")
```

Do not hardcode production tokens in public Lua files.

---

### `github_sync(remoteFolder, localFolder)`

Starts synchronization in a background thread.

```lua
local ok, err = github_sync("resources", "")
```

Arguments:

| Argument       | Description                                                  |
| -------------- | ------------------------------------------------------------ |
| `remoteFolder` | Folder inside the GitHub repository                          |
| `localFolder`  | Optional local subfolder inside `mods/deathmatch/resources/` |

Example:

```lua
github_sync("resources", "")
```

This maps:

```text
GitHub: resources/my_resource/server.lua
Local:  mods/deathmatch/resources/my_resource/server.lua
```

Example with local subfolder:

```lua
github_sync("resources", "downloaded")
```

This maps:

```text
GitHub: resources/my_resource/server.lua
Local:  mods/deathmatch/resources/downloaded/my_resource/server.lua
```

---

### `github_is_busy()`

Returns whether a sync operation is currently running.

```lua
if github_is_busy() then
    outputDebugString("Sync is already running")
end
```

---

### `github_has_result()`

Returns whether the background sync worker has finished and produced a result.

```lua
if github_has_result() then
    -- read result
end
```

---

### `github_get_status()`

Returns the current module status.

Possible values:

```text
idle
connected
syncing
done
error: ...
```

---

### `github_get_changed_resources()`

Returns a Lua table containing names of resources that were changed.

```lua
local changedResources = github_get_changed_resources()

for _, resourceName in ipairs(changedResources) do
    outputDebugString("Changed resource: " .. resourceName)
end
```

---

### `github_clear_result()`

Clears the last sync result.

```lua
github_clear_result()
```

## Example Lua usage

```lua
local GITHUB_OWNER = "OWNER"
local GITHUB_REPO = "REPO"
local GITHUB_BRANCH = "master"
local GITHUB_TOKEN = ""

local REMOTE_FOLDER = "resources"
local LOCAL_FOLDER = ""

addEventHandler("onResourceStart", resourceRoot, function()
    local ok = github_connect(
        GITHUB_OWNER,
        GITHUB_REPO,
        GITHUB_BRANCH,
        GITHUB_TOKEN
    )

    outputDebugString("[mta-repo-sync] github_connect: " .. tostring(ok))
end)

addCommandHandler("gitsync", function()
    if github_is_busy() then
        outputDebugString("[mta-repo-sync] Sync is already running")
        return
    end

    local ok, err = github_sync(REMOTE_FOLDER, LOCAL_FOLDER)

    if not ok then
        outputDebugString("[mta-repo-sync] Sync failed to start: " .. tostring(err), 2)
        return
    end

    outputDebugString("[mta-repo-sync] Sync started")
end)

setTimer(function()
    if not github_has_result() then
        return
    end

    local status = github_get_status()
    local changedResources = github_get_changed_resources()

    outputDebugString("[mta-repo-sync] Sync status: " .. tostring(status))

    if status == "done" then
        local updaterResourceName = getResourceName(getThisResource())

        for _, resourceName in ipairs(changedResources) do
            outputDebugString("[mta-repo-sync] Changed resource: " .. tostring(resourceName))

            if resourceName ~= updaterResourceName then
                local resource = getResourceFromName(resourceName)

                if resource then
                    outputDebugString("[mta-repo-sync] Restarting resource: " .. resourceName)
                    restartResource(resource)
                else
                    outputDebugString("[mta-repo-sync] Resource not found: " .. resourceName, 2)
                end
            else
                outputDebugString("[mta-repo-sync] Skipping updater resource restart")
            end
        end
    else
        outputDebugString("[mta-repo-sync] Sync error: " .. tostring(status), 2)
    end

    github_clear_result()
end, 500, 0)
```

## Building on Windows

Build:

```bat
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output:

```text
build/Release/mta-repo-sync.dll
```

## Building on Linux

```bash
cmake -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j$(nproc)
```

Output:

```text
build-linux/mta-repo-sync.so
```


## Docker - Linux build


```dockerfile
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    build-essential \
    cmake \
    g++ \
    make \
    file \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /project

CMD rm -rf build-linux && \
    cmake -B build-linux -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build-linux -- -j$(nproc) && \
    file build-linux/mta-repo-sync.so && \
    ldd build-linux/mta-repo-sync.so
```

Build Docker image:

```bat
docker build -f Dockerfile.build -t mta-repo-sync-builder-20 .
```

Run build from Windows CMD:

```bat
docker run --rm -v "%cd%:/project" mta-repo-sync-builder-20
```

The Linux module will be generated at:

```text
build-linux/mta-repo-sync.so
```


## Notes

* Resource restarts should be handled from Lua.
* The updater resource should not restart itself.
* File writes are restricted to `mods/deathmatch/resources/`.

## License

MIT License

