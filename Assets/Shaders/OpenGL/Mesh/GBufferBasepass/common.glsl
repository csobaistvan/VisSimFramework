#include <Shaders/OpenGL/Common/Optional/material.glsl>
#include <Shaders/OpenGL/Common/Optional/depth_peel.glsl>

// Model uniforms
layout (location = 16) uniform mat4 mModel;
layout (location = 17) uniform mat4 mNormal;
layout (location = 18) uniform mat4 mPrevModel;
layout (location = 19) uniform mat4 mPrevNormal;
layout (location = 20) uniform float fMeshToUV;

// The previous depth texture
layout (binding = TEXTURE_DEPTH) uniform sampler2DArray sPrevDepthBuffer;