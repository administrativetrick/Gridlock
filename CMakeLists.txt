cmake_minimum_required(VERSION 3.20)
project(Gridlock LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(MSVC)
  add_compile_options(/W3 /EHsc /permissive- /utf-8)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

file(GLOB SIM_SOURCES CONFIGURE_DEPENDS sim/src/*.cpp)
add_library(gridlock_sim STATIC ${SIM_SOURCES})
target_include_directories(gridlock_sim PUBLIC sim/include)

add_executable(gridlock cli/main.cpp)
target_link_libraries(gridlock PRIVATE gridlock_sim)

add_executable(gridlock_tests tests/test_main.cpp)
target_link_libraries(gridlock_tests PRIVATE gridlock_sim)
target_include_directories(gridlock_tests PRIVATE sim/src)

enable_testing()
add_test(NAME sim_tests COMMAND gridlock_tests)
