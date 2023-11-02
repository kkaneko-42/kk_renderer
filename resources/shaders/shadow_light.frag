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
} perView;

layout(set = 0, binding = 1) uniform sampler2D shadowSampler;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 2, binding = 0) uniform PerObject {
	mat4 modelToWorld;
	mat4 worldToModel;
} perObject;

layout(location = 0) in vec3 inNorm;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inPosOnShadowCoord;

layout(location = 0) out vec4 outColor;

// TODO: uniform
const vec3 ambientColor = vec3(1.0);
const float ambientStrength = 0.3;

float CalcShadow() {
	float closestDepth = texture(shadowSampler, inPosOnShadowCoord.xy).r;
	float currentDepth = inPosOnShadowCoord.z;
	return (currentDepth > closestDepth) ? 0.5 : 1.0;
}

void main() {
	// NOTE: Calc in camera space
	vec3 normal = normalize(inNorm);
	vec3 lightDir = normalize(mat3(perView.view) * perView.light_dir);
	vec3 lightReflectDir = reflect(lightDir, normal);
	vec3 cameraDir = vec3(0, 0, 1.0);

	// Ambient
	vec3 ambient = ambientStrength * ambientColor;
	
	// Diffuse
	float diff = max(-dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * perView.light_color;
	
	// Specular
	float spec = pow(max(dot(cameraDir, lightReflectDir), 0.0), 32);
	vec3 specular = spec * perView.light_color;
	
	// Shadow
	float shadow = CalcShadow();
	
	vec3 color = texture(texSampler, inUV).rgb;
	vec3 lighting = (ambient + shadow * (diffuse + specular)) * color;
	outColor = vec4(lighting, 1.0);
}
