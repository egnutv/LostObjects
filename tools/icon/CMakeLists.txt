# Builds the icon mesh tool (make_icon_nif) on Windows, Linux or macOS.
# nifly (https://github.com/ousnius/nifly, GPL-3.0) is downloaded at a pinned
# commit and only used by this build tool - it is not part of the mod.
#
#   cmake -S tools/icon -B build/icon
#   cmake --build build/icon --config Release
#   build/icon/make_icon_nif data/Meshes/LostObjects/MenuIcon.nif        (Linux/macOS)
#   build\icon\Release\make_icon_nif.exe data\Meshes\LostObjects\MenuIcon.nif   (Windows)

cmake_minimum_required(VERSION 3.21)
project(LostObjectsIconTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
	nifly
	GIT_REPOSITORY https://github.com/ousnius/nifly.git
	GIT_TAG 6cb15fb5da71527b33cea8ff7913be8790e1c633
)
FetchContent_MakeAvailable(nifly)

add_executable(make_icon_nif make_icon_nif.cpp)
target_link_libraries(make_icon_nif PRIVATE nifly)
target_include_directories(make_icon_nif PRIVATE "${nifly_SOURCE_DIR}/include" "${nifly_SOURCE_DIR}/external")
