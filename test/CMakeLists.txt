project(kk_renderer_test)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)
set(SRCS
    lib_link_test.cpp
    rendering_loop_test.cpp
    runner.cpp
)

add_executable(${PROJECT_NAME} ${SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/vulkan/Include)
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/glfw/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/googletest/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/include)
target_link_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}//external/vulkan/Lib)
target_link_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/glfw/lib)
target_link_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/googletest/lib)
target_link_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE kk_renderer)
if (LINUX)
	target_link_libraries(
		${PROJECT_NAME} PRIVATE
		vulkan
		glfw
		gtest
		pthread
	)
elseif (MSVC)
	target_link_libraries(
		${PROJECT_NAME} PRIVATE
		vulkan-1
		glfw3_mt
		gtest
	)
	target_compile_options(${PROJECT_NAME} PRIVATE /MTd)
endif (LINUX)
