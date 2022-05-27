#version 330 core

out vec4 FragColor;

in vec3 color;
in vec2 uv;
in vec3 normal;
in vec4 worldPixel;

uniform vec3 cameraPosition;
uniform vec3 lightDirection;

uniform sampler2D firstDepthPass;

float near = 0.1;
float far = 100.0;

vec3 lerp(vec3 a, vec3 b, float t) {
	return a + (b - a) * t;
}

void main() {
	vec4 depth = texture(firstDepthPass, uv);

	vec3 top = vec3(8 / 255.0, 58 / 255.0, 129 / 255.0);
	vec3 bot = vec3(187 / 255.0, 214 / 255.0, 231 / 255.0);

	float d = distance(worldPixel.xyz, cameraPosition);
	float fogAmount = clamp((d - 100) / 350, 0, 1);

	vec3 waterDepth = lerp(top, bot, depth.r);

	float invertDepth = 1 - depth.r;

	FragColor = vec4(lerp(waterDepth, bot, fogAmount), invertDepth);
}