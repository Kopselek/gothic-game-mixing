# The Witcher 3 mashup's Union plugin, Witcher.vdf. plugin\build.bat builds it with the other
# mashups' plugins (plugin/CMakeLists.txt); all of its code is in src/Plugin.ipp.
project(Witcher
	DESCRIPTION "The Witcher 3 in Gothic 1: Geralt, his controls, combat, signs and HUD"
	VERSION ${PROJECT_VERSION}
)

add_union_plugin()
