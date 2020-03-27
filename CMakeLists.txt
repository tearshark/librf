cmake_minimum_required(VERSION 3.14)
PROJECT(librf)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-std=c++20 -fcoroutines-ts")	
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(CMAKE_CXX_FLAGS "-std=c++20 -fcoroutines-ts")	
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CMAKE_CXX_FLAGS "/std:c++latest /await /EHsc")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "-std=c++20 -fcoroutines-ts")	
endif()

message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")

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



aux_source_directory(${PROJECT_SOURCE_DIR}/tutorial TSRC)
set(TSRC ${TSRC}
${PROJECT_SOURCE_DIR}/benchmark/benchmark_async_mem.cpp
${PROJECT_SOURCE_DIR}/benchmark/benchmark_channel_passing_next.cpp
${PROJECT_SOURCE_DIR}/test_librf.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
add_executable(test ${TSRC})
target_link_libraries(test ${LIB_NAME})