cmake_minimum_required(VERSION 3.21)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(PROJECT_VERSION "1.0.0.0" CACHE STRING "Project version")

include(target_add_versioninfo)
include(generate_ipp_translation_units)
include(configure_msvc_wine_intellisense)
include(add_union_plugin)

# Every mashup's Union plugin is built here with one copy of the template's build and of the Union
# libraries: one DLL packed into one VDF per mashup (add_union_plugin in cmake/add_union_plugin.cmake).
project(GothicMashups
	DESCRIPTION "Union plugins of the Gothic 1 mashups"
	VERSION ${PROJECT_VERSION}
)

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

add_subdirectory(dependencies)

# mashups/<game>/plugin: its CMakeLists.txt names the plugin, its src/ has the code. The build goes
# to out/build/<preset>/<game>/vdf/<plugin>.vdf.
add_subdirectory(../mashups/gta/plugin gta)
add_subdirectory(../mashups/witcher/plugin witcher)
add_subdirectory(../mashups/minecraft/plugin minecraft)
