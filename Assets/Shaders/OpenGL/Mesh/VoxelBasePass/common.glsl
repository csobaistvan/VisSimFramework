
////////////////////////////////////////////////////////////////////////////////
#include <Shaders/OpenGL/Common/Optional/material.glsl>
#include <Shaders/OpenGL/Shading/Voxel/common.glsl>

////////////////////////////////////////////////////////////////////////////////
// Model uniforms
layout (location = 16) uniform mat4 mModel;
layout (location = 17) uniform mat4 mNormal;
layout (location = 18) uniform float fMeshToUV;

// Voxel uniforms
layout (location = 19) uniform mat4 mVoxelViewProjections[3];
layout (location = 22) uniform mat4 mInverseVoxelViewProjections[3];

////////////////////////////////////////////////////////////////////////////////
// Output textures
layout(binding = 0, r32ui) uniform volatile coherent uimage3D sVoxelAlbedo;
layout(binding = 1, r32ui) uniform volatile coherent uimage3D sVoxelNormal;
layout(binding = 2, r32ui) uniform volatile coherent uimage3D sVoxelSpecular;

////////////////////////////////////////////////////////////////////////////////
// Atomic RGBA ops
//void imageAtomicRGBA8Avg(layout(r32ui) volatile coherent uimage3D grid, const ivec3 coords, vec3 value)
//{
//    vec4 valueStore = vec4(value * 255.0, 1.0);
//    uint newVal = packFloat4x8(valueStore);
//    uint prevStoredVal = 0;
//    uint curStoredVal;
//    uint numIterations = 0;
//
//    while((curStoredVal = imageAtomicCompSwap(grid, coords, prevStoredVal, newVal)) != prevStoredVal && numIterations < 255)
//    {
//        prevStoredVal = curStoredVal;
//        vec4 rval = unpackFloat4x8(curStoredVal);
//        rval.rgb = (rval.rgb * rval.a);     // Denormalize
//        vec4 curValF = rval + valueStore;   // Add
//        curValF.rgb /= curValF.a;           // Renormalize
//        newVal = packFloat4x8(curValF);
//
//        ++numIterations;
//    }
//}

#define imageAtomicRGBA8Avg(grid, coords, value) \
{ \
    vec4 valueStore = vec4(value * 255.0, 1.0); \
    uint newVal = packFloat4x8(valueStore); \
    uint prevStoredVal = 0; \
    uint curStoredVal; \
    uint numIterations = 0; \
    while((curStoredVal = imageAtomicCompSwap(grid, coords, prevStoredVal, newVal)) != prevStoredVal && numIterations < 255) \
    { \
        prevStoredVal = curStoredVal; \
        vec4 rval = unpackFloat4x8(curStoredVal); \
        rval.rgb = (rval.rgb * rval.a); \
        vec4 curValF = rval + valueStore; \
        curValF.rgb /= curValF.a; \
        newVal = packFloat4x8(curValF); \
        ++numIterations; \
    } \
} \