# =============================================================================
#  Lost Objects - F4SE plugin
#  Vibecoded by AI (Claude, Anthropic). Project owner and tester: egnutv.
#  License: PolyForm Noncommercial 1.0.0 (see LICENSE and NOTICE.md)
# =============================================================================

# CommonLibF4 AV itself requires CMake 4.3.
cmake_minimum_required(VERSION 4.3.0)

project(
	LostObjects
	VERSION 0.4.2.0
	DESCRIPTION "Adds a workshop menu tab that collects every craftable workshop item which is not reachable in the build menu."
	LANGUAGES CXX
)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are not allowed. Use: cmake --preset release")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Same link-time optimisation settings as CommonLibF4 (it compiles with /GL).
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

# --- CommonLibF4 AV (MIT) ----------------------------------------------------
# One DLL for Old-Gen (1.10.163), Next-Gen (1.10.984) and the Anniversary
# Edition (1.11.191 and newer, through the Address Library).
# Pinned to an exact commit so every build uses the same code.

set(COMMONLIB_RUNTIME_VR OFF CACHE BOOL "" FORCE)
set(COMMONLIB_RUNTIME_OG ON CACHE BOOL "" FORCE)
set(COMMONLIB_RUNTIME_NG ON CACHE BOOL "" FORCE)
set(COMMONLIB_RUNTIME_AE ON CACHE BOOL "" FORCE)
set(COMMONLIB_OPTION_FMT ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
	commonlibf4
	GIT_REPOSITORY https://github.com/LucaDotGit/CommonLibF4.git
	GIT_TAG e337150ac993daaa9d089fcddf7a6ce9a6bad42f
	GIT_SHALLOW FALSE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(commonlibf4)

# --- Plugin metadata (used by F4SE and the DLL's file properties) ------------

set(COMMONLIB_PLUGIN_NAME "${PROJECT_NAME}")
set(COMMONLIB_PLUGIN_AUTHOR "egnutv")
set(COMMONLIB_PLUGIN_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(COMMONLIB_PLUGIN_COMMENTS "Vibecoded by AI (Claude, Anthropic). AI-generated code can contain mistakes.")
set(COMMONLIB_PLUGIN_VERSION "${PROJECT_VERSION}")
set(COMMONLIB_PLUGIN_COPYRIGHT_YEAR "2026")
set(COMMONLIB_PLUGIN_LICENSE "PolyForm Noncommercial 1.0.0")

include("${commonlibf4_SOURCE_DIR}/cmake/GenerateXsePluginInfo.cmake")
generate_xse_plugin_info("${commonlibf4_SOURCE_DIR}")

# --- Plugin -----------------------------------------------------------------

set(SOURCES
	src/Forms.cpp
	src/Grouping.cpp
	src/Matcher.cpp
	src/ModelCheck.cpp
	src/Inventory.cpp
	src/Main.cpp
	src/MenuBuilder.cpp
	src/MenuDump.cpp
	src/NameMap.cpp
	src/Notice.cpp
	src/Recovery.cpp
	src/Report.cpp
	src/Scanner.cpp
	src/Settings.cpp
)

set(HEADERS
	src/Forms.h
	src/Grouping.h
	src/Inventory.h
	src/MenuBuilder.h
	src/MenuDump.h
	src/NameMap.h
	src/Notice.h
	src/PCH.h
	src/PluginForms.h
	src/Recovery.h
	src/Report.h
	src/Scanner.h
	src/Settings.h
	src/Util.h
)

add_library(
	"${PROJECT_NAME}"
	SHARED
		${SOURCES}
		${HEADERS}
		"${XSE_PLUGIN_INFO_CPP_TARGET}"
		"${WIN32_VERSION_TARGET}"
)

target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${CMAKE_CURRENT_BINARY_DIR}/include"
)

target_precompile_headers(
	"${PROJECT_NAME}"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src/PCH.h"
)

target_link_libraries(
	"${PROJECT_NAME}"
	PRIVATE
		CommonLibF4::CommonLibF4
)

# CommonLibF4 turns every warning into an error for everything that links to
# it. Keep the warnings visible, but do not fail the build because of them.
if(MSVC)
	target_compile_options("${PROJECT_NAME}" PRIVATE "/WX-")
	target_link_options("${PROJECT_NAME}" PRIVATE "/WX:NO")
endif()

# --- Packaging helpers -------------------------------------------------------

# Copies the finished DLL straight into a mod folder or the game, if one of
# these environment variables is set (optional, for local testing):
#   XSE_FO4_MODS_PATH  - e.g. MO2's "mods" folder
#   XSE_FO4_GAME_PATH  - the Fallout 4 game folder
include("${commonlibf4_SOURCE_DIR}/cmake/SetXseOutputPath.cmake")
include("${commonlibf4_SOURCE_DIR}/cmake/CopyXseOutput.cmake")
set_xse_output_path("${PROJECT_NAME}")
copy_xse_copy_output("${PROJECT_NAME}")
