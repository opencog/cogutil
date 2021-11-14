
# Create an ld.so.conf file
file(WRITE opencog.conf
	"# Location of OpenCog C/C++ libraries\n"
	"${CMAKE_INSTALL_PREFIX}/lib${LIB_DIR_SUFFIX}/opencog\n")

# Well, this isn't portable, but it works for Linux
install(FILES opencog.conf DESTINATION "/etc/ld.so.conf.d")
