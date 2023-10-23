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

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 2, binding = 0) uniform PerObject {
	mat4 modelToWorld;
	mat4 worldToModel;
} perObject;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

// TODO: uniform
const vec3 ambientColor = vec3(1.0);
const float ambientStrength = 0.3;

void main() {
	vec3 normal = normalize(fragNorm);
	vec3 lightDir = normalize(perView.light.dir);

	vec3 ambient = ambientStrength * ambientColor;
	vec3 diffuse = perView.light.color * max(-dot(normal, lightDir), 0.0);
	// outColor = vec4((ambient + diffuse), 1.0) * texture(texSampler, fragUV);
	outColor = vec4(diffuse, 1.0) * texture(texSampler, fragUV);
}
