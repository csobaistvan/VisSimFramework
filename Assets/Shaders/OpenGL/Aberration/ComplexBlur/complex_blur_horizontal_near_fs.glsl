#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/ComplexBlur/common.glsl>

// Input attribs
in vec2 vUv;

// Render targets
layout (location = 0) out vec4 colorBuffers[8];

void main()
{
    // Output accumulation
    vec2 blurSum[KERNEL_COMPONENTS][3];
    vec3 sampleSum = vec3(0.0);
    for (int i = 0; i < KERNEL_COMPONENTS; ++i) 
    {
        for (int j = 0; j < 3; ++j) 
        {
            blurSum[i][j] = vec2(0.0);
        }
    }

    // Center blur size
    const vec4 s_c = textureDR(sDownscaledBuffer, vUv, sComplexBlurData.vUvScale);
    const vec3 c_c = s_c.rgb;
    const float r_c = s_c.a;
    const float rd_c = nearBlurRadius(dilatedBlurRadius(gl_FragCoord.xy));
    const float nearFieldness_c = nearFieldness(r_c);
    const float farFieldness_c = farFieldness(r_c);

    // Calculate the blur step
    //const float gatherRadius = rd_c;
    const float gatherRadius = abs(r_c);
    const vec2 sampleStep = sComplexBlurData.vHorizontalDir * getGatherRadius(gatherRadius);
    
    // Perform the convolution
    for (int t = -KERNEL_RADIUS_HORIZONTAL, s = 0; t <= KERNEL_RADIUS_HORIZONTAL; ++t, ++s)
    {
        // Sample the scene buffers
        const vec2 texelUV = saturate(vUv + t * sampleStep);
        const vec4 texel = textureDR(sDownscaledBuffer, texelUV, sComplexBlurData.vUvScale);
        const float rd_s = nearBlurRadius(dilatedBlurRadius(texelUV * sRenderData.vResolution));
        const float r_s = texel.a;
        const vec3 c_s = texel.rgb;

        // Calculate the weight of the sample
        const float weight = nearBlurWeight(calcSampleDist(t, gatherRadius), r_c, rd_c, r_s, rd_s) * nearFieldness(r_s);

        // Accum the sample weights
        sampleSum += c_s;

        // Convolve the pixel with each kernel component
        for (int c = 0; c < KERNEL_COMPONENTS; ++c)
        {
            const vec2 kernel = horizontalKernel(c, s);            
            for (int ch = 0; ch < 3; ++ch)
            {
                blurSum[c][ch] += c_s[ch] * weight * kernel + c_c[ch] * (1-weight) * kernel * nearFieldness_c;
            }
        }
    }
    
    // Write out the results
    #if KERNEL_COMPONENTS == 1
        colorBuffers[0] = vec4(blurSum[0][0].xy, blurSum[0][1].xy);
        colorBuffers[1] = vec4(blurSum[0][2].xy, 0.0, 0.0);
    #endif
    #if KERNEL_COMPONENTS == 2
        colorBuffers[0] = vec4(blurSum[0][0].xy, blurSum[0][1].xy);
        colorBuffers[1] = vec4(blurSum[0][2].xy, blurSum[1][0].xy);
        colorBuffers[2] = vec4(blurSum[1][1].xy, blurSum[1][2].xy);
    #endif
}