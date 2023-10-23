#version 450

struct DirectionalLight {
	vec3 dir;
	vec3 color;
	float intensity;
};

layout(set = 0, binding = 0) uniform PerView {
	mat4 view;
	mat4 proj;
	DirectionalLight light;
} perView;

layout(set = 2, binding = 0) uniform PerObject {
	mat4 modelToWorld;
	mat4 worldToModel;
} perObject;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec3 outNorm;
layout(location = 1) out vec2 outUV;

void main() {
	gl_Position = perView.proj * perView.view * perObject.modelToWorld * vec4(inPos, 1.0);
	outNorm = mat3(perView.view) * mat3(perObject.modelToWorld) * inNorm;
	outUV = inUV;
}
