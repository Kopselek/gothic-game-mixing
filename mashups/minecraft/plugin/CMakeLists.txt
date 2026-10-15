# The Minecraft mashup's Union plugin, Minecraft.vdf. plugin\build.bat builds it with the other
# mashups' plugins (plugin/CMakeLists.txt); all of its code is in src/Plugin.ipp.
project(Minecraft
	DESCRIPTION "Minecraft's camera for Steve in Gothic 1 and Gothic 2"
	VERSION ${PROJECT_VERSION}
)

add_union_plugin()
