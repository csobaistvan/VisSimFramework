#include <Shaders/OpenGL/Generated/resource_indices.glsl>

// Trigonometric form
vec2 cmpxtrig(in float r, in float arg)
{
    return r * vec2(cos(arg), sin(arg));
}

vec2 cmpxtrig(in vec2 z)
{
    return z.x * vec2(cos(z.y), sin(z.y));
}

// Complex conjugate
vec2 cmpxcjg(in vec2 c)
{
	return vec2(c.x, -c.y);
}

// Complex multiplication
vec2 cmpxmul(in vec2 a, in vec2 b)
{
	return vec2(a.x * b.x - a.y * b.y, a.y * b.x + a.x * b.y);
}

// Complex division
vec2 cmpxdiv(in vec2 a, in vec2 b)
{
    return cmpxmul(a, cmpxcjg(b));
}

// Complex magnitude
float cmpxmag(in vec2 c)
{
    return length(c);
}

// Complex absolute value
float cmpxabs(in vec2 c)
{
    return length(c);
}

// Complex squared magnitude
float cmpxmag2(in vec2 c)
{
    return dot(c, c);
}

// Complex squared magnitude
float cmpxabs2(in vec2 c)
{
    return dot(c, c);
}

// Complex argument
float cmpxarg(in vec2 c)
{
    return atan(c.y, c.x);
}

// Complex log function
vec2 cmpxlog(in vec2 c)
{
    return vec2(log(cmpxmag(c)), cmpxarg(c));
}

// Complex exponential function
vec2 cmpxexp(in vec2 c)
{
    return exp(c.x) * vec2(cos(c.y), sin(c.y));
}

// Complex power (scalar power)
vec2 cmpxpow(in vec2 c, float a)
{
	return cmpxexp(a * cmpxlog(c));
}

// Complex power (complex power)
vec2 cmpxpow(in vec2 c, in vec2 b)
{
	return cmpxexp(cmpxmul(b, cmpxlog(c)));
}

// Algebraic to trigonometric conversion
vec2 cmpxtotrig(in vec2 v)
{
    return vec2(cmpxmag(v), cmpxarg(v));
}

// Linear interpolation
vec2 cmpxlerp(in vec2 a, in vec2 b, in float t)
{
    return lerp(a, b, vec2(t));
}

// Non-linear interpolation
vec2 cmpxclerp(in vec2 a, in vec2 b, in float t)
{
    return cmpxtrig
    (
        lerp(cmpxmag(a), cmpxmag(b), t), 
        lerp(cmpxarg(a), cmpxarg(b), t)
    );
}