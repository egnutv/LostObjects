# Builds the Papyrus compiler Caprica and compiles the scripts of the settings
# holotape (data/Scripts/Source/User/*.psc) into build/papyrus/Scripts/*.pex.
#
# Caprica (https://github.com/nikitalita/Caprica, MIT) is downloaded at a
# pinned commit and only used as a build tool - it is not part of the mod.
# Boost comes from vcpkg (see vcpkg.json next to this file). Windows only,
# like Caprica itself.
#
#   cmake -S tools/papyrus -B build/papyrus -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static-md-release -DVCPKG_HOST_TRIPLET=x64-windows-static-md-release
#   cmake --build build/papyrus --config Release
#
# The compiled scripts land in build/papyrus/Scripts.

cmake_minimum_required(VERSION 3.21)
project(LostObjectsPapyrus LANGUAGES CXX)

if(NOT WIN32)
	message(FATAL_ERROR "Caprica only builds on Windows (MSVC).")
endif()

include(FetchContent)
FetchContent_Declare(
	caprica
	GIT_REPOSITORY https://github.com/nikitalita/Caprica.git
	GIT_TAG 9cd6d32ec2750adae329e81f7276dac95251e000
)
FetchContent_MakeAvailable(caprica)

set(LO_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(LO_SCRIPT_SOURCES
	"${LO_REPO_ROOT}/data/Scripts/Source/User/LostObjects.psc"
	"${LO_REPO_ROOT}/data/Scripts/Source/User/LostObjects_Holotape.psc"
)
set(LO_SCRIPT_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Scripts")

# One compiler run for the whole script folder. The import folder holds the
# compile-time stubs of the base game scripts we extend (tools/papyrus/stubs).
add_custom_command(
	OUTPUT "${LO_SCRIPT_OUTPUT}/LostObjects.pex" "${LO_SCRIPT_OUTPUT}/LostObjects_Holotape.pex"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${LO_SCRIPT_OUTPUT}"
	COMMAND $<TARGET_FILE:Caprica>
		--game fallout4
		--import "${CMAKE_CURRENT_SOURCE_DIR}/stubs"
		--output "${LO_SCRIPT_OUTPUT}"
		"${LO_REPO_ROOT}/data/Scripts/Source/User"
	DEPENDS Caprica ${LO_SCRIPT_SOURCES}
	COMMENT "Compiling the Lost Objects Papyrus scripts"
	VERBATIM
)
add_custom_target(lostobjects_scripts ALL
	DEPENDS "${LO_SCRIPT_OUTPUT}/LostObjects.pex" "${LO_SCRIPT_OUTPUT}/LostObjects_Holotape.pex"
)
