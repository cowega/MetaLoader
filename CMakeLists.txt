cmake_minimum_required(VERSION 3.15)
cmake_policy(VERSION 3.15)

project(MetaLoader VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(FetchContent)

set(MH_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(MH_ENABLE_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        1e9ad1eb42db11bfcb65461f687c656612d1b555
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.12.0
)

FetchContent_Declare(
    sigsearch
    GIT_REPOSITORY https://github.com/LADIlib/SigSearch.git
    GIT_TAG        a9aa2bc59ccfa60766ca7fb74193d2ab0126c56a
)

FetchContent_MakeAvailable(minhook spdlog sigsearch)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${sigsearch_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    minhook 
    spdlog::spdlog
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "metaloader"
    SUFFIX ".dll"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)