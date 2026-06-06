cmake_minimum_required(VERSION 3.20)

project(NebojsaKaisarevic_1196_24
    VERSION 0.1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(project_core STATIC ${PROJECT_SOURCES})
add_executable(os_app src/main.cpp)
target_link_libraries(os_app PRIVATE project_core)

target_include_directories(project_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(project_core PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(project_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(project_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Ako kasnije dodaš main.cpp, možeš uključiti i izvršni program ovako:
#
# add_executable(os_app src/main.cpp)
# target_link_libraries(os_app PRIVATE project_core)
