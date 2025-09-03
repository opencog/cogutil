
# A compatible pkgconfig
configure_package_config_file(cogutil.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/cogutil.pc
	INSTALL_DESTINATION lib/pkgconfig
	PATH_VARS CMAKE_INSTALL_PREFIX
)

INSTALL(FILES
	${CMAKE_CURRENT_BINARY_DIR}/cogutil.pc
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/
)
