cmake_minimum_required(VERSION 3.21)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(PROJECT_VERSION "1.0.0.0" CACHE STRING "Project version")

include(target_add_versioninfo)
include(generate_ipp_translation_units)
include(configure_msvc_wine_intellisense)

project(CJMovement
	DESCRIPTION "GTA San Andreas movement and camera for CJ in Gothic 1"
	VERSION ${PROJECT_VERSION}
)

if (NOT PROJECT_VERSION_PATCH)
	set(PROJECT_VERSION_PATCH 0)
endif()

if (NOT PROJECT_VERSION_TWEAK)
	set(PROJECT_VERSION_TWEAK 0)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# By default template uses dynamically linked union-api target to avoid compatibility problems.
# If you want to produce a standalone plugin, set this option to OFF
set(GENERATE_UNION_API_DLL ON)

# By default template generates .vdf with the .dll and files form \vdf\_Work and \vdf\System
# If you want to produce only the .dll, set this option to OFF
set(GENERATE_VDF ON)

if (GENERATE_UNION_API_DLL AND NOT GENERATE_VDF)
	message(WARNING "GENERATE_UNION_API_DLL is ON but GENERATE_VDF is OFF. UnionAPI.dll won't be included in the release.")
endif()

file(GLOB_RECURSE IPP_FILES CONFIGURE_DEPENDS
	"src/*.ipp"
)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
	"src/*.cpp"
)

generate_ipp_translation_units(
	INPUT ${IPP_FILES}
	OUTPUT IPP_TRANSLATION_UNITS
)

add_library(${CMAKE_PROJECT_NAME} SHARED)

target_sources(${CMAKE_PROJECT_NAME}
	PRIVATE
		${SRC}
		${IPP_TRANSLATION_UNITS}
)

target_precompile_headers(${CMAKE_PROJECT_NAME}
	PRIVATE
		<Union/Hook.h>
		<ZenGin/zGothicAPI.h>
)

target_include_directories(${CMAKE_PROJECT_NAME}
	INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_include_directories(${CMAKE_PROJECT_NAME}
 	BEFORE PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/userapi"
)

target_add_versioninfo(${CMAKE_PROJECT_NAME}
	VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}"
	COMPANY_NAME "Gothic Community"
	PRODUCT_NAME "${CMAKE_PROJECT_NAME}"
)

if (GENERATE_VDF)
	set(VDF_DIR "${PROJECT_SOURCE_DIR}/vdf/")
	set(VDF_BUILD_DIR "${CMAKE_BINARY_DIR}/vdf/")
	set(VDF_BUILD_AUTORUN_DIR "${VDF_BUILD_DIR}System/Autorun/")

	file(COPY "${VDF_DIR}" DESTINATION "${VDF_BUILD_DIR}" FOLLOW_SYMLINK_CHAIN)

	set(VDF_BUILD_VM_FILE "${VDF_BUILD_DIR}build.vm")
	configure_file("${VDF_DIR}build.vm" "${VDF_BUILD_VM_FILE}")
	set(VDF_OUTPUT_FILE "${VDF_BUILD_DIR}${CMAKE_PROJECT_NAME}.vdf")

	if (GENERATE_UNION_API_DLL)
		add_custom_target(copy_union_api_dll
			ALL
				COMMAND ${CMAKE_COMMAND} -E make_directory ${VDF_BUILD_AUTORUN_DIR}
				COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:union_api_dll> ${VDF_BUILD_AUTORUN_DIR}
				COMMAND ${CMAKE_COMMAND} -E echo "[Post Build] $<TARGET_FILE_NAME:union_api_dll> copied to: ${VDF_BUILD_AUTORUN_DIR}"
			DEPENDS union_api_dll
		)
	endif()

	add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
		POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory ${VDF_BUILD_AUTORUN_DIR}
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${VDF_BUILD_AUTORUN_DIR}
			COMMAND ${CMAKE_COMMAND} -E echo "[Post Build] $<TARGET_FILE_NAME:${CMAKE_PROJECT_NAME}> copied to: ${VDF_BUILD_AUTORUN_DIR}")

	add_custom_target(build_vdf
		ALL
			COMMAND ${CMAKE_COMMAND} -E echo "[VDF] Building ${CMAKE_PROJECT_NAME}.vdf ..."
			COMMAND ${CMAKE_COMMAND}
				"-DVDF_EXECUTABLE=${VDF_BUILD_DIR}/GothicVDFS.exe"
				"-DVDF_SCRIPT=${VDF_BUILD_VM_FILE}"
				"-DVDF_OUTPUT=${VDF_OUTPUT_FILE}"
				"-DVDF_WORKING_DIRECTORY=${VDF_BUILD_DIR}"
				-P "${CMAKE_SOURCE_DIR}/cmake/run_gothic_vdfs.cmake"
			COMMAND ${CMAKE_COMMAND} -E echo "[VDF] ${CMAKE_PROJECT_NAME}.vdf build successfully!"
		WORKING_DIRECTORY "${VDF_BUILD_DIR}"
		DEPENDS ${CMAKE_PROJECT_NAME}
	)

	if(GENERATE_UNION_API_DLL)
		add_dependencies(build_vdf copy_union_api_dll)
	endif()
endif()

add_subdirectory(dependencies)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_EXPORT_COMPILE_COMMANDS)
	configure_msvc_wine_intellisense(
		TARGET ${CMAKE_PROJECT_NAME}
		INPUT ${IPP_FILES}
	)
endif()