# The GTA San Andreas mashup's Union plugin, CJMovement.vdf. plugin\build.bat builds it with the
# other mashups' plugins (plugin/CMakeLists.txt); all of its code is in src/Plugin.ipp.
project(CJMovement
	DESCRIPTION "GTA San Andreas movement and camera for CJ in Gothic 1"
	VERSION ${PROJECT_VERSION}
)

add_union_plugin()
