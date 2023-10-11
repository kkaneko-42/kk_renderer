project(kk_renderer)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(SRCS
    src/RenderingContext.cpp
)

add_library(${PROJECT_NAME} STATIC ${SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ./external/vulkan/Include)
target_include_directories(${PROJECT_NAME} PRIVATE ./external/glfw/include)
target_include_directories(${PROJECT_NAME} PRIVATE ./include)
