# LS2X cmake
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prevent in-tree build.
if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
	message(FATAL_ERROR "Prevented in-tree build!")
endif()

project(ls2xlib LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Find Lua(JIT)
find_package(LuaJIT)
if (NOT LUAJIT_FOUND)
	find_package(Lua 5.1 EXACT REQUIRED)
	set(DESIRED_LUA_INCLUDE_DIR ${LUA_INCLUDE_DIR})
	set(DESIRED_LUA_LIBRARY ${LUA_LIBRARIES})
else()
	set(DESIRED_LUA_INCLUDE_DIR ${LUAJIT_INCLUDE_DIR})
	set(DESIRED_LUA_LIBRARY ${LUAJIT_LIBRARY})
endif()

# Check libav include, only include.
if (NOT LIBAV_INCLUDE_DIR)
	find_path(LIBAV_INCLUDE_DIR
		NAMES libavcodec/avcodec.h
		PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/local/include
		/usr/include
		/sw/include # Fink
		/opt/local/include # DarwinPorts
		/opt/csw/include # Blastwave
		/opt/include
		/usr/freeware/include
	)
endif()

if (LIBAV_INCLUDE_DIR)
	message(STATUS "libav include path: ${LIBAV_INCLUDE_DIR}")
else()
	message(WARNING "libav not found!")
endif()

# Source Files
set(LS2X_SOURCE_FILES src/main.cpp)
set(LS2X_DEFINES "")
set(LS2X_INCLUDE ${DESIRED_LUA_INCLUDE_DIR})

# Audiomix routine, always supported
list(APPEND LS2X_SOURCE_FILES src/audiomix.cpp)

# KissFFT
if (NOT LS2X_DISABLE_FFT)
	list(APPEND LS2X_SOURCE_FILES src/fft.cpp src/kissfft/kiss_fft.c)
	list(APPEND LS2X_DEFINES LS2X_USE_KISSFFT)
endif()

# libav
if (LIBAV_INCLUDE_DIR)
	# Dynwrap first
	if (WIN32)
		list(APPEND LS2X_SOURCE_FILES src/dynwrap/dynwrap_win.cpp)
	else()
		list(APPEND LS2X_SOURCE_FILES src/dynwrap/dynwrap_unix.cpp)
	endif()

	list(APPEND LS2X_SOURCE_FILES src/libav.cpp)
	list(APPEND LS2X_DEFINES LS2X_USE_LIBAV)
	list(APPEND LS2X_INCLUDE ${LIBAV_INCLUDE_DIR})

	# If LOVE include dir specified then it's embedded in LOVE.
	if (LOVE_INCLUDE_DIR)
		list(APPEND LS2X_SOURCE_FILES
			src/lvep/FFMpegStream.cpp
			src/lvep/LFSIOContext.cpp
			src/lvep/lvep.cpp
			src/lvep/LVEPDecoder.cpp
			src/lvep/LVEPVideoStream.cpp
		)
		list(APPEND LS2X_DEFINES LS2X_EMBEDDED_IN_LOVE)
		list(APPEND LS2X_INCLUDE ${LOVE_INCLUDE_DIR})
	endif()
endif()

# Library
if (LOVE_INCLUDE_DIR)
	# static library
	add_library(ls2xlib STATIC ${LS2X_SOURCE_FILES})
	message(STATUS "LOVE include (static lib mode): ${LOVE_INCLUDE_DIR}")
else()
	# shared library
	add_library(ls2xlib SHARED ${LS2X_SOURCE_FILES})
	message(STATUS "LOVE not found (shared lib mode)")
endif()

target_include_directories(ls2xlib PRIVATE ${LS2X_INCLUDE})
target_compile_definitions(ls2xlib PRIVATE ${LS2X_DEFINES})

# Installdir
install(TARGETS ls2xlib DESTINATION lib)
