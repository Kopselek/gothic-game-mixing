include(get_targeted_gothic_engines)

function(configure_msvc_wine_intellisense)
    cmake_parse_arguments(ARG "" "TARGET" "INPUT" ${ARGN})

    if (NOT DEFINED ARG_TARGET)
        message(SEND_ERROR "configure_msvc_wine_intellisense function requires TARGET argument")
        return()
    endif()

    if (NOT DEFINED ARG_INPUT)
        message(SEND_ERROR "configure_msvc_wine_intellisense function requires INPUT argument")
        return()
    endif()

    if(ARG_INPUT)
        set(_editor_context_header "${CMAKE_BINARY_DIR}/intellisense/IntelliSenseContext.h")
        
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/intellisense")
        file(GENERATE
            OUTPUT "${_editor_context_header}"
            CONTENT [=[#pragma once
#include <Union/Hook.h>
#include <ZenGin/zGothicAPI.h>
]=]
        )

        get_targeted_gothic_engines(_editor_engines)

        while(_editor_engines)
            list(POP_FRONT _editor_engines _editor_suffix _editor_namespace)

            string(TOLOWER "${_editor_suffix}" _editor_suffix_lowercase)
            set(_editor_target "${ARG_TARGET}_intellisense_${_editor_suffix_lowercase}")

            add_library("${_editor_target}" OBJECT EXCLUDE_FROM_ALL)

            target_sources(${_editor_target} PRIVATE ${ARG_INPUT})
            set_source_files_properties(${ARG_INPUT}
                PROPERTIES LANGUAGE CXX
            )

            get_target_property(_target_private_include_dirs ${CMAKE_PROJECT_NAME} INCLUDE_DIRECTORIES)
            target_include_directories("${_editor_target}" PRIVATE ${_target_private_include_dirs})

            target_compile_definitions("${_editor_target}"
                PRIVATE
                    "GOTHIC_NAMESPACE=${_editor_namespace}"
                    "ENGINE=ENGINE_${_editor_suffix}"
            )

            target_compile_options("${_editor_target}"
                PRIVATE
                    "/FI${_editor_context_header}"
            )

            get_target_property(_target_private_link_libs ${CMAKE_PROJECT_NAME} LINK_LIBRARIES)
            target_link_libraries("${_editor_target}" PRIVATE ${_target_private_link_libs})
        endwhile()
    endif()

    set(_editor_compile_commands "${CMAKE_SOURCE_DIR}/compile_commands.json")

    if(IS_SYMLINK "${_editor_compile_commands}")
        file(REMOVE "${_editor_compile_commands}")
    elseif(EXISTS "${_editor_compile_commands}")
        message(FATAL_ERROR "${_editor_compile_commands} exists and is not a symlink.")
    endif()

    file(CREATE_LINK
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${_editor_compile_commands}"
        SYMBOLIC
        RESULT _editor_compile_commands_result
    )

    if(NOT _editor_compile_commands_result STREQUAL "0")
        message(FATAL_ERROR "Could not select the active editor compilation database: ${_editor_compile_commands_result}")
    endif()
endfunction()
