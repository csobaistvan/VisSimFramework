#version 440

#extension GL_ARB_shader_image_load_store : require

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Mesh/VoxelBasePass/common.glsl>

////////////////////////////////////////////////////////////////////////////////
// GS input
in GeometryData
{
    vec3 vPosition;
    vec3 vPositionCS;
    vec3 vNormal;
    vec2 vUv;
    mat3 mTBN;
    vec3 vVoxelPosition;
    flat vec4 vTriangleAABB;
} fs_in;

////////////////////////////////////////////////////////////////////////////////
// Dummy output
layout (location = 0) out vec4 fragColor;

////////////////////////////////////////////////////////////////////////////////
void main()
{
    // Make sure the triangle is within the screen AABB
    if (fs_in.vPositionCS.x < fs_in.vTriangleAABB.x || fs_in.vPositionCS.y < fs_in.vTriangleAABB.y || 
		fs_in.vPositionCS.x > fs_in.vTriangleAABB.z || fs_in.vPositionCS.y > fs_in.vTriangleAABB.w) 
        discard;
    
    // Compute the view direction for parallax mapping
    //const mat3 invTbn = transpose(fs_in.mTBN);
    //const vec3 viewDirection = normalize(invTbn * sCameraData.vEye - invTbn * fs_in.vPosition);
    const vec3 viewDirection = vec3(0, 0, 1);

    // Evalute the material fn.
    MaterialInfo materialInfo = evaluateMaterialFn(fs_in.vUv, fs_in.vNormal, fs_in.mTBN, viewDirection, gl_FrontFacing);

    // Discard transparent pixels
    if (materialInfo.opacity < 0.02) discard;

    // output texture coords
    const ivec3 gridTexCoords = ivec3(fs_in.vVoxelPosition);

    // average albedo per fragments sorrounding the voxel volume
    const vec3 albedo = materialInfo.albedo * materialInfo.opacity;
    imageAtomicRGBA8Avg(sVoxelAlbedo, gridTexCoords, albedo);

    // average normal per fragments sorrounding the voxel volume
    const vec3 normal = normalize(materialInfo.normal) * 0.5 + 0.5;
    imageAtomicRGBA8Avg(sVoxelNormal, gridTexCoords, normal);
    
    // average the rest of the surface properties
    const vec3 specular = vec3(materialInfo.metallic, materialInfo.roughness, materialInfo.specular);
    imageAtomicRGBA8Avg(sVoxelSpecular, gridTexCoords, specular);
}