project(vulkan-playground)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cc")

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(main ${SOURCES})

target_link_libraries(main glfw Vulkan::Vulkan)
