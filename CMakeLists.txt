cmake_minimum_required(VERSION 3.10)
project(librf VERSION 3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(LIBRF_COMPILER_SETTING )
if(WIN32)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(LIBRF_COMPILER_SETTING "clang_on_msvc")
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
message(STATUS "CMAKE_CXX_COMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}")

if(${LIBRF_COMPILER_SETTING} STREQUAL "msvc")
	if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "19.30.0.0")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /EHsc")		#VS2022
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /EHsc /await")	#VS2019
	endif()
elseif ("${LIBRF_COMPILER_SETTING}" STREQUAL "clang_on_msvc")
	if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "12.0.0")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /EHsc -Wno-unused-private-field")		#VS2022
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /EHsc -Wno-unused-private-field")	#VS2019
	endif()
elseif ("${LIBRF_COMPILER_SETTING}" STREQUAL "clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fcoroutines-ts -stdlib=libstdc++")
elseif ("${LIBRF_COMPILER_SETTING}" STREQUAL "gcc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fcoroutines -fconcepts-diagnostics-depth=8")
endif()


if(${LIBRF_COMPILER_SETTING} STREQUAL "msvc")
	option(LIBRF_INLINE_STATE "Enable Inline state" OFF)
elseif ("${LIBRF_COMPILER_SETTING}" STREQUAL "clang_on_msvc")
	option(LIBRF_INLINE_STATE "Enable Inline state" OFF)
elseif(${LIBRF_COMPILER_SETTING} STREQUAL "clang")
	option(LIBRF_INLINE_STATE "Enable Inline state" OFF)
elseif(${LIBRF_COMPILER_SETTING} STREQUAL "gcc")
	option(LIBRF_INLINE_STATE "Enable Inline state" OFF)
else()
	option(LIBRF_INLINE_STATE "Enable Inline state" OFF)
endif()

option(LIBRF_DEBUG_COUNTER "Debug objects count" OFF)
option(LIBRF_KEEP_REAL_SIZE "Keep real size in queue" OFF)
option(LIBRF_DISABLE_MULT_THREAD "Disable multi-threaded scheduler" OFF)
option(LIBRF_USE_MIMALLOC "Use mimalloc" OFF)
option(LIBRF_DYNAMIC_LIBRARY "Use shared library" ON)
option(CMAKE_ENABLE_UNIT_TEST "Enable unit test" OFF)

if (UNIX)
	if(LIBRF_USE_MIMALLOC)
		find_package(mimalloc 1.4 REQUIRED)
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pthread")
endif()

if(${LIBRF_COMPILER_SETTING} STREQUAL "msvc")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "/W3 /WX /MP /GS- /Gm- /Ox /Ob2 /Oy /Oi /Os /GT /EHsc /Zc:inline")
	set(CMAKE_CXX_FLAGS_RELEASE    "/W3 /WX /MP /GS- /Gm- /Ox /Ob2 /Oy /Oi /Os /GT /EHsc /Zc:inline")
elseif ("${LIBRF_COMPILER_SETTING}" STREQUAL "clang_on_msvc")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "/W3 /GS- /Ox /Ob2 /Oy /Oi /Os /EHsc /Zc:inline")
	set(CMAKE_CXX_FLAGS_RELEASE    "/W3 /GS- /Ox /Ob2 /Oy /Oi /Os /EHsc /Zc:inline")
elseif()
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
	endif()
endif()


message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")

#set(RESUMEF_USE_CUSTOM_SPINLOCK "std::mutex")

if(LIBRF_INLINE_STATE)
	set(RESUMEF_INLINE_STATE 1)
endif()
if(LIBRF_DEBUG_COUNTER)
	set(RESUMEF_DEBUG_COUNTER 1)
endif()
if(LIBRF_KEEP_REAL_SIZE)
	set(_WITH_LOCK_FREE_Q_KEEP_REAL_SIZE 1)
endif()
if(LIBRF_DISABLE_MULT_THREAD)
	set(RESUMEF_DISABLE_MULT_THREAD 1)
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_SOURCE_DIR}/include/librf/src/config.h
)


file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.*)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(LIBRF_DYNAMIC_LIBRARY)
	add_library(${PROJECT_NAME} SHARED
		${HEADER_FILES}
		${SOURCE_FILES}
	)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE LIBRF_DYNAMIC_EXPORTS=1
	)
else()
	add_library(${PROJECT_NAME} STATIC
		${HEADER_FILES}
		${SOURCE_FILES}
	)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE LIBRF_USE_STATIC_LIBRARY=1
	)
endif()

target_include_directories(${PROJECT_NAME}
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/modern_cb
)

if(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/")
endif(UNIX)

if(LIBRF_USE_MIMALLOC)
	set(LIB_MIMALLOC, "mimalloc")
else()
	set(LIB_MIMALLOC, "")
endif()

if(CMAKE_ENABLE_UNIT_TEST)
	add_subdirectory(tutorial)

	aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/tutorial ALL_TUTORIAL_FILES)
	add_executable(test_librf
		${CMAKE_CURRENT_SOURCE_DIR}/test_librf.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/benchmark/benchmark_async_mem.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/benchmark/benchmark_channel_passing_next.cpp
		${ALL_TUTORIAL_FILES})
	target_link_libraries(test_librf PUBLIC librf)
	if(UNIX)
		set_target_properties(test_librf PROPERTIES INSTALL_RPATH "$ORIGIN/")
	endif(UNIX)

	add_subdirectory(benchmark)
endif()


include(${CMAKE_SOURCE_DIR}/cmake/install.cmake)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/librf DESTINATION include)
