#version 450

layout(set = 0, binding = 0) uniform PerView {
	mat4 view;
	mat4 proj;
} perView;

layout(set = 1, binding = 0) uniform PerObject {
	mat4 model;
}	perObject;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

void main() {
	gl_Position = perView.proj * perView.view * perObject.model * vec4(inPos, 1.0);
}
