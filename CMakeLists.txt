project(kk_renderer)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(SRCS
    src/RenderingContext.cpp
    src/Window.cpp
    src/Swapchain.cpp
	src/Renderer.cpp
	src/Texture.cpp
	src/Buffer.cpp
	src/Geometry.cpp
	src/Vertex.cpp
	src/ResourceDescriptor.cpp
	src/Renderable.cpp
	src/PerspectiveCamera.cpp
	src/Texture.cpp
	src/Shader.cpp
	src/Material.cpp
	src/Image.cpp
	src/Editor.cpp
	src/PipelineBuilder.cpp

	external/imgui/src/imgui.cpp
	external/imgui/src/imgui_impl_glfw.cpp
	external/imgui/src/imgui_impl_vulkan.cpp
	external/imgui/src/imgui_widgets.cpp
	external/imgui/src/imgui_tables.cpp
	external/imgui/src/imgui_draw.cpp
)

add_library(${PROJECT_NAME} STATIC ${SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/vulkan/Include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/glfw/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/glm/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/stb/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/tiny/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/imgui/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)

if (MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /MTd)
	# for test purpose
	target_compile_options(${PROJECT_NAME} PRIVATE /D TEST_RESOURCE_DIR="${PROJECT_SOURCE_DIR}/resources/")
else (MSVC)
	# for test purpose
	target_compile_options(${PROJECT_NAME} PRIVATE -D TEST_RESOURCE_DIR="${PROJECT_SOURCE_DIR}/resources/")
endif (MSVC)

add_subdirectory(test)
