# Cross-compile 32-bit Windows binaries with mstorsjo/msvc-wine.
# A CMake cache value (for example from CMakeUserConfigPresets.json) takes
# precedence over the MSVC_WINE_ROOT environment variable and the default.
set(_MSVC_WINE_DEFAULT_ROOT "$ENV{HOME}/my_msvc/opt/msvc")
if(DEFINED ENV{MSVC_WINE_ROOT} AND NOT "$ENV{MSVC_WINE_ROOT}" STREQUAL "")
    set(_MSVC_WINE_DEFAULT_ROOT "$ENV{MSVC_WINE_ROOT}")
endif()

set(MSVC_WINE_ROOT "${_MSVC_WINE_DEFAULT_ROOT}" CACHE PATH
    "Root of the installed msvc-wine toolchain")

if(MSVC_WINE_ROOT STREQUAL "")
    message(FATAL_ERROR
        "MSVC_WINE_ROOT is empty. Set it in CMakeUserConfigPresets.json, "
        "as an environment variable, or with -DMSVC_WINE_ROOT=/path/to/msvc.")
endif()

set(_MSVC_WINE_BIN "${MSVC_WINE_ROOT}/bin/x86")

file(GLOB _MSVC_WINE_MSVC_DIRS LIST_DIRECTORIES true
    "${MSVC_WINE_ROOT}/VC/Tools/MSVC/*")
file(GLOB _MSVC_WINE_SDK_DIRS LIST_DIRECTORIES true
    "${MSVC_WINE_ROOT}/Windows Kits/10/Include/*")
list(SORT _MSVC_WINE_MSVC_DIRS COMPARE NATURAL ORDER DESCENDING)
list(SORT _MSVC_WINE_SDK_DIRS COMPARE NATURAL ORDER DESCENDING)

if(NOT _MSVC_WINE_MSVC_DIRS OR NOT _MSVC_WINE_SDK_DIRS)
    message(FATAL_ERROR
        "Could not locate the MSVC or Windows SDK include directories below "
        "${MSVC_WINE_ROOT}. Set MSVC_WINE_ROOT in "
        "CMakeUserConfigPresets.json, as an environment variable, or with "
        "-DMSVC_WINE_ROOT=/path/to/msvc. The selected directory must contain "
        "bin/x86, VC/Tools/MSVC, and Windows Kits/10.")
endif()

list(GET _MSVC_WINE_MSVC_DIRS 0 _MSVC_WINE_MSVC_DIR)
list(GET _MSVC_WINE_SDK_DIRS 0 _MSVC_WINE_SDK_DIR)

set(_MSVC_WINE_SYSTEM_INCLUDES
    "${_MSVC_WINE_MSVC_DIR}/atlmfc/include"
    "${_MSVC_WINE_MSVC_DIR}/include"
    "${_MSVC_WINE_SDK_DIR}/shared"
    "${_MSVC_WINE_SDK_DIR}/ucrt"
    "${_MSVC_WINE_SDK_DIR}/um"
    "${_MSVC_WINE_SDK_DIR}/winrt"
)

foreach(_tool cl link rc mt)
    if(NOT EXISTS "${_MSVC_WINE_BIN}/${_tool}")
        message(FATAL_ERROR
            "msvc-wine tool '${_tool}' was not found at "
            "${_MSVC_WINE_BIN}/${_tool}. Set MSVC_WINE_ROOT to the directory "
            "containing bin/x86.")
    endif()
endforeach()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/msvc-wine-link.in"
    "${CMAKE_BINARY_DIR}/msvc-wine-link"
    @ONLY
)
file(CHMOD "${CMAKE_BINARY_DIR}/msvc-wine-link"
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)

set(CMAKE_C_COMPILER "${_MSVC_WINE_BIN}/cl" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${_MSVC_WINE_BIN}/cl" CACHE FILEPATH "" FORCE)
set(CMAKE_LINKER "${CMAKE_BINARY_DIR}/msvc-wine-link" CACHE FILEPATH "" FORCE)
set(CMAKE_RC_COMPILER "${_MSVC_WINE_BIN}/rc" CACHE FILEPATH "" FORCE)
set(CMAKE_MT "${_MSVC_WINE_BIN}/mt" CACHE FILEPATH "" FORCE)

# The Wine wrappers provide these paths through INCLUDE, which real MSVC can
# consume but Linux editor tooling cannot discover. Emitting them explicitly
# also puts them into compile_commands.json for clangd and VS Code cpptools.
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES
    "${_MSVC_WINE_SYSTEM_INCLUDES}" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
    "${_MSVC_WINE_SYSTEM_INCLUDES}" CACHE STRING "" FORCE)

unset(_MSVC_WINE_BIN)
unset(_MSVC_WINE_DEFAULT_ROOT)
unset(_MSVC_WINE_MSVC_DIRS)
unset(_MSVC_WINE_SDK_DIRS)
unset(_MSVC_WINE_MSVC_DIR)
unset(_MSVC_WINE_SDK_DIR)
unset(_MSVC_WINE_SYSTEM_INCLUDES)
