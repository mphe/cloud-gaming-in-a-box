cmake_minimum_required(VERSION 3.1)

project(syncinput)

# Set default build type to Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(syncinput syncinput.cpp event.cpp tcp.cpp)

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

    add_executable(xorg_test xorg_test_main.cpp xorg.cpp)

    target_sources(syncinput PRIVATE xorg.cpp)
    target_sources(xorg_test PRIVATE xorg.cpp)

    # x11
    find_package(X11 REQUIRED)
    include_directories(SYSTEM ${X11_INCLUDE_DIR})
    target_link_libraries(syncinput ${X11_LIBRARIES} -lXtst)
    target_link_libraries(xorg_test  ${X11_LIBRARIES} -lXtst)

elseif (MSVC)
    # Untested
    add_definitions(/W3)
endif()
