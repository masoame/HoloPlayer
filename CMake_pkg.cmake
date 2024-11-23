include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY  https://github.com/nlohmann/json.git
    GIT_TAG  v3.11.3
)

FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    curl
    GIT_REPOSITORY  https://github.com/curl/curl.git
    GIT_TAG curl-8_11_0
)

FetchContent_MakeAvailable(curl)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9
)

FetchContent_MakeAvailable(SDL)

add_subdirectory(external_library)
