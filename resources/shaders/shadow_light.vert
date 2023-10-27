#version 450

layout(set = 0, binding = 0) uniform PerView {
	mat4 view;
	mat4 proj;
	mat4 light_view;
	mat4 light_proj;
	vec3 light_pos;
	vec3 light_dir;
	vec3 light_color;
	float light_intensity;
}	perView;

layout(set = 2, binding = 0) uniform PerObject {
	mat4 model;
}	perObject;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec3 outNorm;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec4 outPosOnShadowCoord;

const mat4 bias = mat4(
	vec4(0.5, 0.0, 0.0, 0.0),
	vec4(0.0, 0.5, 0.0, 0.0),
	vec4(0.0, 0.0, 1.0, 0.0),
	vec4(0.5, 0.5, 0.0, 1.0)
);

void main() {
	gl_Position = perView.proj * perView.view * perObject.model * vec4(inPos, 1.0);
	outNorm = mat3(perView.view) * mat3(perObject.model) * inNorm;
	outUV = inUV;
	outPosOnShadowCoord = bias * perView.light_proj * perView.light_view * perObject.model * vec4(inPos, 1.0);
}
