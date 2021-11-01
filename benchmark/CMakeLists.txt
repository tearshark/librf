set(BENCHMARK_FILES "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} BENCHMARK_FILES)

foreach(BENCHMARK_FILE_PATH ${BENCHMARK_FILES})
    string(REGEX REPLACE ".+[/\]([^/\.]+)\\.cpp" "\\1" BENCHMARK_FILE_NAME ${BENCHMARK_FILE_PATH})
    message(STATUS "Generating benchmark target: ${BENCHMARK_FILE_NAME}")

    add_executable(${BENCHMARK_FILE_NAME} ${BENCHMARK_FILE_PATH})
    target_link_libraries(${BENCHMARK_FILE_NAME} PUBLIC librf)
endforeach(BENCHMARK_FILE_PATH)
