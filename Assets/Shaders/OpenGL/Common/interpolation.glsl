#include <Shaders/OpenGL/Common/gentype.glsl>

// Linear
#define BUILTIN_LINEAR_INTERPOLATION(GENTYPE, BASE) \
    GENTYPE linearInterpolation(const GENTYPE a, const GENTYPE b, const GENTYPE t) \
    { \
        return mix(a, b, t); \
    } \

DEF_GENFTYPE(BUILTIN_LINEAR_INTERPOLATION)
DEF_GENDTYPE(BUILTIN_LINEAR_INTERPOLATION)

#undef BUILTIN_LINEAR_INTERPOLATION

// Cosine
#define BUILTIN_COSINE_INTERPOLATION(GENTYPE, BASE) \
    GENTYPE cosineInterpolation(const GENTYPE a, const GENTYPE b, const GENTYPE t) \
    { \
        GENTYPE ft = t * 3.1415927; \
        GENTYPE f = (1 - cos(ft)) * 0.5; \
        return a * (1 - f) + b * f; \
    } \

DEF_GENFTYPE(BUILTIN_COSINE_INTERPOLATION)

#undef BUILTIN_COSINE_INTERPOLATION