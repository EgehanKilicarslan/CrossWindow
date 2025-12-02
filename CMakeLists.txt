cmake_minimum_required(VERSION 3.15)
project(CrossWindow VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(CROSSWINDOW_BUILD_SHARED "Build CrossWindow as a shared library" OFF)
option(CROSSWINDOW_BUILD_TESTS "Build CrossWindow tests" ON)
option(CROSSWINDOW_BUILD_EXAMPLES "Build CrossWindow examples" ON)

# Common sources
set(CROSSWINDOW_SOURCES
    src/WindowManager.cpp
)

# Platform-specific sources and libraries
if(WIN32)
    list(APPEND CROSSWINDOW_SOURCES
        src/platform/windows/WindowManagerWindows.cpp
    )
    set(CROSSWINDOW_PLATFORM_LIBS dwmapi)
elseif(APPLE)
    list(APPEND CROSSWINDOW_SOURCES
        src/platform/macos/WindowManagerMacOS.mm
    )
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(APPSERVICES_LIBRARY ApplicationServices REQUIRED)
    set(CROSSWINDOW_PLATFORM_LIBS ${COCOA_LIBRARY} ${APPSERVICES_LIBRARY})
elseif(UNIX)
    list(APPEND CROSSWINDOW_SOURCES
        src/platform/linux/WindowManagerLinux.cpp
    )
    find_package(X11 REQUIRED)
    set(CROSSWINDOW_PLATFORM_LIBS ${X11_LIBRARIES})
    set(CROSSWINDOW_PLATFORM_INCLUDES ${X11_INCLUDE_DIR})
endif()

# Create library
if(CROSSWINDOW_BUILD_SHARED)
    add_library(CrossWindow SHARED ${CROSSWINDOW_SOURCES})
    target_compile_definitions(CrossWindow PRIVATE CROSSWINDOW_BUILD_SHARED)
else()
    add_library(CrossWindow STATIC ${CROSSWINDOW_SOURCES})
endif()

# Include directories
target_include_directories(CrossWindow 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CROSSWINDOW_PLATFORM_INCLUDES}
)

# Link libraries
target_link_libraries(CrossWindow PRIVATE ${CROSSWINDOW_PLATFORM_LIBS})

# Set properties
set_target_properties(CrossWindow PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER include/CrossWindow.h
)

# Build examples
if(CROSSWINDOW_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests
if(CROSSWINDOW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Install
include(GNUInstallDirs)
install(TARGETS CrossWindow
    EXPORT CrossWindowTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT CrossWindowTargets
    FILE CrossWindowTargets.cmake
    NAMESPACE CrossWindow::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CrossWindow
)

# Package configuration
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CrossWindowConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CrossWindowConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CrossWindow
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/CrossWindowConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CrossWindowConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/CrossWindowConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CrossWindow
)
