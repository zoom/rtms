cmake_minimum_required(VERSION 3.25.1)
project(rtms VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

set(RTMS_SDK ${CMAKE_SOURCE_DIR}/lib/librtmsdk)
set(RTMS_LIB ${RTMS_SDK}/librtmsdk.so.0)
file(GLOB SOURCE_FILES src/rtms.h src/rtms.cpp)

include_directories(${CMAKE_JS_INC})
include_directories(${RTMS_SDK}/h)
link_directories(${RTMS_LIB} ${RTMS_LIB})

if(NODE)
    include_directories(node_modules/node-addon-api)

    set(NODE_ADDON ${CMAKE_SOURCE_DIR}/src/node.cpp)
    add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS)
    add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS_ALL)

    if(NOT napi_build_version)
        set(napi_build_version 9)
    endif()
    add_compile_definitions(NAPI_VERSION=${napi_build_version})

    add_library(${PROJECT_NAME} SHARED ${CMAKE_JS_SRC} ${SOURCE_FILES} ${NODE_ADDON})

    set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX "" SUFFIX ".node"
    )

    target_link_libraries(${PROJECT_NAME} ${RTMS_LIB} ${CMAKE_JS_LIB})
elseif(PYTHON)
    set(PY_TARGET "_${PROJECT_NAME}")
    set(PYBIND11_FINDPYTHON ON)
    find_package(pybind11 CONFIG REQUIRED)

    pybind11_add_module(${PY_TARGET} ${SOURCE_FILES}
        src/python.cpp
    )

    target_link_libraries(${PY_TARGET} PRIVATE ${RTMS_LIB})

    # Install the Python module
    install(TARGETS ${PY_TARGET} DESTINATION ${SKBUILD_PROJECT_NAME})
elseif(GO)

endif()