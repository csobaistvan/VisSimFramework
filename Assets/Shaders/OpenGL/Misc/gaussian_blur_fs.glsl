#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>

// Input attribus
in VertexData
{
    vec2 vUv;
} fs_in;

// Various texture maps
layout (binding = TEXTURE_POST_PROCESS_1) uniform sampler2D sTexture;

// Blur parameters
layout (location = 0) uniform vec2 vDirection;
layout (location = 1) uniform vec2 vUvScale = vec2(1.0);
layout (location = 2) uniform vec2 vUvMin = vec2(0.0);
layout (location = 3) uniform vec2 vUvMax = vec2(1.0);
layout (location = 4) uniform uint uiNumSamples = 1;
layout (location = 5) uniform vec2 vKernelWeights[NUM_SAMPLES];

// Render targets
layout (location = 0) out vec4 colorBuffer;

//////////////////////////////////////////////////
vec4 filterTap(const float stepSize)
{
    const vec2 uv = vUvMin + vUvScale * fs_in.vUv + stepSize * vDirection;
    return texture(sTexture, clamp(uv, vUvMin, vUvMax));
}

//////////////////////////////////////////////////
// Kernel for linear filtering
vec4 kernel_linear()
{
    vec4 sum = vec4(0);
    for (int i = 0; i < NUM_SAMPLES; ++i)
        sum += vKernelWeights[i].x * filterTap(vKernelWeights[i].y);
    return sum;
}

//////////////////////////////////////////////////
// Kernel for log-space filtering
vec4 kernel_logarithmic()
{
    // Radius & center coordinate
    const int radius = NUM_SAMPLES / 2;

    // Weight and sample for the center
    const float w0 = vKernelWeights[radius].x;
    const vec4 s0 = filterTap(vKernelWeights[radius].y);

    // Compute the summation
    vec4 sum = vec4(0);
    for (int i = 1; i <= radius; ++i)
    {
        sum += vKernelWeights[radius - i].x * exp(filterTap(vKernelWeights[radius - i].y) - s0);
        sum += vKernelWeights[radius + i].x * exp(filterTap(vKernelWeights[radius + i].y) - s0);
    }

    return s0 + log(w0 + sum);
}

//////////////////////////////////////////////////
void main()
{
    colorBuffer = CONCAT(kernel_, FILTER_SPACE)();
}