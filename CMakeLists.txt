cmake_minimum_required(VERSION 3.10)
project(librf)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-std=c++2a -fcoroutines-ts -stdlib=libc++")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(CMAKE_CXX_FLAGS "-std=c++2a -fcoroutines-ts")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_CXX_FLAGS "/std:c++latest /await /EHsc")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "-std=c++2a -fcoroutines -pthread")
	
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -ggdb")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
	endif()
endif()

message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	option(OPT_INLINE_STATE "Enable Inline state" ON)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	option(OPT_INLINE_STATE "Enable Inline state" ON)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	option(OPT_INLINE_STATE "Enable Inline state" ON)
else()
	option(OPT_INLINE_STATE "Enable Inline state" OFF)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	option(OPT_USE_CONCEPT "Use conecpt instead of enable_if" ON)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	option(OPT_USE_CONCEPT "Use conecpt instead of enable_if" OFF)
else()
	option(OPT_USE_CONCEPT "Use conecpt instead of enable_if" OFF)
endif()

option(OPT_DEBUG_COUNTER "Debug objects count" OFF)
option(OPT_KEEP_REAL_SIZE "Keep real size in queue" OFF)

#set(RESUMEF_USE_CUSTOM_SPINLOCK "std::mutex")

if(OPT_INLINE_STATE)
	set(RESUMEF_INLINE_STATE 1)
endif()
if(OPT_DEBUG_COUNTER)
	set(RESUMEF_DEBUG_COUNTER 1)
endif()
if(OPT_USE_CONCEPT)
	set(RESUMEF_ENABLE_CONCEPT 1)
endif()
if(OPT_KEEP_REAL_SIZE)
	set(_WITH_LOCK_FREE_Q_KEEP_REAL_SIZE 1)
endif()

configure_file(
${PROJECT_SOURCE_DIR}/config.h.in
${PROJECT_SOURCE_DIR}/librf/src/config.h)


include_directories(
${PROJECT_SOURCE_DIR}/librf
)

set(SRC 
${PROJECT_SOURCE_DIR}/librf/src/event_v1.cpp
${PROJECT_SOURCE_DIR}/librf/src/event_v2.cpp
${PROJECT_SOURCE_DIR}/librf/src/mutex_v1.cpp
${PROJECT_SOURCE_DIR}/librf/src/mutex_v2.cpp
${PROJECT_SOURCE_DIR}/librf/src/rf_task.cpp
${PROJECT_SOURCE_DIR}/librf/src/scheduler.cpp
${PROJECT_SOURCE_DIR}/librf/src/sleep.cpp
${PROJECT_SOURCE_DIR}/librf/src/state.cpp
${PROJECT_SOURCE_DIR}/librf/src/timer.cpp
${PROJECT_SOURCE_DIR}/librf/src/when_v2.cpp
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(LIB_NAME librf)
#add_library(${LIB_NAME} SHARED ${SRC})
add_library(${LIB_NAME} STATIC ${SRC})



include_directories(
${PROJECT_SOURCE_DIR}/modern_cb
)

aux_source_directory(${PROJECT_SOURCE_DIR}/tutorial TSRC)
set(TSRC ${TSRC}
${PROJECT_SOURCE_DIR}/benchmark/benchmark_async_mem.cpp
${PROJECT_SOURCE_DIR}/benchmark/benchmark_channel_passing_next.cpp
${PROJECT_SOURCE_DIR}/test_librf.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
add_executable(rf_tutorial ${TSRC})
target_link_libraries(rf_tutorial ${LIB_NAME})