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

# Find all .framework directories
file(GLOB FRAMEWORK_DIRS ${RTMS_PLATFORM_LIB_DIR}/*.framework)

# ===== Node.js binding target =====
if(DEFINED CMAKE_JS_VERSION)
  message(STATUS "Building Node.js bindings")

  # Include Node API headers
  include_directories(node_modules/node-addon-api)

  # Enable exceptions
  add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS)
  add_definitions(-DNODE_ADDON_API_CPP_EXCEPTIONS_ALL)
  add_definitions(-DNODE_API_SWALLOW_UNHANDLED_EXCEPTIONS=0)

  # Set NAPI version - check multiple sources
  if(DEFINED ENV{npm_config_napi_build_version})
    # Use the version provided by prebuild via npm config
    set(NAPI_VERSION $ENV{npm_config_napi_build_version})
    message(STATUS "Using NAPI version from npm_config_napi_build_version: ${NAPI_VERSION}")
  elseif(DEFINED napi_build_version)
    # Use the version provided as a CMake variable
    set(NAPI_VERSION ${napi_build_version})
    message(STATUS "Using NAPI version from napi_build_version: ${NAPI_VERSION}")
  else()
    # Fall back to a default version
    set(NAPI_VERSION 10)
    message(STATUS "Using default NAPI version: ${NAPI_VERSION}")
  endif()

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

  # Use @loader_path to make RPATH relative to the binary
  set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "@loader_path"
    MACOSX_RPATH TRUE)


  # Set up direct linking to the RTMS library
  set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY
    LINK_FLAGS " -L${RTMS_LIB_DIR}")


  # Platform-specific linker settings
  if(${RTMS_PLATFORM} STREQUAL "darwin")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")

    # Convert library name for macOS linking (remove lib prefix and extension)
    string(REGEX REPLACE "^lib" "" RTMS_LIB_NAME_CLEAN ${RTMS_LIB_FILENAME})
    string(REGEX REPLACE "\\.dylib$" "" RTMS_LIB_NAME_CLEAN ${RTMS_LIB_NAME_CLEAN})

    # Link using standard macOS syntax
    target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} ${RTMS_LIBRARY})


    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${RTMS_LIB_DIR}
      ${RTMS_OUTPUT_DIR}
      COMMENT "Copying libraries to output directory"
    )

  # Create tar archives for each .framework directory
  foreach(FRAMEWORK_PATH ${FRAMEWORK_DIRS})
    get_filename_component(FRAMEWORK_NAME ${FRAMEWORK_PATH} NAME)
    add_custom_command(
      WORKING_DIRECTORY ${RTMS_OUTPUT_DIR}
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E tar czf
      "${FRAMEWORK_NAME}.tar.gz"
      "${FRAMEWORK_NAME}"
      COMMENT "Tarring ${FRAMEWORK_NAME} framework for bundling"
    )
  endforeach()


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

  # Platform-specific linker settings
  if(${RTMS_PLATFORM} STREQUAL "darwin")
    # macOS specific settings
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")

    
    # Use direct path to library for linking on macOS
    target_link_libraries(${PYTHON_MODULE_NAME} PRIVATE ${RTMS_LIBRARY})
    
    # Set RPATH to @loader_path for macOS
    set_target_properties(${PYTHON_MODULE_NAME} PROPERTIES
      BUILD_WITH_INSTALL_RPATH TRUE
      INSTALL_RPATH "@loader_path"
      MACOSX_RPATH TRUE)
    
    # Copy libraries to output directory
    add_custom_command(
      TARGET ${PYTHON_MODULE_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${RTMS_LIBRARY}
      ${CMAKE_CURRENT_BINARY_DIR}/
      COMMENT "Copying ${RTMS_LIB_FILENAME} to output directory"
    )

    file(GLOB DYLIB_FILES ${RTMS_PLATFORM_LIB_DIR}/*.dylib)
    foreach(DYLIB_FILE ${DYLIB_FILES})
      get_filename_component(DYLIB_NAME ${DYLIB_FILE} NAME)
      # Copy to build directory
      add_custom_command(
        TARGET ${PYTHON_MODULE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${DYLIB_FILE}
        ${CMAKE_CURRENT_BINARY_DIR}/${DYLIB_NAME}
        COMMENT "Copying ${DYLIB_NAME} to output directory"
      )
      # Install to package
      install(FILES ${DYLIB_FILE} DESTINATION rtms)
    endforeach()

    foreach(FRAMEWORK_PATH ${FRAMEWORK_DIRS})
      get_filename_component(FRAMEWORK_NAME ${FRAMEWORK_PATH} NAME)
      # Install the entire framework directory
      install(DIRECTORY ${FRAMEWORK_PATH}
              DESTINATION rtms
              USE_SOURCE_PERMISSIONS)
    endforeach()
  else()
    # Linux specific settings
    set_property(TARGET ${PYTHON_MODULE_NAME} APPEND_STRING PROPERTY
      LINK_FLAGS " -L${RTMS_LIB_DIR}")
      
    set_property(TARGET ${PYTHON_MODULE_NAME} APPEND_STRING PROPERTY
      LINK_FLAGS " -Wl,-rpath,'$$ORIGIN'")
      
    target_link_libraries(${PYTHON_MODULE_NAME} PRIVATE -l:${RTMS_LIB_FILENAME})
  endif()

  # Installation
  install(TARGETS ${PYTHON_MODULE_NAME} DESTINATION rtms)
  install(FILES ${RTMS_LIBRARY} DESTINATION rtms)
# ===== Go binding target (placeholder) =====
elseif(GO)
  message(STATUS "Go bindings not yet implemented")
endif()
