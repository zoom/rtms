cmake_minimum_required(VERSION 3.25.1)
project(rtms VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

set(RTMS_SDK_SEARCH_PATHS
  "${CMAKE_SOURCE_DIR}/lib/librtmsdk"  
  "$ENV{LD_LIBRARY_PATH}"              
  "/usr/local/lib"                   
  "/usr/lib"
)

if(RTMS_SDK_PATH)
  list(INSERT RTMS_SDK_SEARCH_PATHS 0 "${RTMS_SDK_PATH}")
endif()

# Find the header directory
find_path(RTMS_INCLUDE_DIR
  NAMES rtms_csdk.h rtms_common.h
  PATHS ${RTMS_SDK_SEARCH_PATHS}
  PATH_SUFFIXES include h
  DOC "RTMS SDK header directory"
)

# Find the library
find_library(RTMS_LIBRARY
  NAMES rtmsdk librtmsdk librtmsdk.so.0 rtmsdk.so.0
  PATHS ${RTMS_SDK_SEARCH_PATHS}
  DOC "RTMS SDK library"
)

if(NOT RTMS_LIBRARY)
  message(FATAL_ERROR "RTMS SDK library not found. Please install it or specify its location with -DRTMS_SDK_PATH=<path>")
endif()

get_filename_component(RTMS_LIB_DIR ${RTMS_LIBRARY} DIRECTORY)

message(STATUS "Found RTMS library: ${RTMS_LIBRARY}")
message(STATUS "Found RTMS include dir: ${RTMS_INCLUDE_DIR}")

include_directories(${CMAKE_JS_INC})
include_directories(${RTMS_INCLUDE_DIR})

file(GLOB SOURCE_FILES src/rtms.h src/rtms.cpp)

# is Node
if(CMAKE_JS_VERSION)
    include_directories(node_modules/node-addon-api)

    set(NODE_ADDON ${CMAKE_SOURCE_DIR}/src/node.cpp)
    add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS)
    add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS_ALL)

    if(NOT napi_build_version)
        set(napi_build_version 9)
    endif()

    add_compile_definitions(NAPI_VERSION=${napi_build_version})

    add_library(${PROJECT_NAME} SHARED ${CMAKE_JS_SRC} ${SOURCE_FILES} ${NODE_ADDON})

    set_target_properties(${PROJECT_NAME} PROPERTIES
      PREFIX "" SUFFIX ".node"
      BUILD_WITH_INSTALL_RPATH TRUE
      INSTALL_RPATH "$ORIGIN;${RTMS_LIB_DIR}"
    )

    target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} ${RTMS_LIBRARY})
elseif(PYTHON)
    set(PY_TARGET "_${PROJECT_NAME}")
    set(PYBIND11_FINDPYTHON ON)
    find_package(pybind11 CONFIG REQUIRED)

    pybind11_add_module(${PY_TARGET} ${SOURCE_FILES}
        src/python.cpp
    )

    # Modern CMake linking
    target_link_libraries(${PY_TARGET} PRIVATE ${RTMS_LIBRARY})
    
    # Set RPATH for Python module
    set_target_properties(${PY_TARGET} PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN;${RTMS_LIB_DIR}"
    )

    # Install the Python module
    install(TARGETS ${PY_TARGET} DESTINATION ${SKBUILD_PROJECT_NAME})

elseif(GO)
    # Go implementation future placeholder
endif()