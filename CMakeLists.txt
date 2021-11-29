cmake_minimum_required(VERSION 3.22.0)
project(templates VERSION 0.0.1 DESCRIPTION "C++ templates")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET1 "threaded_input_loop")
set(TARGET2 "threaded_lambda_input_loop")
set(TARGET3 "asio_tcp_server")
set(TARGET4 "asio_tcp_client")
set(TARGET5 "asio_udp_server")
set(TARGET6 "asio_udp_client")

# Change this to your package manager toolchain if you're not using vcpkg.
set(VCPKG_ROOT "P:/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/packages")

# I'm not moving headers over one at a time for boost
include_directories("${VCPKG_ROOT}/installed/x64-windows/include")


if(WIN32)
	# Set vcpkg triplet based on OS architecture
	# 64 bits
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(VCPKG_TARGET_TRIPLET "x64-windows")
	# 32 bits
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(VCPKG_TARGET_TRIPLET "x84-windows")
	endif()

    macro(get_WIN32_WINNT version)
        if(CMAKE_SYSTEM_VERSION)
            set(ver ${CMAKE_SYSTEM_VERSION})
            string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
            string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
            # Check for Windows 10, b/c we'll need to convert to hex 'A'.
            if("${verMajor}" MATCHES "10")
                set(verMajor "A")
                string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
            endif()
            # Remove all remaining '.' characters.
            string(REPLACE "." "" ver ${ver})
            # Prepend each digit with a zero.
            string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
            set(${version} "0x${ver}")
        endif()
    endmacro()

    get_WIN32_WINNT(ver)
    add_definitions(-D_WIN32_WINNT=${ver})
endif()

add_executable(${TARGET1} threaded_input_loop.cpp)
add_executable(${TARGET2} threaded_lambda_input_loop.cpp)
add_executable(${TARGET3} asio_tcp_server.cpp)
add_executable(${TARGET4} asio_tcp_client.cpp)
