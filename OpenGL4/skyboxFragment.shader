#version 330 core

out vec4 FragColor;

in vec4 worldPixel;

uniform vec3 cameraPosition;
uniform vec3 lightDirection;

vec3 lerp(vec3 a, vec3 b, float t) {
	return a + (b - a) * t;
}

void main() {
	vec3 lightDir = normalize(lightDirection);
	vec3 viewDirection = normalize(worldPixel.xyz - cameraPosition);

	vec3 top = vec3(68 / 255.0, 118 / 255.0, 189 / 255.0);
	vec3 bot = vec3(187 / 255.0, 214 / 255.0, 231 / 255.0);

	float sun = pow(max( dot(-viewDirection, lightDir), 0.0), 256);

	FragColor = vec4(lerp(bot, top, viewDirection.y) + sun * vec3(0.85, 0.55, 0.15), 1.0);
}