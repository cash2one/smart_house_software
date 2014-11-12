PROJECT(Qrobot2)
cmake_minimum_required( VERSION 2.8 )

# Setting CXX flags
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++0x")

#SET(CMAKE_BUILD_TYPE "DEBUG")
SET(CMAKE_BUILD_TYPE "RELEASE")
#message(STATUS ${CMAKE_BUILD_TYPE})

SET(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/lib")

################################################################
# find depended package

# PkgConfig.
message(STATUS "Check for PkgConfig module")
find_package(PkgConfig 0.26 REQUIRED)
if(PKG_CONFIG_FOUND)
  message(STATUS "-- found: " ${PKG_CONFIG_EXECUTABLE})
else()
  message(FATAL_ERROR "-- not found")
endif()

#lib usb
set(LIBUSB_LIBRARIES usb-1.0 )

# Add include directories
include_directories(
	${PROJECT_SOURCE_DIR}/include
)

# Add link directories
link_directories(
	${PROJECT_SOURCE_DIR}/lib
)

add_subdirectory(src)


