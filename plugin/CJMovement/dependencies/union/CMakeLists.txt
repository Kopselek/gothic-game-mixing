add_subdirectory(union-api)
add_subdirectory(gothic-api)

# union
if(GENERATE_UNION_API_DLL)
	set(UNION_API union_api_dll)
else()
	set(UNION_API union_api_lib)
endif()

add_library(union INTERFACE)

target_link_libraries(union
	INTERFACE
		gothic_api
		${UNION_API}
)