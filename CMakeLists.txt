# CMake Project for IRO - Image Reconstitution Orchestrator
# Written by @thatcosmonaut
cmake_minimum_required(VERSION 3.5)
project(IRO C)
find_package(SDL3 REQUIRED)

# Options
option(BUILD_SHARED_LIBS "Build shared library" ON)

# Version
SET(LIB_MAJOR_VERSION "1")
SET(LIB_MINOR_VERSION "0")
SET(LIB_REVISION "0")
SET(LIB_VERSION "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_REVISION}")

# Build Type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	# By default, we use Release
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE "Release" CACHE
		STRING "Choose the type of build." FORCE
	)

	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
		STRINGS "Debug" "Release" "RelWithDebInfo"
	)
endif()

# Platform Flags
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
	set(LOBJC "objc")
elseif(WIN32)
	# "IRO.dll", not "libIRO.dll"
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# Source lists
add_library(IRO
	# Public Headers
	include/IRO.h
	# Source Files
	src/IRO.c
	src/stb_image.h
	src/stb_image_write.h
	src/miniz.h
)

# Build flags
if(NOT MSVC)
	set_property(TARGET IRO PROPERTY COMPILE_FLAGS "-std=gnu99 -Wall -Wno-strict-aliasing -pedantic")
endif()

# IRO folders as includes, for other targets to consume
target_include_directories(IRO PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# MinGW builds should statically link libgcc
if(MINGW)
	target_link_libraries(IRO PRIVATE -static-libgcc)
endif()

# Soname
set_target_properties(IRO PROPERTIES OUTPUT_NAME "IRO"
	VERSION ${LIB_VERSION}
	SOVERSION ${LIB_MAJOR_VERSION}
)

target_link_libraries(IRO
    SDL3::SDL3
    SDL3::Headers
)
