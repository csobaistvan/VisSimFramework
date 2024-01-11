#include <Shaders/OpenGL/Shading/Deferred/gbuffer.glsl>

// FXAA uniform buffer
layout (std140, binding = UNIFORM_BUFFER_GENERIC_1) uniform FxaaData
{
    float fDirReduceMin;
    float fDirReduceMultiplier;
    float fMaxBlur;
} sFxaaData;