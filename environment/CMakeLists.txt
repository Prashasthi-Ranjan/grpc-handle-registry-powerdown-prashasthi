cmake_minimum_required(VERSION 3.16)
project(handle_registry_powerdown CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(registry STATIC handle_registry.cc module_sim.cc)
target_include_directories(registry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(registry pthread)

add_executable(test_registry main.cc)
target_link_libraries(test_registry registry pthread)

add_executable(test_concurrency test_concurrency.cc)
target_link_libraries(test_concurrency registry pthread)

add_executable(test_powerdown test_powerdown.cc)
target_link_libraries(test_powerdown registry pthread)

# For TSan check - optional, exclude from default all to avoid build failure on systems without libtsan
add_executable(test_tsan EXCLUDE_FROM_ALL test_concurrency.cc)
target_link_libraries(test_tsan registry pthread)
target_compile_options(test_tsan PRIVATE -fsanitize=thread -g)
target_link_options(test_tsan PRIVATE -fsanitize=thread)
