#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Mesh/GBufferBasepass/common.glsl>

// Input attribus
in GeometryData
{
    vec3 vPosition;
    vec3 vPrevPosition;
    vec3 vNormal;
    vec2 vUv;
    mat3 mTBN;
} fs_in;

// Render targets
layout (location = 0) out vec4 colorBuffer;
layout (location = 1) out vec4 normalBuffer;
layout (location = 2) out vec4 specularBuffer;

////////////////////////////////////////////////////////////////////////////////
void main()
{
    // Discard the current fragment if it doesn't survive the depth peeling process
    // TODO: proper MSAA support
    if (gl_Layer > 0 && !depthPeel(sPrevDepthBuffer, fs_in.vPosition, fs_in.vPrevPosition, gl_Layer - 1))
        discard;

    // Compute the view direction for parallax mapping
    const mat3 invTbn = transpose(fs_in.mTBN);
    const vec3 viewDirection = normalize(invTbn * sCameraData.vEye - invTbn * fs_in.vPosition);

    // Compute the velocity
    const vec4 currPositionCS = sCameraData.mViewProjection * vec4(fs_in.vPosition, 1);
    const vec4 prevPositionCS = sCameraData.mPrevViewProjection * vec4(fs_in.vPrevPosition, 1);
    const vec2 velocity = (currPositionCS.xy / currPositionCS.w) - (prevPositionCS.xy / prevPositionCS.w);

    // Evalute the material fn.
    MaterialInfo materialInfo = evaluateMaterialFn(fs_in.vUv, fs_in.vNormal, fs_in.mTBN, viewDirection, gl_FrontFacing);
    
    // Discard transparent pixels
    if (materialInfo.opacity < 0.01)
        discard;
   
    // Write out the gbuffer data
    colorBuffer = vec4(materialInfo.albedo, 1.0);
    normalBuffer = vec4(materialInfo.normal, materialInfo.specular);
    specularBuffer = vec4(materialInfo.metallic, materialInfo.roughness, velocity);
}