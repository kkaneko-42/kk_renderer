#version 450

layout(set = 1, binding = 0) uniform UBO {
	mat4 mvp;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
	gl_Position = ubo.mvp * vec4(inPos, 1.0);
	fragColor = inColor;
}
