cmake_minimum_required(VERSION 3.5)
project(cortex.python-runtime C CXX)
set(TARGET engine)

include(CheckIncludeFileCXX)
check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)

if(HAS_ANY AND HAS_STRING_VIEW AND HAS_COROUTINE)
  set(CMAKE_CXX_STANDARD 20)
elseif(HAS_ANY AND HAS_STRING_VIEW)
  set(CMAKE_CXX_STANDARD 17)
else()
  set(CMAKE_CXX_STANDARD 14)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(OPENSSL_USE_STATIC_LIBS TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install)
SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(CMAKE_CXX_STANDARD LESS 17)
  # With C++14, use boost to support any and std::string_view
  message(STATUS "use c++14")
  find_package(Boost 1.61.0 REQUIRED)
  target_include_directories(${TARGET} PRIVATE ${Boost_INCLUDE_DIRS})
elseif(CMAKE_CXX_STANDARD LESS 20)
  message(STATUS "use c++17")
else()
  message(STATUS "use c++20")
endif()
# This is the critical line for installing another package

add_library(${TARGET} SHARED src/python_runtime_engine.cc)

if(UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC")
  add_compile_options(-fPIC)
endif()

set(THIRD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install)

find_library(JSONCPP
    NAMES jsoncpp
    HINTS "${THIRD_PARTY_PATH}/lib/"
)

find_library(TRANTOR
    NAMES trantor
    HINTS "${THIRD_PARTY_PATH}/lib/"
)

if(WIN32)
  # dlfcn for dynamic library
  set(dlfcn-win32_DIR "${THIRD_PARTY_PATH}/lib/cmake/dlfcn-win32")
  find_package(dlfcn-win32 REQUIRED)
  target_link_libraries(${TARGET} PRIVATE dlfcn-win32::dl)
endif()

target_link_libraries(${TARGET} PRIVATE ${JSONCPP} ${TRANTOR} ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${THIRD_PARTY_PATH}/include/)
