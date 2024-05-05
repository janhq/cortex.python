cmake_minimum_required(VERSION 3.5)
project(cortex.python-runtime C CXX)
SET(TARGET engine)

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
set(OPENSSL_USE_STATIC_LIBS TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install)

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
    HINTS "${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install/lib/"
)

find_library(TRANTOR
    NAMES trantor
    HINTS "${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install/lib/"
)


# Copy the python3 related files to the build folder
if (WIN32)
  # Copy the whole library
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install/python DESTINATION ${CMAKE_BINARY_DIR}/Release)
else() # APPLE and LINUX
  # Copy library files
  file(GLOB PYTHON_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install/lib/python*)
  if(PYTHON_LIBS)
    list(GET PYTHON_LIBS 0 FIRST_PYTHON_LIB)
    file(COPY ${FIRST_PYTHON_LIB} DESTINATION ${CMAKE_BINARY_DIR}/python/lib)
  endif()

  # Create symbolic link .../lib/python for target .../lib/python3.*
  file(GLOB PYTHON_LIBS_DES ${CMAKE_BINARY_DIR}/python/lib/python*)
  if(PYTHON_LIBS_DES)
    list(GET PYTHON_LIBS_DES 0 FIRST_PYTHON_LIB)
    file(CREATE_LINK ${FIRST_PYTHON_LIB} ${CMAKE_BINARY_DIR}/python/lib/python SYMBOLIC)
  endif()

  # Copy executable files
  file(GLOB PYTHON_EXECUTABLES ${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install/bin/python*)
  foreach(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLES})
      file(COPY ${PYTHON_EXECUTABLE} DESTINATION ${CMAKE_BINARY_DIR}/python/bin)
  endforeach()

  # Copy dynamic library files
  file(GLOB PYTHON_DLS ${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install/lib/libpython*)
  foreach(PYTHON_DL ${PYTHON_DLS})
      file(COPY ${PYTHON_DL} DESTINATION ${CMAKE_BINARY_DIR}/python)
  endforeach()
endif()

if(WIN32)
  # dlfcn for dynamic library
  find_package(dlfcn-win32 REQUIRED)
  target_link_libraries(${TARGET} PRIVATE dlfcn-win32::dl)
endif()


add_library(${TARGET} SHARED 
    src/PythonRuntimeEngine.cc
)

target_link_libraries(${TARGET} PRIVATE ${JSONCPP} ${TRANTOR} ${CMAKE_THREAD_LIBS_INIT})

# aux_source_directory(src SRC)
target_sources(${TARGET} PRIVATE ${SRC})

target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/build_deps/_install/include/)
