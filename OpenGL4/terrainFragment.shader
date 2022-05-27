#version 330 core

out vec4 FragColor;

in vec3 color;
in vec2 uv;
in vec3 normal;
in vec4 worldPixel;

uniform sampler2D heightMap;
uniform sampler2D normalMap;

uniform sampler2D dirt, sand, grass;

uniform vec3 lightDirection;
uniform vec3 cameraPosition;

vec3 lerp(vec3 a, vec3 b, float t) {
	return a + (b - a) * t;
}

void main() {
	vec3 normalColor = texture(normalMap, uv).rbg * 2 - 1;
	normalColor.b = -normalColor.b;
	normalColor.r = -normalColor.r;

	vec3 lightDir = normalize(lightDirection);
	float light = max(dot(-lightDir, normalColor), .25); 

	vec3 dirtColor = texture(dirt, uv).rgb;
	vec3 sandColor = texture(sand, uv).rgb;
	vec3 grassColor = texture(grass, uv).rgb;

	float ds = clamp((worldPixel.y - 25) / 10, 0, 1);
	float sg = clamp((worldPixel.y - 50) / 10, 0, 1);

	float d = distance(worldPixel.xyz, cameraPosition);
	float fogAmount = clamp((d - 100) / 350, 0, 1);

	vec3 terrainColor = lerp(lerp(dirtColor, sandColor, ds), grassColor, sg);

	vec3 skyColor = vec3(188 / 255.0, 214 / 255.0, 231 / 255.0);

	FragColor = vec4(lerp(terrainColor * light, skyColor, fogAmount), 1.0);
}