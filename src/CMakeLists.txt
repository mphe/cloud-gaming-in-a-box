cmake_minimum_required(VERSION 3.1)

project(CloudGamingInABox)

# Set default build type to Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compile flags
if (UNIX)
    add_definitions(
        -Wall
        -Wextra
        -Wsuggest-override
        -Wsuggest-final-methods
        -Wsuggest-final-types
        -Werror=suggest-override
        -Werror=return-type
        -Wmaybe-uninitialized
        -fdiagnostics-color=always
        )
elseif (MSVC)
    # Untested
    add_definitions(/W3)
endif()

# Find SDL2
find_package(SDL2 REQUIRED)

# Find libav
# https://stackoverflow.com/questions/50760024/cmake-configuration-for-ffmpeg-in-c-project
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h REQUIRED)
find_library(AVCODEC_LIBRARY avcodec REQUIRED)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h REQUIRED)
find_library(AVFORMAT_LIBRARY avformat REQUIRED)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h REQUIRED)
find_library(AVUTIL_LIBRARY avutil REQUIRED)

# Define executables
add_library(shared STATIC
    network/socket.cpp
    network/input.cpp
    )
target_include_directories(shared PRIVATE ${PROJECT_SOURCE_DIR})

add_executable(syncinput
    syncinput/syncinput.cpp
    )
target_include_directories(syncinput PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    )
target_link_libraries(syncinput PRIVATE shared)

add_executable(frontend
    frontend/frontend.cpp
    frontend/av.cpp
    frontend/ui.cpp
    )

target_include_directories(frontend SYSTEM PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIR}
    ${AVFORMAT_INCLUDE_DIR}
    ${AVUTIL_INCLUDE_DIR}
    )

target_link_libraries(frontend PRIVATE
    shared
    ${SDL2_LIBRARIES}
    ${AVCODEC_LIBRARY}
    ${AVFORMAT_LIBRARY}
    ${AVUTIL_LIBRARY}
    )

# Platform specific libraries
if (UNIX)
    # Add X11 sources to syncinput
    target_sources(syncinput PRIVATE syncinput/input_sender/xorg.cpp)

    # Include and link X11
    find_package(X11 REQUIRED)
    target_include_directories(syncinput SYSTEM PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(syncinput PRIVATE ${X11_LIBRARIES} -lXtst)
elseif (WIN32)
    # TODO: Implement and add windows sources to syncinput
endif()
