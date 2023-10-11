project(kk_renderer_test)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)
set(SRCS
    lib_link_test.cpp
    rendering_loop_test.cpp
    runner.cpp
)

add_executable(${PROJECT_NAME} ${SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ../external/vulkan/Include)
target_include_directories(${PROJECT_NAME} PRIVATE ../external/glfw/include)
target_include_directories(${PROJECT_NAME} PRIVATE ../external/googletest/include)
target_include_directories(${PROJECT_NAME} PRIVATE ../include)

target_link_directories(${PROJECT_NAME} PRIVATE ../external/vulkan/Lib/linux)
target_link_directories(${PROJECT_NAME} PRIVATE ../external/glfw/lib/linux)
target_link_directories(${PROJECT_NAME} PRIVATE ../external/googletest/lib/linux)
target_link_directories(${PROJECT_NAME} PRIVATE ../build)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    vulkan
    glfw
    gtest
    pthread
    kk_renderer
)
