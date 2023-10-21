#version 450

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNorm;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

// TODO: uniform
const vec3 ambientColor = vec3(1.0);
const float ambientStrength = 0.3;
const vec3 lightPos = vec3(3.0, -3.0, -3.0);

void main() {
	vec3 ambient = ambientStrength * ambientColor;

	outColor = vec4(fragNorm * ambient, 1.0);
}
