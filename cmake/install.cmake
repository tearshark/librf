
# Configuration
# Used by cmake to find_package(xxx)
set(PROJECT_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")

# Add definitions for targets
# Values:
#   * Debug: -Dxxx_DEBUG=1
#   * Release: -Dxxx_DEBUG=0
#   * other: -Dxxx_DEBUG=0
target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}_DEBUG=$<CONFIG:Debug>)

# Include module with function 'write_basic_package_version_file'
# Configure 'xxxConfigVersion.cmake'
include(CMakePackageConfigHelpers)
write_basic_package_version_file(${VERSION_CONFIG} VERSION ${PACKAGE_VERSION}
        COMPATIBILITY SameMajorVersion)

# Configure 'xxxConfig.cmake'
configure_package_config_file(Config.cmake.in ${PROJECT_CONFIG}
        INSTALL_DESTINATION cmake/${PROJECT_NAME})

# Targets:
#   * <prefix>/lib/Windows/x64-Debug/xxx.lib
#   * <prefix>/bin/Windows/x64-Debug/xxx.dll
set(INSTALL_TARGET_PREFIX "${CMAKE_CXX_PLATFORM_ID}/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Debug
        LIBRARY DESTINATION "lib/${INSTALL_TARGET_PREFIX}-Debug"
        ARCHIVE DESTINATION "lib/${INSTALL_TARGET_PREFIX}-Debug"
        RUNTIME DESTINATION "bin/${INSTALL_TARGET_PREFIX}-Debug"
        )
#   * <prefix>/lib/Windows/x64-Release/xxx.lib
#   * <prefix>/bin/Windows/x64-Release/xxx.dll
install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Release RelWithDebInfo MinSizeRel
        LIBRARY DESTINATION "lib/${INSTALL_TARGET_PREFIX}-Release"
        ARCHIVE DESTINATION "lib/${INSTALL_TARGET_PREFIX}-Release"
        RUNTIME DESTINATION "bin/${INSTALL_TARGET_PREFIX}-Release"
        )

# Config
#   * <prefix>/cmake/xxxConfig.cmake
#   * <prefix>/cmake/xxxConfigVersion.cmake
install(FILES ${PROJECT_CONFIG} DESTINATION cmake)
