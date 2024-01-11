#include <Shaders/OpenGL/Common/Sequence/halton.glsl>

// 'i'th element of the Hammersley set of size 'n'
vec2 hammersley(int i, int n)
{
    return vec2(halton2(i), float(i) / float(n));
}
