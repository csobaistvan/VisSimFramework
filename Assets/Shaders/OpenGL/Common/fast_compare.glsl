#include <Shaders/OpenGL/Generated/resource_indices.glsl>

//  Branch eliminators
float eq(float x, float y)
{
    return 1.0-abs(sign(x-y));
}

vec2 eq(vec2 x, vec2 y)
{
    return 1.0-abs(sign(x-y));
}

vec3 eq(vec3 x, vec3 y)
{
    return 1.0-abs(sign(x-y));
}

vec4 eq(vec4 x, vec4 y)
{
    return 1.0-abs(sign(x-y));
}

float neq(float x, float y)
{
    return abs(sign(x-y));
}

vec2 neq(vec2 x, vec2 y)
{
    return abs(sign(x-y));
}

vec3 neq(vec3 x, vec3 y)
{
    return abs(sign(x-y));
}

vec4 neq(vec4 x, vec4 y)
{
    return abs(sign(x-y));
}

float gt(float x, float y)
{
    return max(sign(x-y),0.0);
}

vec2 gt(vec2 x, vec2 y)
{
    return max(sign(x-y),0.0);
}

vec3 gt(vec3 x, vec3 y)
{
    return max(sign(x-y),0.0);
}

vec4 gt(vec4 x, vec4 y)
{
    return max(sign(x-y),0.0);
}

float lt(float x, float y)
{
    return step(x,y);
}

vec2 lt(vec2 x, vec2 y)
{
    return step(x,y);
}

vec3 lt(vec3 x, vec3 y)
{
    return step(x,y);
}

vec4 lt(vec4 x, vec4 y)
{
    return step(x,y);
}

float ge(float x, float y)
{
    return 1.0-step(x,y);
}

vec2 ge(vec2 x, vec2 y)
{
    return 1.0-step(x,y);
}

vec3 ge(vec3 x, vec3 y)
{
    return 1.0-step(x,y);
}

vec4 ge(vec4 x, vec4 y)
{
    return 1.0-step(x,y);
}

float le(float x, float y)
{
    return 1.0-gt(x,y);
}

vec2 le(vec2 x, vec2 y)
{
    return 1.0-gt(x,y);
}

vec3 le(vec3 x, vec3 y)
{
    return 1.0-gt(x,y);
}

vec4 le(vec4 x, vec4 y)
{
    return 1.0-gt(x,y);
}

float and(float a, float b)
{
    return a*b;
}

vec2 and(vec2 a, vec2 b)
{
    return a*b;
}

vec3 and(vec3 a, vec3 b)
{
    return a*b;
}

vec4 and(vec4 a, vec4 b)
{
    return a*b;
}

float or(float a, float b)
{
    return min(a+b,1.0);
}

vec2 or(vec2 a, vec2 b)
{
    return min(a+b,1.0);
}

vec3 or(vec3 a, vec3 b)
{
    return min(a+b,1.0);
}

vec4 or(vec4 a, vec4 b)
{
    return min(a+b,1.0);
}