
# Create an ld.so.conf file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/opencog.conf
	"# Location of OpenCog C/C++ libraries\n"
	"${CMAKE_INSTALL_PREFIX}/lib${LIB_DIR_SUFFIX}/opencog\n")

# Well, this isn't portable, but it works for Linux
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/opencog.conf DESTINATION "/etc/ld.so.conf.d")
