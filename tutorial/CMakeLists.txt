set(TUTORIAL_FILES "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} TUTORIAL_FILES)

foreach(TUTORIAL_FILE_PATH ${TUTORIAL_FILES})
    string(REGEX REPLACE ".+[/\]([^/\.]+)\\.cpp" "\\1" TUTORIAL_FILE_NAME ${TUTORIAL_FILE_PATH})
    message(STATUS "Generating test target: ${TUTORIAL_FILE_NAME}")

    add_executable(${TUTORIAL_FILE_NAME} ${TUTORIAL_FILE_PATH})
    target_link_libraries(${TUTORIAL_FILE_NAME} PUBLIC librf)
    target_compile_definitions(${TUTORIAL_FILE_NAME}
        PRIVATE LIBRF_TUTORIAL_STAND_ALONE=1
    )
    if(UNIX)
        set_target_properties(${TUTORIAL_FILE_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/")
    endif(UNIX)
endforeach(TUTORIAL_FILE_PATH)
