# One mashup's Union plugin, called from the CMakeLists.txt in its mashups/<game>/plugin folder after
# its project(): ${PROJECT_NAME}.dll from the .ipp and .cpp files in src/ there and the template's
# DllMain.cpp, packed with UnionAPI.dll into ${PROJECT_NAME}.vdf. It is union-plugin-template's own
# build, with every name and folder made per plugin so that several plugins build side by side.
set(UNION_PLUGIN_TEMPLATE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

function(add_union_plugin)
	set(name ${PROJECT_NAME})

	file(GLOB_RECURSE IPP_FILES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/src/*.ipp"
	)

	file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
	)

	generate_ipp_translation_units(
		INPUT ${IPP_FILES}
		OUTPUT IPP_TRANSLATION_UNITS
	)

	add_library(${name} SHARED)

	target_sources(${name}
		PRIVATE
			"${UNION_PLUGIN_TEMPLATE_DIR}/src/DllMain.cpp"
			${SRC}
			${IPP_TRANSLATION_UNITS}
	)

	target_precompile_headers(${name}
		PRIVATE
			<Union/Hook.h>
			<ZenGin/zGothicAPI.h>
	)

	target_include_directories(${name}
		INTERFACE
			"${CMAKE_CURRENT_SOURCE_DIR}/include"
		PRIVATE
			"${CMAKE_CURRENT_SOURCE_DIR}/src"
	)

	target_include_directories(${name}
		BEFORE PRIVATE
			"${UNION_PLUGIN_TEMPLATE_DIR}/userapi"
	)

	target_link_libraries(${name}
		PRIVATE
			union
	)

	target_add_versioninfo(${name}
		VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}"
		COMPANY_NAME "Gothic Community"
		PRODUCT_NAME "${name}"
	)

	if (GENERATE_VDF)
		set(VDF_DIR "${UNION_PLUGIN_TEMPLATE_DIR}/vdf/")
		set(VDF_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/vdf/")
		set(VDF_BUILD_AUTORUN_DIR "${VDF_BUILD_DIR}System/Autorun/")

		file(COPY "${VDF_DIR}" DESTINATION "${VDF_BUILD_DIR}" FOLLOW_SYMLINK_CHAIN)

		set(VDF_BUILD_VM_FILE "${VDF_BUILD_DIR}build.vm")
		configure_file("${VDF_DIR}build.vm" "${VDF_BUILD_VM_FILE}")
		set(VDF_OUTPUT_FILE "${VDF_BUILD_DIR}${name}.vdf")

		if (GENERATE_UNION_API_DLL)
			add_custom_target(copy_union_api_dll_${name}
				ALL
					COMMAND ${CMAKE_COMMAND} -E make_directory ${VDF_BUILD_AUTORUN_DIR}
					COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:union_api_dll> ${VDF_BUILD_AUTORUN_DIR}
					COMMAND ${CMAKE_COMMAND} -E echo "[Post Build] $<TARGET_FILE_NAME:union_api_dll> copied to: ${VDF_BUILD_AUTORUN_DIR}"
				DEPENDS union_api_dll
			)
		endif()

		add_custom_command(TARGET ${name}
			POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory ${VDF_BUILD_AUTORUN_DIR}
				COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${name}> ${VDF_BUILD_AUTORUN_DIR}
				COMMAND ${CMAKE_COMMAND} -E echo "[Post Build] $<TARGET_FILE_NAME:${name}> copied to: ${VDF_BUILD_AUTORUN_DIR}")

		add_custom_target(build_vdf_${name}
			ALL
				COMMAND ${CMAKE_COMMAND} -E echo "[VDF] Building ${name}.vdf ..."
				COMMAND ${CMAKE_COMMAND}
					"-DVDF_EXECUTABLE=${VDF_BUILD_DIR}/GothicVDFS.exe"
					"-DVDF_SCRIPT=${VDF_BUILD_VM_FILE}"
					"-DVDF_OUTPUT=${VDF_OUTPUT_FILE}"
					"-DVDF_WORKING_DIRECTORY=${VDF_BUILD_DIR}"
					-P "${UNION_PLUGIN_TEMPLATE_DIR}/cmake/run_gothic_vdfs.cmake"
				COMMAND ${CMAKE_COMMAND} -E echo "[VDF] ${name}.vdf build successfully!"
			WORKING_DIRECTORY "${VDF_BUILD_DIR}"
			DEPENDS ${name}
		)

		if(GENERATE_UNION_API_DLL)
			add_dependencies(build_vdf_${name} copy_union_api_dll_${name})
		endif()
	endif()

	if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_EXPORT_COMPILE_COMMANDS)
		configure_msvc_wine_intellisense(
			TARGET ${name}
			INPUT ${IPP_FILES}
		)
	endif()
endfunction()
