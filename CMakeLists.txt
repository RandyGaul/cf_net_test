cmake_minimum_required(VERSION 3.0)
project(cf_net_test)

# Must have at least C++14.
set(CMAKE_CXX_STANDARD 14)

# Make sure all binaries are placed into the same build folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CUTE_FRAMEWORK_STATIC ON)

# This will download and build Cute Framework just once the first time you build your game.
# There are other ways to get cute, such as downloading and installing prebuilt binaries, or
# by building Cute Framework yourself from source and installing the binaries on your machine.
# Using FetchContent here is just the simplest way, but ultimately how you build is up to you.
# View the readme at Cute Framework for more info: https://github.com/RandyGaul/cute_framework
include(FetchContent)
FetchContent_Declare(
	cute
	GIT_REPOSITORY https://github.com/RandyGaul/cute_framework
)
FetchContent_MakeAvailable(cute)

add_executable(
	client
	src/client.cpp
)
add_executable(
	server
	src/server.cpp
)

target_include_directories(client PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_include_directories(server PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

target_link_libraries(client cute)
target_link_libraries(server cute)

if (MSVC)
	set_property(TARGET client PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:client>)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT client)
endif()
