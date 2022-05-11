### CMake file for camv4l2 
###
###     Author : glaconbleu@codices.org
###
###############################################################################

cmake_minimum_required (VERSION 3.9)

project (camv4l2)
set (camv4l2_VERSION_MAJOR 0)
set (camv4l2_VERSION_MINOR 1)
set (camv4l2_VERSION_PATCH 1)
set (camv4l2_VERSION
	${camv4l2_VERSION_MAJOR}.${camv4l2_VERSION_MINOR}.${camv4l2_VERSION_PATCH} )

set( CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /lib /usr/lib /usr/local/lib )
set( CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} /usr/include /usr/local/include )
set( CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake" ) 

set ( INSTALL_BIN_PATH bin )
set ( INSTALL_LIB_PATH lib )
set ( INSTALL_AR_PATH  lib )
set ( INSTALL_HEADERS_PATH include )
set ( INSTALL_DATASETS_PATH "${CMAKE_INSTALL_PREFIX}/share/camv4l2/datasets" )

### Options ###

option(BUILD_TEST   "Build camv4l2 tests" ON)

### Config File

configure_file ( "${PROJECT_SOURCE_DIR}/config.h.in"
		         "${PROJECT_SOURCE_DIR}/src/config.h" )

### Set include directories 
        
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/include/"
	                 "${CMAKE_CURRENT_BINARY_DIR}" 
                      )

### File lists

file( GLOB camv4l2_src "src/*.c" )
file( GLOB camv4l2_hdr "src/*.h" )

### Targets

add_library( "cam_v4l2" SHARED ${camv4l2_src} ${camv4l2_hdr} ) 

set_target_properties( cam_v4l2 PROPERTIES VERSION ${camv4l2_hwcam_VERSION}
    SOVERSION ${camv4l2_hwcam_VERSION_MAJOR})

### Linking

target_link_libraries( "cam_v4l2" "m" )

### Install 

install( TARGETS "cam_v4l2"
    LIBRARY DESTINATION "${INSTALL_PLUGINS_PATH}" ) 


### tests

if (BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
