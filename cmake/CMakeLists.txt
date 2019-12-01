
# install Find* cmake files to the DATADIR
INSTALL(FILES
        AddCxxtest.cmake        
        FindBFD.cmake
        FindCxxtest.cmake
        FindCython.cmake
        FindGTK3.cmake
        FindGuile.cmake
        FindHyperTable.cmake
        FindIberty.cmake
        FindLibGearman.cmake
        FindMsgPack.cmake
        FindPthreads.cmake
        FindStack.cmake
        FindSTLPort.cmake
        FindVALGRIND.cmake
        FindZMQ.cmake
        FindSIGAR.cmake
        FindPython3Interp.cmake
        FindPGSQL.cmake
        FindTBB.cmake
        FindBFD.cmake
        FindGHC.cmake
        FindLinkGrammar.cmake
        FindMOSES.cmake
        FindOctomap.cmake
        FindUUID.cmake
        Findcpprest.cmake
        Summary.cmake
        OpenCogGccOptions.cmake
        OpenCogFindPython.cmake
        COPYING-CMAKE-SCRIPTS
        DESTINATION
        ${DATADIR}/cmake/)

set(INCLUDE_INSTALL_DIR include/ )
set(LIB_INSTALL_DIR lib/ )

set(COGUTIL_CMAKE_DIR lib/cmake/CogUtil)

include(CMakePackageConfigHelpers)
configure_package_config_file(CogUtilConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CogUtilConfig.cmake
                              INSTALL_DESTINATION COGUTIL_CMAKE_DIR
                              PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/CogUtilConfigVersion.cmake"
  VERSION ${SEMANTIC_VERSION}
  COMPATIBILITY SameMajorVersion
)

INSTALL(FILES
       ${CMAKE_CURRENT_BINARY_DIR}/CogUtilConfigVersion.cmake
       ${CMAKE_CURRENT_BINARY_DIR}/CogUtilConfig.cmake
       DESTINATION
       ${COGUTIL_CMAKE_DIR})

