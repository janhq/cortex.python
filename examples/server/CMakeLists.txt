cmake_minimum_required(VERSION 3.5)
project(server)

find_package(Threads REQUIRED)

if(UNIX AND NOT APPLE)
  set(LINKER_FLAGS -ldl)
endif()

include(CheckIncludeFileCXX)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)

if(HAS_ANY
   AND HAS_STRING_VIEW
   AND HAS_COROUTINE)
  set(CMAKE_CXX_STANDARD 20)
elseif(HAS_ANY AND HAS_STRING_VIEW)
  set(CMAKE_CXX_STANDARD 17)
else()
  set(CMAKE_CXX_STANDARD 14)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OPENSSL_USE_STATIC_LIBS TRUE)

add_executable(${PROJECT_NAME}
    server.cc
    dylib.h
    httplib.h
)

set(THIRD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../build_deps/_install)

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

find_library(JSONCPP
    NAMES jsoncpp
    HINTS "${THIRD_PARTY_PATH}/lib/"
)

find_library(TRANTOR
    NAMES trantor
    HINTS "${THIRD_PARTY_PATH}/lib/"
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${JSONCPP} ${TRANTOR} ${LINKER_FLAGS} ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ ${THIRD_PARTY_PATH}/include/)
