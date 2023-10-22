#version 450

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

// TODO: uniform
const vec3 ambientColor = vec3(1.0);
const float ambientStrength = 0.3;
// const vec3 lightPos = vec3(3.0, -3.0, -3.0);
const vec3 lightPos = vec3(0.0, 0.0, -3.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float lightStrength = 2.0f;

void main() {
	vec3 ambient = ambientStrength * ambientColor;

	vec3 norm = normalize(fragNorm);
	vec3 lightDir = normalize(lightPos - fragPos);
	vec3 diffuse = lightStrength * max(dot(norm, lightDir), 0.0) * lightColor;

	// outColor = vec4(fragNorm * ambient, 1.0);
	outColor = vec4((ambient + diffuse), 1.0) * texture(texSampler, fragUV);
}
