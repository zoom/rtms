cmake_minimum_required(VERSION 3.25.1)
project (rtms_sdk VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(RTMS_SDK ${CMAKE_SOURCE_DIR}/lib/rtms_csdk)
set(RTMS_LIB ${RTMS_SDK}/librtmsdk.so)

include_directories(${CMAKE_JS_INC})
include_directories(${RTMS_SDK})
link_directories(${RTMS_LIB} ${RTMS_LIB})

file(GLOB SOURCE_FILES src/rtms.h src/rtms.cpp)

if(NODE)
    set(NODE_SRC ${CMAKE_SOURCE_DIR}/bin)
    set(NODE_DIST ${CMAKE_SOURCE_DIR}/dist/js/)

    file(MAKE_DIRECTORY ${NODE_DIST})
    file(COPY ${NODE_SRC}/index.js DESTINATION ${NODE_DIST})

    add_definitions(-DNAPI_VERSION=4)
    add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS)
    add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS_ALL)

    include_directories(node_modules/node-addon-api)

    add_library(${PROJECT_NAME} SHARED ${CMAKE_JS_SRC} ${SOURCE_FILES}
            src/node.cpp
    )

    set_target_properties(${PROJECT_NAME} 
        PROPERTIES PREFIX "" SUFFIX ".node"
        LIBRARY_OUTPUT_DIRECTORY ${NODE_DIST}
        RUNTIME_OUTPUT_DIRECTORY ${NODE_DIST}
    )
    
    if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
        # Generate node.lib
        execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
    endif()

    target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} ${RTMS_LIB})
elseif(PYTHON)
    set(PYBIND11_FINDPYTHON ON)
    find_package(pybind11 CONFIG REQUIRED)

    pybind11_add_module(${PROJECT_NAME} ${SOURCE_FILES}
        src/python.cpp
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE ${RTMS_LIB})

    install(TARGETS ${PROJECT_NAME} DESTINATION .)
elseif(GO)

endif()

