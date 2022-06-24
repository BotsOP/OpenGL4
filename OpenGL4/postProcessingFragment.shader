#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D noiseTexture;
uniform sampler2D ditherRampTexture;

void main()
{
    //FragColor = texture(screenTexture, TexCoords);
    //FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);

    vec3 col = texture(screenTexture, TexCoords).rgb;
    float lum = dot(col, vec3(0.299f, 0.587f, 0.114f));

    vec2 noiseUV = TexCoords * vec2(1600 / 16, 1200 / 16);
    vec3 threshold = texture(noiseTexture, noiseUV).rgb;
    float thresholdLum = dot(threshold, vec3(0.299f, 0.587f, 0.114f));

    float rampVal;

    if (lum < thresholdLum)
    {
        rampVal = thresholdLum - lum;
    }
    else
    {
        rampVal = 1.0;
    }

    vec3 rgb = texture(ditherRampTexture, vec2(rampVal, 0.5)).rgb;

    FragColor = vec4(rgb, 1.0);
}