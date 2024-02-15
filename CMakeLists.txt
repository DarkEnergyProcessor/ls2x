# LS2X cmake
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prevent in-tree build.
if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
	message(FATAL_ERROR "Prevented in-tree build!")
endif()

project(ls2xlib LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (LOVE_SRC_COMMON)
	# LOVE exist and this is added via add_subdirectory
	get_directory_property(LOVE_INCLUDE_DIR PARENT_DIRECTORY)
	set(LOVE_INCLUDE_DIR ${LOVE_INCLUDE_DIR}/src)
endif()

function(print_option name)
	message(STATUS "Option ${name} = ${${name}}")
endfunction()

option(LS2X_NO_LIBAV "Disable libav" OFF)
option(LIBAV_INCLUDE_DIR "FFmpeg include directories" "")
option(LS2X_DISABLE_FFT "Disable FFT" OFF)
option(LS2X_OPENMP "Use OpenMP for AudioMix when possible" ON)

print_option(LS2X_NO_LIBAV)
print_option(LIBAV_INCLUDE_DIR)
print_option(LS2X_DISABLE_FFT)
print_option(LS2X_OPENMP)

set(LS2X_EXTRA_DEPS "")

if (MEGA)
	# Dependencies brought along with build system
	if (LOVE_JIT)
		set(DESIRED_LUA_INCLUDE_DIR ${MEGA_LUAJIT_INCLUDE})
		set(DESIRED_LUA_LIBRARY ${MEGA_LUAJIT_LIB})
		set(LS2X_EXTRA_DEPS luajit)
	else()
		message(ERROR "Cannot use LS2X without LuaJIT")
	endif()
else()
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
endif()

# Check FFmpeg libav include, only include.
if (NOT LS2X_NO_LIBAV)
	find_path(LS2X_LIBAV_INCLUDE_DIR
		NAMES libavcodec/avcodec.h
		PATHS
		${LIBAV_INCLUDE_DIR}
		$ENV{LIBAV_INCLUDE_DIR}
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

	if (LS2X_LIBAV_INCLUDE_DIR)
		message(STATUS "FFmpeg libav include path: ${LS2X_LIBAV_INCLUDE_DIR}")
	else()
		message(WARNING "FFmpeg libav not found!")
	endif()
else()
	message(STATUS "FFmpeg libav support is disabled")
endif()

# Source Files
set(LS2X_SOURCE_FILES src/main.cpp)
set(LS2X_DEFINES "")
set(LS2X_INCLUDE ${DESIRED_LUA_INCLUDE_DIR})

if (WIN32)
	list(APPEND LS2X_DEFINES LUA_BUILD_AS_DLL)
endif()

if (MSVC)
	# ...
	list(APPEND LS2X_DEFINES _CRT_SECURE_NO_WARNINGS)
endif()

# Audiomix routine, always supported
list(APPEND LS2X_SOURCE_FILES src/audiomix.cpp)

# KissFFT
if (NOT LS2X_DISABLE_FFT)
	list(APPEND LS2X_SOURCE_FILES src/fft.cpp src/kissfft/kiss_fft.c src/kissfft/kiss_fftr.c)
	list(APPEND LS2X_DEFINES LS2X_USE_KISSFFT kiss_fft_scalar=double)
endif()

# libav
if (LS2X_LIBAV_INCLUDE_DIR)
	# Dynwrap first
	if (WIN32)
		list(APPEND LS2X_SOURCE_FILES src/dynwrap/dynwrap_win.cpp)
	else()
		list(APPEND LS2X_SOURCE_FILES src/dynwrap/dynwrap_unix.cpp)
	endif()

	list(APPEND LS2X_SOURCE_FILES src/libav.cpp)
	list(APPEND LS2X_DEFINES LS2X_USE_LIBAV)
	list(APPEND LS2X_INCLUDE ${LS2X_LIBAV_INCLUDE_DIR})

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
		list(APPEND LS2X_INCLUDE "${LOVE_INCLUDE_DIR}" "${LOVE_INCLUDE_DIR}/modules")
	endif()
endif()

message(STATUS "INCLUDE LS2X ${LS2X_INCLUDE}")
# Library
if (LOVE_INCLUDE_DIR)
	# static library
	add_library(ls2xlib STATIC ${LS2X_SOURCE_FILES})
	message(STATUS "LOVE include (static lib mode): ${LOVE_INCLUDE_DIR}")
else()
	# shared library
	add_library(ls2xlib MODULE ${LS2X_SOURCE_FILES})
	message(STATUS "LOVE not found (shared lib mode)")
	install(TARGETS ls2xlib DESTINATION lib)
	target_link_libraries(ls2xlib ${DESIRED_LUA_LIBRARY})
endif()

if(LS2X_EXTRA_DEPS)
	add_dependencies(ls2xlib ${LS2X_EXTRA_DEPS})
endif()
set_target_properties(ls2xlib PROPERTIES PREFIX "")
target_include_directories(ls2xlib PRIVATE ${LS2X_INCLUDE})
target_compile_definitions(ls2xlib PRIVATE ${LS2X_DEFINES})

# CMake must be at least 3.9.6 for OpenMP detection
if (LS2X_OPENMP)
	if (${CMAKE_VERSION} VERSION_LESS "3.9.6")
		message(WARNING "CMake version is too old for OpenMP detection (v3.9.6 or later is needed). \
	Compiling without OpenMP support. Performance may be slower slightly!")
	else ()
		find_package(OpenMP)
		if (OpenMP_CXX_FOUND)
			target_link_libraries(ls2xlib OpenMP::OpenMP_CXX)
		else ()
			message(WARNING "Compiling without OpenMP support. Performance may be slower slightly!")
		endif ()
	endif ()
endif ()
