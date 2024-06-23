
# Create an ld.so.conf file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/opencog.conf
	"# Location of OpenCog C/C++ libraries\n"
	"${CMAKE_INSTALL_PREFIX}/lib${LIB_DIR_SUFFIX}/opencog\n")

# Well, this isn't portable, but it works for Linux
set(CMAKE_CONF_PREFIX "/etc" CACHE PATH "Directory for configuration files")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/opencog.conf DESTINATION "${CMAKE_CONF_PREFIX}/ld.so.conf.d")
