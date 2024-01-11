#include <Shaders/OpenGL/Common/gentype.glsl>

////////////////////////////////////////////////////////////////////////////////
// Barycentric coordinates determination functions
////////////////////////////////////////////////////////////////////////////////

// Compute the barycentric coordinates of a 2D points inside a triangle
vec3 getBarycentricCoordinates(const vec2 p, const vec2 a, const vec2 b, const vec2 c)
{
    const vec2 v0 = b - a, v1 = c - a, v2 = p - a;
    const float d00 = dot(v0, v0);
    const float d01 = dot(v0, v1);
    const float d11 = dot(v1, v1);
    const float d20 = dot(v2, v0);
    const float d21 = dot(v2, v1);
    const float denom = d00 * d11 - d01 * d01;

    vec3 result;

    result.y = (d11 * d20 - d01 * d21) / denom;
    result.z = (d00 * d21 - d01 * d20) / denom;
    result.x = 1.0f - result.y - result.z;

    return result;
}

// Determines if the point corresponding to the parameter barycentric coordinates is inside the triangle
bool isBarycentricInside(const vec3 barycentric)
{
    return (all(greaterThanEqual(barycentric, vec3(0.0))) && all(lessThanEqual(barycentric, vec3(1.0))));
}

////////////////////////////////////////////////////////////////////////////////
// Barycentric coordinates interpolation functions
////////////////////////////////////////////////////////////////////////////////

// define barycentric interpolation functions
#define LERP_BARYCENTRIC(GENTYPE, BASE) \
    GENTYPE lerpBarycentric(const vec3 barycentricCoords, const GENTYPE a, const GENTYPE b, const GENTYPE c) \
    { return GENTYPE(barycentricCoords.x) * a + GENTYPE(barycentricCoords.y) * b + GENTYPE(barycentricCoords.z) * c; } \

DEF_GENFTYPE(LERP_BARYCENTRIC)
DEF_GENDTYPE(LERP_BARYCENTRIC)

#undef LERP_BARYCENTRIC
