#version 450

layout(set = 0, binding = 0) uniform PerView {
	mat4 view;
	mat4 proj;
	vec3 light_pos;
	vec3 light_dir;
	vec3 light_color;
	float light_intensity;
} perView;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 2, binding = 0) uniform PerObject {
	mat4 modelToWorld;
	mat4 worldToModel;
} perObject;

layout(location = 0) in vec3 inNorm;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColor;

// TODO: uniform
const vec3 ambientColor = vec3(1.0);
const float ambientStrength = 0.3;
const float specularStrength = 0.5;

void main() {
	// Calc in camera space
	vec3 normal = normalize(inNorm);
	vec3 lightDir = normalize(mat3(perView.view) * perView.light_dir);
	vec3 lightReflectDir = reflect(lightDir, normal);
	vec3 cameraDir = vec3(0, 0, 1.0);

	vec3 ambient = ambientStrength * ambientColor;
	vec3 diffuse = max(-dot(normal, lightDir), 0.0) * perView.light_color;
	vec3 specular = specularStrength * pow(max(dot(cameraDir, lightReflectDir), 0.0), 32) * perView.light_color;

	outColor = vec4((ambient + diffuse + specular), 1.0) * texture(texSampler, inUV);
}
