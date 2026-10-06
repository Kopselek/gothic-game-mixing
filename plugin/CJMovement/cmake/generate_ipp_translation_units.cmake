include(get_targeted_gothic_engines)

function(generate_ipp_translation_units)
    cmake_parse_arguments(ARG "" "OUTPUT" "INPUT" ${ARGN})

	if (NOT DEFINED ARG_INPUT)
		message(SEND_ERROR "generate_ipp_translation_units function requires INPUT argument")
		return()
	endif()

    if (NOT DEFINED ARG_OUTPUT)
		message(SEND_ERROR "generate_ipp_translation_units function requires OUTPUT argument")
		return()
	endif()

    set(ipp_translation_units "")

    get_targeted_gothic_engines(GothicEngines)

    # Generate helper variable that points to dir where .cpp translation units resides
    set(GENERATED_IPP_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/generated_ipp")

    # Delete "generated_ipp" folder first
    file(REMOVE_RECURSE "${GENERATED_IPP_DIR}")

    while(GothicEngines)
        list(POP_FRONT GothicEngines PlatformSuffix GothicNamespace)

        string(TOLOWER "${PlatformSuffix}" platform_suffix_lowercase)

        foreach(ipp ${ARG_INPUT})
            # Extract relative src filepath
            string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/src" "" filepath "${ipp}")

            # Extract relative_dir & filename (without extensions)
            cmake_path(GET filepath PARENT_PATH relative_dir)
            cmake_path(GET filepath STEM filename_no_ext)

            # Generate unique platform file name, e.g (Plugin_g2a.cpp)
            if(relative_dir STREQUAL "/")
                set(platform_filepath_no_ext "/${filename_no_ext}_${platform_suffix_lowercase}")
            else()
                set(platform_filepath_no_ext "${relative_dir}/${filename_no_ext}_${platform_suffix_lowercase}")
            endif()

            set(out "${GENERATED_IPP_DIR}/${platform_filepath_no_ext}.cpp")

            # Tell configure_file what to substitute (convert absolute path to relative one)
            file(RELATIVE_PATH IppFile "${CMAKE_CURRENT_SOURCE_DIR}/src/" "${ipp}")

            # Generate the .cpp file
            configure_file(
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/IppFileTemplate.in
                    ${out}
                    @ONLY
            )

            list(APPEND ipp_translation_units ${out})
        endforeach()
    endwhile()

    set(${ARG_OUTPUT} ${ipp_translation_units} PARENT_SCOPE)
endfunction()