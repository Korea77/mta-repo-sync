cmake_minimum_required(VERSION 3.8)

project("mta-repo-sync" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mta-repo-sync SHARED
    "mta-repo-sync.cpp"
    "mta-repo-sync.h"

    "github/GitHubClient.h"
    "github/GitHubClient.cpp"
    "github/GitHubDownload.h"
    "github/GitHubDownload.cpp"

    "sync/SyncManifest.h"
    "sync/SyncManifest.cpp"
    "sync/PathUtils.h"
    "sync/PathUtils.cpp"

    "utils/Base64.h"
    "utils/Base64.cpp"

)

target_include_directories(mta-repo-sync PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(mta-repo-sync PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/libcurl_a.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/lua51.lib"

    ws2_32
    wldap32
    crypt32
    normaliz
)

target_compile_definitions(mta-repo-sync PRIVATE
    CURL_STATICLIB
)

if (MSVC)
    target_compile_options(mta-repo-sync PRIVATE /utf-8)
endif()