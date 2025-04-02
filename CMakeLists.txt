cmake_minimum_required(VERSION 3.25.1)
project(rtms VERSION 0.0.1 LANGUAGES CXX)

# ===== Build settings =====
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
cmake_policy(SET CMP0091 NEW)

# ===== Platform detection =====
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

message(STATUS "Building for ${RTMS_PLATFORM}-${RTMS_ARCH}")

# ===== Project structure =====
set(RTMS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(RTMS_LIB_DIR "${CMAKE_SOURCE_DIR}/lib")
set(RTMS_PLATFORM_LIB_DIR "${RTMS_LIB_DIR}/${RTMS_PLATFORM}-${RTMS_ARCH}")
set(RTMS_INCLUDE_DIR "${RTMS_LIB_DIR}/include")

set(RTMS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
get_filename_component(RTMS_OUTPUT_DIR "${RTMS_OUTPUT_DIR}" ABSOLUTE)

# ===== Library search locations =====
set(RTMS_SEARCH_PATHS
  "${RTMS_PLATFORM_LIB_DIR}"
  "${CMAKE_SOURCE_DIR}/build/Release"
  "${CMAKE_SOURCE_DIR}/build/Debug"
  "$ENV{RTMS_SDK_PATH}"
  "$ENV{LD_LIBRARY_PATH}"
  "/usr/local/lib"
  "/usr/lib"
)

# ===== Locate library and headers =====
find_path(RTMS_HEADER_DIR
  NAMES rtms_csdk.h rtms_common.h
  PATHS ${RTMS_INCLUDE_DIR} ${RTMS_SEARCH_PATHS}
  DOC "RTMS SDK headers"
)

find_library(RTMS_LIBRARY
  NAMES librtmsdk.so.0 librtms_sdk.dylib
  PATHS ${RTMS_SEARCH_PATHS}
  DOC "RTMS SDK library"
)

# Error if not found
if(NOT RTMS_LIBRARY)
  message(FATAL_ERROR "RTMS SDK library not found. Please install it or specify its location.")
endif()

if(NOT RTMS_HEADER_DIR)
  message(FATAL_ERROR "RTMS SDK headers not found. Please install them or specify their location.")
endif()

# Get library information
get_filename_component(RTMS_LIB_FILENAME ${RTMS_LIBRARY} NAME)
get_filename_component(RTMS_LIB_DIR ${RTMS_LIBRARY} DIRECTORY)

message(STATUS "Found RTMS library: ${RTMS_LIBRARY} (${RTMS_LIB_FILENAME})")
message(STATUS "Found RTMS headers: ${RTMS_HEADER_DIR}")

# ===== Set up includes =====
include_directories(${RTMS_HEADER_DIR})
include_directories(${CMAKE_JS_INC})

# ===== Core library sources =====
file(GLOB RTMS_CORE_SOURCES
  "${RTMS_SOURCE_DIR}/rtms.h"
  "${RTMS_SOURCE_DIR}/rtms.cpp"
)

# ===== Node.js binding target =====
if(DEFINED CMAKE_JS_VERSION)
  message(STATUS "Building Node.js bindings")

  # Include Node API headers
  include_directories(node_modules/node-addon-api)

  # Enable exceptions
  add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS)
  add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS_ALL)

  # Set NAPI version
  set(NAPI_VERSION 9)
  add_compile_definitions(NAPI_VERSION=${NAPI_VERSION})

  # Create Node addon
  add_library(${PROJECT_NAME} SHARED
    ${RTMS_CORE_SOURCES}
    "${RTMS_SOURCE_DIR}/node.cpp"
  )

  # Set output properties
  set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    OUTPUT_NAME "rtms"
  )

  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    BUILD_RPATH "${RTMS_OUTPUT_DIR}"
    INSTALL_RPATH "${RTMS_OUTPUT_DIR}")


  # Set up direct linking to the RTMS library
  set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY
    LINK_FLAGS " -L${RTMS_LIB_DIR}")


  # Platform-specific linker settings
  if(${RTMS_PLATFORM} STREQUAL "darwin")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/build/${CMAKE_BUILD_TYPE}")

    # Convert library name for macOS linking (remove lib prefix and extension)
    string(REGEX REPLACE "^lib" "" RTMS_LIB_NAME_CLEAN ${RTMS_LIB_FILENAME})
    string(REGEX REPLACE "\\.dylib$" "" RTMS_LIB_NAME_CLEAN ${RTMS_LIB_NAME_CLEAN})

    # Link using standard macOS syntax
    target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} ${RTMS_LIBRARY})

    # Find all .framework directories
    file(GLOB FRAMEWORK_DIRS ${RTMS_PLATFORM_LIB_DIR}/*.framework)

    # Create tar archives for each .framework directory
    foreach(FRAMEWORK_PATH ${FRAMEWORK_DIRS})
      get_filename_component(FRAMEWORK_NAME ${FRAMEWORK_PATH} NAME)
      add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar czf
        "${RTMS_OUTPUT_DIR}/${FRAMEWORK_NAME}.tar.gz"
        "${FRAMEWORK_PATH}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Tarring ${FRAMEWORK_NAME} framework for bundling"
      )
    endforeach()

    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${RTMS_LIB_DIR}
      ${RTMS_OUTPUT_DIR}
      COMMENT "Copying .dylibs to output directory"
    )
  elseif(${RTMS_PLATFORM} STREQUAL "linux")
    set(CMAKE_SKIP_RPATH TRUE)

    set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY
      LINK_FLAGS " -Wl,-rpath,'$ORIGIN'")

    # Link using Linux-specific syntax with colon
    target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} -l:${RTMS_LIB_FILENAME})

    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${RTMS_LIBRARY}
      ${RTMS_OUTPUT_DIR}/${RTMS_LIB_FILENAME}
      COMMENT "Copying ${RTMS_LIB_FILENAME} to output directory"
    )
  endif()

  # Post-build: TypeScript compilation
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND tsc --outDir ${RTMS_OUTPUT_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling TypeScript files"
  )

  # ===== Python binding target =====
elseif(DEFINED SKBUILD_PROJECT_NAME)
  message(STATUS "Building Python bindings")

  # Get pybind11
  set(PYBIND11_FINDPYTHON ON)
  find_package(pybind11 CONFIG REQUIRED)

  # Create the Python module
  set(PYTHON_MODULE_NAME "_${PROJECT_NAME}")
  pybind11_add_module(${PYTHON_MODULE_NAME}
    ${RTMS_CORE_SOURCES}
    "${RTMS_SOURCE_DIR}/python.cpp"
  )

  # Link with the RTMS library
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND_STRING PROPERTY
    LINK_FLAGS " -L${RTMS_LIB_DIR}")

  # Linux: Set RPATH directly
  if(UNIX AND NOT APPLE)
    set_property(TARGET ${PYTHON_MODULE_NAME} APPEND_STRING PROPERTY
      LINK_FLAGS " -Wl,-rpath,'$$ORIGIN'")
  endif()

  target_link_libraries(${PYTHON_MODULE_NAME} PRIVATE -l:${RTMS_LIB_FILENAME})

  # Installation
  install(TARGETS ${PYTHON_MODULE_NAME} DESTINATION .)
  install(FILES ${RTMS_LIBRARY} DESTINATION .)

  # ===== Go binding target (placeholder) =====
elseif(GO)
  message(STATUS "Go bindings not yet implemented")
endif()
