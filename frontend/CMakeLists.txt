cmake_minimum_required(VERSION 3.1)

project(frontend)

# Set default build type to Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

add_executable(frontend frontend.cpp av.cpp ui.cpp)
target_link_libraries(frontend avformat avcodec avutil SDL2main SDL2)
