cmake_minimum_required(VERSION 3.25.1)
project (rtms_js_sdk)
set(CMAKE_CXX_STANDARD 20)

cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(RTMS_SDK ${CMAKE_SOURCE_DIR}/lib/rtms_csdk)
set(RTMS_LIB ${RTMS_SDK}/librtmsdk.so)

add_definitions(-DNAPI_VERSION=4)
add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS)
add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS_ALL)

include_directories(${CMAKE_JS_INC})
include_directories(${RTMS_SDK})
include_directories(node_modules/node-addon-api)

link_directories(${RTMS_LIB} ${RTMS_LIB})

file(GLOB SOURCE_FILES "src/node.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC}
        src/rtms.h
        src/rtms.cpp
        src/common.h
        src/node.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} ${RTMS_LIB})


if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
    # Generate node.lib
    execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()
