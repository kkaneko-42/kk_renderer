#version 450

layout(set = 0, binding = 0) uniform PerView {
	mat4 view;
	mat4 proj;
}	perView;

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 outUV;

void main() {
	outUV = inPos;
	outUV.y *= -1;
	gl_Position	= (perView.proj * mat4(mat3(perView.view)) * vec4(inPos, 1.0)).xyww; 
}
