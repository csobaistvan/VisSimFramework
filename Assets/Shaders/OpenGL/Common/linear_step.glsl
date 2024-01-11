#include <Shaders/OpenGL/Common/gentype.glsl>

// linstep
#define BUILTIN_LINSTEP(GENTYPE, BASE) \
    GENTYPE linear_step(const GENTYPE low, const GENTYPE high, const GENTYPE x) \
    { return clamp((x - low) / (high - low), GENTYPE(0.0), GENTYPE(1.0)); }

DEF_GENFTYPE(BUILTIN_LINSTEP)
DEF_GENDTYPE(BUILTIN_LINSTEP)

#undef BUILTIN_LINSTEP

// linstep (vector variants)
#define BUILTIN_LINSTEPV(GENTYPE, BASE) \
    GENTYPE linear_step(const BASE low, const BASE high, const GENTYPE x) \
    { return clamp((x - GENTYPE(low)) / GENTYPE(high) - GENTYPE(low), GENTYPE(0.0), GENTYPE(1.0)); }

DEF_GENFVTYPE(BUILTIN_LINSTEPV)
DEF_GENDVTYPE(BUILTIN_LINSTEPV)

#undef BUILTIN_LINSTEPV

// Define an alias for linstep
#define linstep linear_step