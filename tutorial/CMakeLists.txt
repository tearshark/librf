set(TUTORIAL_FILES "")
aux_source_directory("${CMAKE_SOURCE_DIR}/tutorial" TUTORIAL_FILES)

foreach(TUTORIAL_FILE_PATH ${TUTORIAL_FILES})
    string(REGEX REPLACE ".+[/\]([^/\.]+)\\.cpp" "\\1" TUTORIAL_FILE_NAME ${TUTORIAL_FILE_PATH})
    message(STATUS "Generating test target: ${TUTORIAL_FILE_NAME}")

    add_executable(${TUTORIAL_FILE_NAME} ${TUTORIAL_FILE_PATH})
    target_link_libraries(${TUTORIAL_FILE_NAME} librf ${LIB_MIMALLOC})
endforeach(TUTORIAL_FILE_PATH)
