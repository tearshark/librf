cmake_minimum_required(VERSION 3.10)
project(librf VERSION 3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LIBRF_COMPILER_SETTING )
if(WIN32)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(LIBRF_COMPILER_SETTING "msvc")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		set(LIBRF_COMPILER_SETTING "msvc")
	else()
		set(LIBRF_COMPILER_SETTING "gcc")
	endif()
elseif(APPLE)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
		set(LIBRF_COMPILER_SETTING "clang")
	else()
		set(LIBRF_COMPILER_SETTING "gcc")
	endif()
elseif(UNIX)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(LIBRF_COMPILER_SETTING "clang")
	else()
		set(LIBRF_COMPILER_SETTING "gcc")
	endif()
else()
	set(LIBRF_COMPILER_SETTING "clang")
endif()

message(STATUS "LIBRF_COMPILER_SETTING=${LIBRF_COMPILER_SETTING}")

if(${LIBRF_COMPILER_SETTING} STREQUAL "msvc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /await /EHsc")
elseif ("${LIBRF_COMPILER_SETTING}" STREQUAL "clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fcoroutines-ts -stdlib=libc++")
elseif ("${LIBRF_COMPILER_SETTING}" STREQUAL "gcc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fcoroutines -fconcepts-diagnostics-depth=8")
endif()


if(${LIBRF_COMPILER_SETTING} STREQUAL "msvc")
	option(OPT_INLINE_STATE "Enable Inline state" ON)
elseif(${LIBRF_COMPILER_SETTING} STREQUAL "clang")
	option(OPT_INLINE_STATE "Enable Inline state" ON)
elseif(${LIBRF_COMPILER_SETTING} STREQUAL "gcc")
	option(OPT_INLINE_STATE "Enable Inline state" OFF)
else()
	option(OPT_INLINE_STATE "Enable Inline state" OFF)
endif()

option(OPT_DEBUG_COUNTER "Debug objects count" OFF)
option(OPT_KEEP_REAL_SIZE "Keep real size in queue" OFF)
option(OPT_DISABLE_MULT_THREAD "Disable multi-threaded scheduler" OFF)
option(OPT_USE_MIMALLOC "Use mimalloc" OFF)

if (UNIX)
	if(OPT_USE_MIMALLOC)
		find_package(mimalloc 1.4 REQUIRED)
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pthread")
endif()

if(${LIBRF_COMPILER_SETTING} STREQUAL "msvc")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "/W3 /WX /MP /GS- /Gm- /Ox /Ob2 /Oy /Oi /Os /GT /EHsc /Zc:inline")
	set(CMAKE_CXX_FLAGS_RELEASE    "/W3 /WX /MP /GS- /Gm- /Ox /Ob2 /Oy /Oi /Os /GT /EHsc /Zc:inline")
elseif()
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
	endif()
endif()


message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")

#set(RESUMEF_USE_CUSTOM_SPINLOCK "std::mutex")

if(OPT_INLINE_STATE)
	set(RESUMEF_INLINE_STATE 1)
endif()
if(OPT_DEBUG_COUNTER)
	set(RESUMEF_DEBUG_COUNTER 1)
endif()
if(OPT_KEEP_REAL_SIZE)
	set(_WITH_LOCK_FREE_Q_KEEP_REAL_SIZE 1)
endif()
if(OPT_DISABLE_MULT_THREAD)
	set(RESUMEF_DISABLE_MULT_THREAD 1)
endif()

configure_file(
${PROJECT_SOURCE_DIR}/config.h.in
${PROJECT_SOURCE_DIR}/librf/src/config.h
)

#{fmt}
add_definitions(-DFMT_HEADER_ONLY=1)
find_package(fmt)

include_directories(
${PROJECT_SOURCE_DIR}/librf
)

set(SRC 
${PROJECT_SOURCE_DIR}/librf/src/event_v2.cpp
${PROJECT_SOURCE_DIR}/librf/src/mutex_v2.cpp
${PROJECT_SOURCE_DIR}/librf/src/rf_task.cpp
${PROJECT_SOURCE_DIR}/librf/src/scheduler.cpp
${PROJECT_SOURCE_DIR}/librf/src/sleep.cpp
${PROJECT_SOURCE_DIR}/librf/src/state.cpp
${PROJECT_SOURCE_DIR}/librf/src/timer.cpp
${PROJECT_SOURCE_DIR}/librf/src/when_v2.cpp
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

#add_library(librf SHARED ${SRC})
add_library(librf STATIC ${SRC})


include_directories(
${PROJECT_SOURCE_DIR}/modern_cb
)

#aux_source_directory(${PROJECT_SOURCE_DIR}/tutorial TSRC)
#set(TSRC ${TSRC}
#${PROJECT_SOURCE_DIR}/benchmark/benchmark_async_mem.cpp
#${PROJECT_SOURCE_DIR}/benchmark/benchmark_channel_passing_next.cpp
#${PROJECT_SOURCE_DIR}/test_librf.cpp
#)

if(OPT_USE_MIMALLOC)
	set(LIB_MIMALLOC, "mimalloc")
else()
	set(LIB_MIMALLOC, "")
endif()

add_subdirectory(tutorial)
