cmake_minimum_required(VERSION 3.25.1)
project(rtms VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

# Detect platform and architecture
if(APPLE)
  set(RTMS_PLATFORM "darwin")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(RTMS_ARCH "arm64")
  else()
    set(RTMS_ARCH "x64")
  endif()
elseif(UNIX AND NOT APPLE)
  set(RTMS_PLATFORM "linux")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(RTMS_ARCH "arm64")
  else()
    set(RTMS_ARCH "x64")
  endif()
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

set(RTMS_PLATFORM_LIB_DIR "${CMAKE_SOURCE_DIR}/lib/${RTMS_PLATFORM}-${RTMS_ARCH}")
set(RTMS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/include")
set(RTMS_LIB_NAME "rtmsdk")

if(APPLE)
  set(RTMS_LIB_EXT "dylib")
elseif(UNIX AND NOT APPLE)
  set(RTMS_LIB_EXT "so")
endif()


message(STATUS "Building for ${RTMS_PLATFORM}-${RTMS_ARCH}")

# Set the search paths for the RTMS SDK
set(RTMS_SDK_SEARCH_PATHS
  "${RTMS_PLATFORM_LIB_DIR}"
  "$ENV{LD_LIBRARY_PATH}"
  "$ENV{RTMS_SDK_PATH}"
  "/usr/local/lib"
  "/usr/lib"
)
if(APPLE)
  set(CMAKE_FRAMEWORK_PATH ${CMAKE_FRAMEWORK_PATH} "${RTMS_PLATFORM_LIB_DIR}")
  set(CMAKE_MACOSX_RPATH ON)
endif()

if($ENV{RTMS_SDK_PATH})
  list(INSERT RTMS_SDK_SEARCH_PATHS 0 "$ENV{RTMS_SDK_PATH}")
endif()

# Find the header directory
find_path(RTMS_INCLUDE_DIR
  NAMES rtms_csdk.h rtms_common.h
  PATHS "${CMAKE_SOURCE_DIR}/lib/include"
  ${RTMS_SDK_SEARCH_PATHS}
  DOC "RTMS SDK header directory"
)

# Find the shared library files
find_library(RTMS_LIBRARY
  NAMES librtmsdk.so.0 librtms-sdk.dylib
  PATHS ${RTMS_SDK_SEARCH_PATHS}
  DOC "RTMS SDK library"
)

message(STATUS "Found RTMS library: ${RTMS_LIBRARY}")
message(STATUS "Found RTMS include dir: ${RTMS_INCLUDE_DIR}")

get_filename_component(RTMS_LIB_DIR ${RTMS_LIBRARY} DIRECTORY)

include_directories(${CMAKE_JS_INC})
include_directories(${RTMS_INCLUDE_DIR})

add_library(rtmSdk SHARED IMPORTED)
set_target_properties(rtmSdk PROPERTIES IMPORTED_LOCATION ${RTMS_LIBRARY})

file(GLOB SOURCE_FILES src/rtms.h src/rtms.cpp)

# If using cmake-js this is the Node Addon API, CMAKE_JS_VERSION is defined
# If using scikit-build-core this is the Python bindings, SKBUILD_PROJECT_NAME is defined
if(DEFINED CMAKE_JS_VERSION)
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

  target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} rtmSdk)

  set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

  # run tsc to compile TypeScript files
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND tsc --outDir ${OUTPUT_DIR}
  )

  # copy shared object files to the output directory
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${RTMS_PLATFORM_LIB_DIR}
    ${OUTPUT_DIR}
    COMMENT "Copying shared object files to ${OUTPUT_DIR}"
  )

  if(APPLE)
    # Find all .framework directories
    file(GLOB FRAMEWORK_DIRS ${RTMS_PLATFORM_LIB_DIR}/*.framework)

    # Create tar archives for each .framework directory
    foreach(FRAMEWORK_PATH ${FRAMEWORK_DIRS})
      get_filename_component(FRAMEWORK_NAME ${FRAMEWORK_PATH} NAME)
      add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar czf
        "${OUTPUT_DIR}/${FRAMEWORK_NAME}.tar.gz"
        "${FRAMEWORK_PATH}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Tarring ${FRAMEWORK_NAME}"
      )
    endforeach()
  endif()
elseif(DEFINED SKBUILD_PROJECT_NAME)
  set(PY_TARGET "_${PROJECT_NAME}")
  set(PYBIND11_FINDPYTHON ON)
  find_package(pybind11 CONFIG REQUIRED)

  pybind11_add_module(${PY_TARGET} ${SOURCE_FILES}
    src/python.cpp
  )

  target_link_libraries(${PY_TARGET} PRIVATE rtmSdk)

  set_target_properties(${PY_TARGET} PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN;${RTMS_LIB_DIR}"
  )

  install(TARGETS ${PY_TARGET} DESTINATION .)

  install(FILES ${RTMS_LIBRARY}
    DESTINATION .
    COMPONENT SharedLibrary
  )

elseif(GO)

endif()
