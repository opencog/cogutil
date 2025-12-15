
# Create an ld.so.conf file
# This is required so that everyone can find the opencog shared libs.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/opencog.conf
	"# Location of OpenCog C/C++ libraries\n"
	"${CMAKE_INSTALL_PREFIX}/lib${LIB_DIR_SUFFIX}/opencog\n")

# Well, this isn't portable, but it works for Debian/Ubuntu
# Skip on Guix/Nix builds where this isn't needed
option(SKIP_LDCONF "Skip installing ld.so.conf.d file" OFF)
if(NOT SKIP_LDCONF)
	set(CMAKE_CONF_PREFIX "/etc" CACHE PATH "Directory for configuration files")
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/opencog.conf DESTINATION "${CMAKE_CONF_PREFIX}/ld.so.conf.d")
endif(NOT SKIP_LDCONF)
