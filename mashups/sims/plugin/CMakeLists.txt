# The Sims 4 mashup's Union plugin, Sims.vdf. plugin\build.bat builds it with the other mashups'
# plugins (plugin/CMakeLists.txt); all of its code is in src/Plugin.ipp.
project(Sims
	DESCRIPTION "The Sims 4's camera, pie menu and autonomy for the hero of Gothic 1 and Gothic 2"
	VERSION ${PROJECT_VERSION}
)

add_union_plugin()
