include(SelectLibraryConfigurations)

macro(_acl_copy_dynamic_library_build_type basename build_type)

    if(${build_type} STREQUAL "Debug")
        set(_acl_build_type_dir "Debug")
        set(_acl_runtime_output_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
    elseif(${build_type} STREQUAL "Release")
        set(_acl_build_type_dir "Release")
        set(_acl_runtime_output_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
    elseif(${build_type} STREQUAL "MinSizeRel")
        set(_acl_build_type_dir "Release")
        set(_acl_runtime_output_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL})
    elseif(${build_type} STREQUAL "RelWithDebInfo")
        set(_acl_build_type_dir "Release")
        set(_acl_runtime_output_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO})
    endif()
    
    #message(STATUS "_acl_build_type_dir=${CMAKE_CURRENT_LIST_DIR}/../lib/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}-${_acl_build_type_dir}")
    #message(STATUS "_acl_runtime_output_dir=${_acl_runtime_output_dir}")

    find_file(_acl_${basename}_dynamic_binary
        NAMES "Acl.${basename}.dll" "Acl.${basename}.so"
        PATHS
        ${CMAKE_CURRENT_LIST_DIR}/../bin/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}-${_acl_build_type_dir}
        NO_DEFAULT_PATH
    )
    if(NOT _acl_${basename}_dynamic_binary)
        find_file(_acl_${basename}_dynamic_binary
            NAMES "${basename}.dll" "${basename}.so"
            PATHS
            ${CMAKE_CURRENT_LIST_DIR}/../bin/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}-${_acl_build_type_dir}
            NO_DEFAULT_PATH
        )
    endif()

    #message(STATUS "_acl_runtime_dynamic_binary=${_acl_runtime_dynamic_binary}")

    if(_acl_${basename}_dynamic_binary)
        file(INSTALL ${_acl_${basename}_dynamic_binary} DESTINATION ${_acl_runtime_output_dir})
    endif()

    unset(_acl_build_type_dir)
    unset(_acl_runtime_output_dir)
    unset(_acl_${basename}_dynamic_binary CACHE)
endmacro(_acl_copy_dynamic_library_build_type)

macro(_acl_copy_dynamic_library basename)

    if(DEFINED CMAKE_CONFIGURATION_TYPES)
        foreach(build_type ${CMAKE_CONFIGURATION_TYPES})
            _acl_copy_dynamic_library_build_type(${basename} ${build_type})
        endforeach(build_type)
    elseif(DEFINED CMAKE_BUILD_TYPE)
        _acl_copy_dynamic_library_build_type(${basename} ${CMAKE_BUILD_TYPE})
    else()
        _acl_copy_dynamic_library_build_type(${basename} "Release")
    endif()

endmacro(_acl_copy_dynamic_library)


macro(select_dynamic_library basename header)
    #message(STATUS "basename=${basename}")
    #message(STATUS "header=${header}")

    # 如果已经找到了 basename 指定的模块，则只做拷贝运行时的动态库的工作
    #message(STATUS "${basename}_FOUND=${${basename}_FOUND}")
    if(${basename}_FOUND)
        _acl_copy_dynamic_library(${basename})
        return()
    endif()



    # 查找头文件所在的路径
    find_path(${basename}_INCLUDE_DIR ${header}
        ${CMAKE_CURRENT_LIST_DIR}/../include
        NO_DEFAULT_PATH
    )

    # 查找调试版本的库文件所在路径
    find_library("${basename}_LIBRARY_DEBUG"
        NAMES "Acl.${basename}"
        PATHS
        ${CMAKE_CURRENT_LIST_DIR}/../lib/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}-Debug
        NO_DEFAULT_PATH
    )
    if(NOT ${basename}_LIBRARY_DEBUG)
        find_library("${basename}_LIBRARY_DEBUG"
            NAMES "${basename}"
            PATHS
            ${CMAKE_CURRENT_LIST_DIR}/../lib/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}-Debug
            NO_DEFAULT_PATH
        )
    endif()

    # 查找发行版本的库文件所在路径
    find_library("${basename}_LIBRARY_RELEASE"
        NAMES "Acl.${basename}"
        PATHS
        ${CMAKE_CURRENT_LIST_DIR}/../lib/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}-Release
        NO_DEFAULT_PATH
    )
    if(NOT ${basename}_LIBRARY_RELEASE)
        find_library("${basename}_LIBRARY_RELEASE"
            NAMES "${basename}"
            PATHS
            ${CMAKE_CURRENT_LIST_DIR}/../lib/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}-Release
            NO_DEFAULT_PATH
        )
    endif()

    # 使用cmake内置的 select_library_configurations 函数生成 ${basename}_LIBRARY 字段
    select_library_configurations(${basename})



    set(${basename}_FOUND FALSE)
    if(${basename}_LIBRARY AND ${basename}_INCLUDE_DIR)

        set(${basename}_FOUND TRUE)
        if(NOT ${basename}_DIR)
            set(${basename}_DIR ${CMAKE_CURRENT_LIST_DIR})
        endif()

        # 做拷贝运行时的动态库到目标目录
        _acl_copy_dynamic_library(${basename})

        # 创建 basename 指定的导入接口模块
        if(NOT TARGET Acl::${basename})

            add_library(Acl::${basename} UNKNOWN IMPORTED)
            set_target_properties(Acl::${basename} PROPERTIES
              INTERFACE_INCLUDE_DIRECTORIES "${${basename}_INCLUDE_DIR}")

            set_property(TARGET Acl::${basename} APPEND PROPERTY
                IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(Acl::${basename} PROPERTIES
                IMPORTED_LOCATION_RELEASE "${${basename}_LIBRARY_RELEASE}")

            set_property(TARGET Acl::${basename} APPEND PROPERTY
                IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(Acl::${basename} PROPERTIES
                IMPORTED_LOCATION_DEBUG "${${basename}_LIBRARY_DEBUG}")
        endif()
    endif()



    mark_as_advanced(${basename}_DIR)
    mark_as_advanced(${basename}_LIBRARY)
    mark_as_advanced(${basename}_INCLUDE_DIR)

    #message(STATUS "${basename}_DIR=${${basename}_DIR}")
    #message(STATUS "${basename}_LIBRARY=${${basename}_LIBRARY}")
    #message(STATUS "${basename}_LIBRARY_DEBUG=${${basename}_LIBRARY_DEBUG}")
    #message(STATUS "${basename}_LIBRARY_RELEASE=${${basename}_LIBRARY_RELEASE}")
    #message(STATUS "${basename}_INCLUDE_DIR=${${basename}_INCLUDE_DIR}")

endmacro(select_dynamic_library)
