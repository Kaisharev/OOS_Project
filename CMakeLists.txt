cmake_minimum_required(VERSION 3.20) 

project(NebojsaKaisarevic_1196_24 
    VERSION 0.1.0 
    LANGUAGES CXX) 

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF) 

# Svi izvršni fajlovi (.exe) će ići direktno u build/bin direktorijum
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) 

# Skupljanje svih izvornih fajlova OSIM main.cpp za core biblioteku
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" 
) 
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "src/main.cpp") 

add_library(project_core STATIC ${PROJECT_SOURCES}) 

# Glavna aplikacija
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

# --- GOOGLE TEST PODESAVANJE ---
include(FetchContent) 
FetchContent_Declare( 
    googletest 
    URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip 
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE 
) 

if (MSVC) 
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) 
endif() 

FetchContent_MakeAvailable(googletest) 

enable_testing() 

# Skupljanje testova i kreiranje test izvršnog fajla
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp" 
) 
add_executable(unit_tests ${TEST_SOURCES}) 
target_link_libraries(unit_tests PRIVATE project_core GTest::gtest_main) 
target_include_directories(unit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests") 

include(GoogleTest) 
gtest_discover_tests(unit_tests) 