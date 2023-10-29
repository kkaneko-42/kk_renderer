project(kk_renderer_test)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)
set(SRCS
#[[
    lib_link_test.cpp
    rendering_loop_test.cpp
	draw_triangle_test.cpp
	draw_texture_test.cpp
	draw_model_test.cpp
	editor_test.cpp
	lighting_test.cpp
]]
	skybox_test.cpp
    runner.cpp
)
set(SHADERS_DIR ${kk_renderer_SOURCE_DIR}/resources/shaders)

add_executable(${PROJECT_NAME} ${SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/vulkan/Include)
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/glfw/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/glm/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/stb/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/tiny/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/googletest/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/include)

target_link_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/vulkan/Lib)
target_link_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/glfw/lib)
target_link_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_SOURCE_DIR}/external/googletest/lib)
target_link_directories(${PROJECT_NAME} PRIVATE ${kk_renderer_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE kk_renderer)
if (UNIX)
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
else (UNIX)
    message(FATAL_ERROR "fatal: unknown build platform")
endif (UNIX)

if (MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /MTd)
	# for test purpose
	target_compile_options(${PROJECT_NAME} PRIVATE /D TEST_RESOURCE_DIR="${kk_renderer_SOURCE_DIR}/resources/")
else (MSVC)
	# for test purpose
	target_compile_options(${PROJECT_NAME} PRIVATE -D TEST_RESOURCE_DIR="${kk_renderer_SOURCE_DIR}/resources/")
endif (MSVC)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND
        python
        ${SHADERS_DIR}/compile_shader.py
        ${SHADERS_DIR}
        glslc
)
