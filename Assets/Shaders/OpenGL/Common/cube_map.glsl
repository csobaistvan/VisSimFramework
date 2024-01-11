#include <Shaders/OpenGL/Common/minmax_elem.glsl>

////////////////////////////////////////////////////////////////////////////////
// Index of the cube map face
//
// 0: -X  2: -Y  4: -Z
// 1: +X  3: +Y  5: +Z
uint cubeMapFaceId(const vec3 position)
{
    const uint id = maxIndex(abs(position));
    return position[id] <= 0.0 ? id * 2 : id * 2 + 1;
}

////////////////////////////////////////////////////////////////////////////////
vec3 cubeMapFaceColor(const uint faceId)
{
    switch (faceId)
    {
        case 0: return vec3(1, 0, 0);
        case 1: return vec3(1, 1, 0);
        case 2: return vec3(0, 1, 0);
        case 3: return vec3(0, 1, 1);
        case 4: return vec3(0, 0, 1);
        case 5: return vec3(1, 0, 1);
    }

    return vec3(0.0);
}