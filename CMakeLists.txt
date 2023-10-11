project(kk_renderer)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(SRCS
    src/RenderingContext.cpp
    src/Window.cpp
	src/SwapChain.cpp
)

add_library(${PROJECT_NAME} STATIC ${SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/vulkan/Include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/glfw/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)

if (MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /MTd)
endif (MSVC)

add_subdirectory(test)
