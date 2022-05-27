#version 330 core

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec3 vNormal;

uniform mat4 world, view, projection;

out vec3 color;
out vec2 uv;
out vec3 normal;
out vec4 worldPixel;

void main() {
	worldPixel = world * vec4(vPos, 1.0f);
	gl_Position = projection * view * world * vec4(vPos, 1.0f);
	color = vColor;
	uv = vUV;
	normal = mat3(world) * vNormal;
}