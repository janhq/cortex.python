cmake_minimum_required(VERSION 3.22)  # Required for FetchContent

project(MyProject)

include(ExternalProject)

# Define variables
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(THIRD_PARTY_INSTALL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../build_deps/_install)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC")
  add_compile_options(-fPIC)
endif()

ExternalProject_Add(
    c-ares
    GIT_REPOSITORY https://github.com/c-ares/c-ares
    GIT_TAG cares-1_26_0
    CMAKE_ARGS
    	-DCARES_SHARED=OFF
	    -DCARES_STATIC=ON
      -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    	-DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_PATH}
)

###############################
# Install dependencies for engine

# trantor for logging utils
ExternalProject_Add(
    trantor
    GIT_REPOSITORY https://github.com/an-tao/trantor
    GIT_TAG v1.5.17
    CMAKE_ARGS 
    	-DBUILD_SHARED_LIBS=OFF
      -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
      -DCMAKE_PREFIX_PATH=${THIRD_PARTY_INSTALL_PATH}
    	-DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_PATH}
)

# jsoncpp for json utils and request handling
ExternalProject_Add(
    jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp
    GIT_TAG 1.9.5
    CMAKE_ARGS 
    	-DBUILD_SHARED_LIBS=OFF
      -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    	-DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_PATH}
)

###############################
# Install dependencies for Python
if(UNIX)
  # Download and install XZUtils
  ExternalProject_Add(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib
    GIT_TAG v1.3.1
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=release
      -DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_PATH}
      -DBUILD_SHARED_LIBS=OFF
      -DBUILD_TESTS=OFF
  )
  if(APPLE)
    set(OPENSSL_CONFIG_COMMAND "./Configure")  
  else()
    set(OPENSSL_CONFIG_COMMAND "./config")  
    # Download and install OpenSSL
    # ExternalProject_Add(
    #   OpenSSL
    #   URL https://www.openssl.org/source/openssl-3.3.0.tar.gz
    #   PREFIX ${THIRD_PARTY_INSTALL_PATH}
    #   CONFIGURE_COMMAND ./Configure --prefix=<INSTALL_DIR> --openssldir=<INSTALL_DIR>
    #   BUILD_COMMAND make -j12
    #   INSTALL_COMMAND make install_sw
    #   BUILD_IN_SOURCE 1
    # )
    # Download and install libffi
    ExternalProject_Add(
      libffi
      URL ftp://sourceware.org/pub/libffi/libffi-3.1.tar.gz
      URL_MD5 f5898b29bbfd70502831a212d9249d10
      PREFIX ${THIRD_PARTY_INSTALL_PATH}
      CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR> --enable-shared
      BUILD_COMMAND make -j12
      INSTALL_COMMAND make install
      BUILD_IN_SOURCE 1
    )

    ExternalProject_Add_Step(
      libffi move_includes
      COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_INSTALL_PATH}/include
      COMMAND ${CMAKE_COMMAND} -E copy_directory <INSTALL_DIR>/lib/libffi-3.1/include ${THIRD_PARTY_INSTALL_PATH}/include
      DEPENDEES install
      WORKING_DIRECTORY ${THIRD_PARTY_INSTALL_PATH}
    )
  endif()

  # OpenSSL external project
  ExternalProject_Add(
      OpenSSL
      URL https://www.openssl.org/source/openssl-3.3.0.tar.gz
      PREFIX ${THIRD_PARTY_INSTALL_PATH}/openssl
      CONFIGURE_COMMAND ${OPENSSL_CONFIG_COMMAND} --prefix=<INSTALL_DIR> --openssldir=<INSTALL_DIR>
      BUILD_COMMAND make -j12
      INSTALL_COMMAND make install
      BUILD_IN_SOURCE 1
  )

else() # WINDOWS
	# Add dlfcn-win32 as an external project
	ExternalProject_Add(
		dlfcn-win32
		GIT_REPOSITORY https://github.com/dlfcn-win32/dlfcn-win32
		GIT_TAG v1.4.1
		CMAKE_ARGS
			-DCMAKE_BUILD_TYPE=release
			-DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_PATH}
			-DBUILD_SHARED_LIBS=OFF
			-DBUILD_TESTS=OFF
  )
endif()

###############################
# Install Python3 library
if (UNIX) # APPLE and LINUX
  # Define Python installing configuration
  if(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm.*|ARM64)$")
      set(PYTHON_INSTALL_CONFIG_HOST "--host=aarch64-apple-darwin")
      set(PYTHON_INSTALL_CONFIG_BUILD "--build=aarch64-apple-darwin")
    else()
      set(PYTHON_INSTALL_CONFIG_HOST "--host=x86_64-apple-darwin")
      set(PYTHON_INSTALL_CONFIG_BUILD "--build=x86_64-apple-darwin")
    endif()
    set(PYTHON_INSTALL_CONFIG_SSL "--with-openssl=${THIRD_PARTY_INSTALL_PATH}")
  else()
    set(PYTHON_INSTALL_CONFIG_HOST "")
    set(PYTHON_INSTALL_CONFIG_BUILD "")
    set(PYTHON_INSTALL_CONFIG_SSL "")
  endif()

  # Download and install Python3 from source
  ExternalProject_Add(
    Python3
    DEPENDS OpenSSL
    URL https://www.python.org/ftp/python/3.10.4/Python-3.10.4.tgz
    PREFIX ${THIRD_PARTY_INSTALL_PATH}
    CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR> --enable-optimizations --with-ensurepip=install --enable-shared ${PYTHON_INSTALL_CONFIG_HOST} ${PYTHON_INSTALL_CONFIG_BUILD} --with-openssl=PREFIX ${THIRD_PARTY_INSTALL_PATH}/openssl
    BUILD_COMMAND make -j12
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE 1
  )

  # Add dependencies for Python3
  if(APPLE)
    add_dependencies(Python3 zlib OpenSSL)
  else() # Linux
    add_dependencies(Python3 zlib libffi)
  endif()

else() # WINDOWS
  ExternalProject_Add(
    Python3
    PREFIX "${CMAKE_BINARY_DIR}/python"
    URL "https://www.python.org/ftp/python/3.10.4/python-3.10.4-amd64.exe"
    DOWNLOAD_NO_EXTRACT 1
    CONFIGURE_COMMAND ""  # No configure command
    BUILD_COMMAND ""  # No build command
    INSTALL_COMMAND ""  # No install command
  )

  # Download and install from binary
  ExternalProject_Add_Step(
    Python3 InstallPython
    COMMAND cmd.exe /c "${CMAKE_CURRENT_SOURCE_DIR}/install_python.bat"
  )
endif()

add_dependencies(trantor c-ares)

include_directories(${THIRD_PARTY_INSTALL_PATH}/include)
link_directories(${THIRD_PARTY_INSTALL_PATH}/lib)
# target_link_libraries(<your-target> ...)
