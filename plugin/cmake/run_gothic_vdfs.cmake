foreach(_required_variable
        VDF_EXECUTABLE
        VDF_SCRIPT
        VDF_OUTPUT
        VDF_WORKING_DIRECTORY)
    if(NOT DEFINED ${_required_variable}
       OR "${${_required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${_required_variable} was not provided.")
    endif()
endforeach()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    find_program(WINE_EXECUTABLE wine REQUIRED)
    get_filename_component(_vdf_script_name "${VDF_SCRIPT}" NAME)
    set(_vdf_command "${WINE_EXECUTABLE}" "${VDF_EXECUTABLE}")
    set(_vdf_script_argument ".\\${_vdf_script_name}")
else()
    file(TO_NATIVE_PATH "${VDF_SCRIPT}" _vdf_script_argument)
    set(_vdf_command "${VDF_EXECUTABLE}")
endif()

# GothicVDFS can return success after falling back to its GUI. Remove any old
# package first and verify that this invocation really produced a new one.
file(REMOVE "${VDF_OUTPUT}")

execute_process(
    COMMAND ${_vdf_command} /B "${_vdf_script_argument}"
    WORKING_DIRECTORY "${VDF_WORKING_DIRECTORY}"
    RESULT_VARIABLE _vdf_result
    OUTPUT_VARIABLE _vdf_stdout
    ERROR_VARIABLE _vdf_stderr
)

if(NOT _vdf_result EQUAL 0)
    message(FATAL_ERROR
        "GothicVDFS failed with exit code ${_vdf_result}.\n"
        "${_vdf_stdout}${_vdf_stderr}"
    )
endif()

if(NOT EXISTS "${VDF_OUTPUT}")
    message(FATAL_ERROR
        "GothicVDFS returned success but did not create '${VDF_OUTPUT}'.\n"
        "${_vdf_stdout}${_vdf_stderr}"
    )
endif()
