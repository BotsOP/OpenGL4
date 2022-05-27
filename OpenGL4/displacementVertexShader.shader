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

uniform sampler2D heightMap;
uniform float time;

void main() {
	//mat4 trs = world * view * projection;
	vec2 newUV = vUV + vec2(0, time * 0.5f);

	vec3 disPos = vPos + (vNormal * texture(heightMap, newUV).r * 0.3f);

	worldPixel = world * vec4(disPos, 1.0f);
	gl_Position = projection * view * world * vec4(disPos, 1.0f);
	color = vColor;
	uv = vUV;
	normal = mat3(world) * vNormal;
}