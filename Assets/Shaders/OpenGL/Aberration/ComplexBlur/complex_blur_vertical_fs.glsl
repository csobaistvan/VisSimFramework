#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Aberration/ComplexBlur/common.glsl>

// Input attribs
in vec2 vUv;

// Render targets
layout (location = 0) out vec4 colorBuffer;

void main()
{    
    // Output accumulation
    vec2 nearSum[KERNEL_COMPONENTS][3];
    vec2 farSum[KERNEL_COMPONENTS][3];
    for (int i = 0; i < KERNEL_COMPONENTS; ++i) 
    {
        for (int j = 0; j < 3; ++j) 
        {
            nearSum[i][j] = vec2(0.0);
            farSum[i][j] = vec2(0.0);
        }
    }

    // Center blur size
    const vec4 s_c = textureDR(sDownscaledBuffer, vUv, sComplexBlurData.vUvScale);
    const vec3 c_c = s_c.rgb;
    const float r_c = s_c.a;
    const vec2 rd_c = dilatedBlurRadius(gl_FragCoord.xy);
    const float rd_c_near = nearBlurRadius(rd_c);
    const float rd_c_far = farBlurRadius(rd_c);
    const float nearFieldness_c = nearFieldness(r_c);
    const float farFieldness_c = farFieldness(r_c);

    // Calculate the blur step
    //const float gatherRadiusNear = rd_c_near * sComplexBlurData.fEllipseContraction;
    //const float gatherRadiusFar = r_c * sComplexBlurData.fEllipseContraction;
    const float gatherRadiusNear = abs(r_c) * sComplexBlurData.fEllipseContraction;
    const float gatherRadiusFar = rd_c_far * sComplexBlurData.fEllipseContraction;
    const vec2 sampleStepNear = sComplexBlurData.vVerticalDir * getGatherRadius(gatherRadiusNear);
    const vec2 sampleStepFar = sComplexBlurData.vVerticalDir * getGatherRadius(gatherRadiusFar);

    // Near buffer samples for the center
    vec2 blurredValuesNearCenter[KERNEL_COMPONENTS][3];
    vec2 blurredValuesFarCenter[KERNEL_COMPONENTS][3];
    {
    #if KERNEL_COMPONENTS == 1
        const vec4 blurTexture0 = textureDR(sBlurBuffersNear, vUv, sComplexBlurData.vUvScale, 0);
        const vec4 blurTexture1 = textureDR(sBlurBuffersNear, vUv, sComplexBlurData.vUvScale, 1);
        blurredValuesNearCenter[0][0] = blurTexture0.xy;
        blurredValuesNearCenter[0][1] = blurTexture0.zw;
        blurredValuesNearCenter[0][2] = blurTexture1.xy;
    #endif
    #if KERNEL_COMPONENTS == 2
        const vec4 blurTexture0 = textureDR(sBlurBuffersNear, vUv, sComplexBlurData.vUvScale, 0);
        const vec4 blurTexture1 = textureDR(sBlurBuffersNear, vUv, sComplexBlurData.vUvScale, 1);
        const vec4 blurTexture2 = textureDR(sBlurBuffersNear, vUv, sComplexBlurData.vUvScale, 2);
        blurredValuesNearCenter[0][0] = blurTexture0.xy;
        blurredValuesNearCenter[0][1] = blurTexture0.zw;
        blurredValuesNearCenter[0][2] = blurTexture1.xy;
        blurredValuesNearCenter[1][0] = blurTexture1.zw;
        blurredValuesNearCenter[1][1] = blurTexture2.xy;
        blurredValuesNearCenter[1][2] = blurTexture2.zw;
    #endif
    }
    {
    #if KERNEL_COMPONENTS == 1
        const vec4 blurTexture0 = textureDR(sBlurBuffersFar, vUv, sComplexBlurData.vUvScale, 0);
        const vec4 blurTexture1 = textureDR(sBlurBuffersFar, vUv, sComplexBlurData.vUvScale, 1);
        blurredValuesFarCenter[0][0] = blurTexture0.xy;
        blurredValuesFarCenter[0][1] = blurTexture0.zw;
        blurredValuesFarCenter[0][2] = blurTexture1.xy;
    #endif
    #if KERNEL_COMPONENTS == 2
        const vec4 blurTexture0 = textureDR(sBlurBuffersFar, vUv, sComplexBlurData.vUvScale, 0);
        const vec4 blurTexture1 = textureDR(sBlurBuffersFar, vUv, sComplexBlurData.vUvScale, 1);
        const vec4 blurTexture2 = textureDR(sBlurBuffersFar, vUv, sComplexBlurData.vUvScale, 2);
        blurredValuesFarCenter[0][0] = blurTexture0.xy;
        blurredValuesFarCenter[0][1] = blurTexture0.zw;
        blurredValuesFarCenter[0][2] = blurTexture1.xy;
        blurredValuesFarCenter[1][0] = blurTexture1.zw;
        blurredValuesFarCenter[1][1] = blurTexture2.xy;
        blurredValuesFarCenter[1][2] = blurTexture2.zw;
    #endif
    }

    // Perform the convolution
    vec2 blurredValuesNear[KERNEL_COMPONENTS][3];
    vec2 blurredValuesFar[KERNEL_COMPONENTS][3];
    for (int t = -KERNEL_RADIUS_VERTICAL, s = 0; t <= KERNEL_RADIUS_VERTICAL; ++t, ++s)
    {
        // Texture coordinates
        const vec2 texelUVNear = saturate(vUv + t * sampleStepNear);
        const vec2 texelUVFar = saturate(vUv + t * sampleStepFar);

        // Sample blur sizes
        const float rd_s_near = nearBlurRadius(dilatedBlurRadius(texelUVNear * sComplexBlurData.vResolution));
        const float rd_s_far = farBlurRadius(dilatedBlurRadius(texelUVFar * sComplexBlurData.vResolution));
        const float r_s_near = textureDR(sDownscaledBuffer, texelUVNear, sComplexBlurData.vUvScale).a;
        const float r_s_far = textureDR(sDownscaledBuffer, texelUVFar, sComplexBlurData.vUvScale).a;

        // Near buffer samples
        {
        #if KERNEL_COMPONENTS == 1
            const vec4 blurTexture0 = textureDR(sBlurBuffersNear, texelUVNear, sComplexBlurData.vUvScale, 0);
            const vec4 blurTexture1 = textureDR(sBlurBuffersNear, texelUVNear, sComplexBlurData.vUvScale, 1);
            blurredValuesNear[0][0] = blurTexture0.xy;
            blurredValuesNear[0][1] = blurTexture0.zw;
            blurredValuesNear[0][2] = blurTexture1.xy;
        #endif
        #if KERNEL_COMPONENTS == 2
            const vec4 blurTexture0 = textureDR(sBlurBuffersNear, texelUVNear, sComplexBlurData.vUvScale, 0);
            const vec4 blurTexture1 = textureDR(sBlurBuffersNear, texelUVNear, sComplexBlurData.vUvScale, 1);
            const vec4 blurTexture2 = textureDR(sBlurBuffersNear, texelUVNear, sComplexBlurData.vUvScale, 2);
            blurredValuesNear[0][0] = blurTexture0.xy;
            blurredValuesNear[0][1] = blurTexture0.zw;
            blurredValuesNear[0][2] = blurTexture1.xy;
            blurredValuesNear[1][0] = blurTexture1.zw;
            blurredValuesNear[1][1] = blurTexture2.xy;
            blurredValuesNear[1][2] = blurTexture2.zw;
        #endif
        }
        
        // Far buffer samples
        {
        #if KERNEL_COMPONENTS == 1
            const vec4 blurTexture0 = textureDR(sBlurBuffersFar, texelUVFar, sComplexBlurData.vUvScale, 0);
            const vec4 blurTexture1 = textureDR(sBlurBuffersFar, texelUVFar, sComplexBlurData.vUvScale, 1);
            blurredValuesFar[0][0] = blurTexture0.xy;
            blurredValuesFar[0][1] = blurTexture0.zw;
            blurredValuesFar[0][2] = blurTexture1.xy;
        #endif
        #if KERNEL_COMPONENTS == 2
            const vec4 blurTexture0 = textureDR(sBlurBuffersFar, texelUVFar, sComplexBlurData.vUvScale, 0);
            const vec4 blurTexture1 = textureDR(sBlurBuffersFar, texelUVFar, sComplexBlurData.vUvScale, 1);
            const vec4 blurTexture2 = textureDR(sBlurBuffersFar, texelUVFar, sComplexBlurData.vUvScale, 2);
            blurredValuesFar[0][0] = blurTexture0.xy;
            blurredValuesFar[0][1] = blurTexture0.zw;
            blurredValuesFar[0][2] = blurTexture1.xy;
            blurredValuesFar[1][0] = blurTexture1.zw;
            blurredValuesFar[1][1] = blurTexture2.xy;
            blurredValuesFar[1][2] = blurTexture2.zw;
        #endif
        }

        // Calculate the weight of the sample
        const float nearWeight = nearBlurWeight(calcSampleDist(t, gatherRadiusNear), r_c, rd_c_near, r_s_near, rd_s_near) * nearFieldness(r_s_near);
        const float farWeight = farBlurWeight(calcSampleDist(t, gatherRadiusFar), r_c, rd_c_far, r_s_far, rd_s_far) * farFieldness(r_s_far) * farFieldness_c;

        // Convolve the pixel with each kernel component
        for (int c = 0; c < KERNEL_COMPONENTS; ++c)
        {
            const vec2 kernel = verticalKernel(c, s);
            for (int ch = 0; ch < 3; ++ch)
            {
                nearSum[c][ch] += cmpxmul(blurredValuesNear[c][ch], kernel) * nearWeight + cmpxmul(blurredValuesNearCenter[c][ch], kernel) * (1-nearWeight) * nearFieldness_c;
                //farSum[c][ch] += cmpxmul(blurredValuesFar[c][ch], kernel) * farWeight + cmpxmul(blurredValuesFarCenter[c][ch], kernel) * (1-farWeight) * farFieldness_c;
                //farSum[c][ch] += cmpxmul(blurredValuesFar[c][ch], kernel) * farWeight;
                farSum[c][ch] += cmpxmul(blurredValuesFar[c][ch], kernel) * farWeight + cmpxmul(blurredValuesFarCenter[c][ch], kernel) * (1-farWeight) * farFieldness_c;
            }
        }
    }

    // Apply the component weights to get the final near and far results
    vec3 nearResult = vec3(0.0);
    vec3 farResult = vec3(0.0);
    vec3 focusedResult = c_c * (1.0 - saturate(abs(r_c)));
    for (int c = 0; c < KERNEL_COMPONENTS; ++c) 
    for (int ch = 0; ch < 3; ++ch)
    {
        nearResult[ch] += dot(nearSum[c][ch], sComplexBlurWeights.vWeights[c].xy);
        farResult[ch] += dot(farSum[c][ch], sComplexBlurWeights.vWeights[c].xy);
    }
    
    // Construct the final output from the near and far results
    vec3 blurResult = nearResult + farResult + focusedResult;

    // =================================================================================================================

    // Debug outputs
    if (sComplexBlurData.uiOutputMode == OutputMode_BlurRadius)
    {
        const float abs_r = abs(r_c / sComplexBlurData.fMaxBlur);
        blurResult = vec3(gt(r_c, 1) * abs_r, lt(r_c, 0) * abs_r, 0);
    }
    if (sComplexBlurData.uiOutputMode == OutputMode_DilatedBlurRadius)
    {
        const vec2 abs_rd = abs(rd_c / sComplexBlurData.fMaxBlur);
        blurResult = vec3(gt(r_c, 1) * abs_rd.y, lt(r_c, 0) * abs_rd.x, 0);
    }
    if (sComplexBlurData.uiOutputMode == OutputMode_Near)
    {
        blurResult = vec3(nearResult);
    }
    if (sComplexBlurData.uiOutputMode == OutputMode_Far)
    {
        blurResult = vec3(farResult);
    }
    if (sComplexBlurData.uiOutputMode == OutputMode_Focus)
    {
        blurResult = vec3(focusedResult);
    }

    // Output the result
    colorBuffer = vec4(blurResult, 1.0);
}