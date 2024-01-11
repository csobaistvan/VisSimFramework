#include <Shaders/OpenGL/Common/gentype.glsl>

// saturate
#define BUILTIN_SATURATE(GENTYPE, BASE) \
    GENTYPE saturate(const GENTYPE v) { return clamp(v, GENTYPE(0.0), GENTYPE(1.0)); }

DEF_GENFTYPE(BUILTIN_SATURATE)
DEF_GENDTYPE(BUILTIN_SATURATE)
DEF_GENITYPE(BUILTIN_SATURATE)
DEF_GENUTYPE(BUILTIN_SATURATE)

#undef BUILTIN_SATURATE

// lerp
#define BUILTIN_LERP(GENTYPE, BASE) \
    GENTYPE lerp(const GENTYPE a, const GENTYPE b, const GENTYPE t) { return mix(a, b, t); }

DEF_GENFTYPE(BUILTIN_LERP)
DEF_GENDTYPE(BUILTIN_LERP)

#undef BUILTIN_LERP

// lerp (vector variants)
#define BUILTIN_LERPV(GENTYPE, BASE) \
    GENTYPE lerp(const GENTYPE a, const GENTYPE b, const BASE t) { return mix(a, b, t); }

DEF_GENFVTYPE(BUILTIN_LERPV)
DEF_GENDVTYPE(BUILTIN_LERPV)

#undef BUILTIN_LERPV

// mad
#define BUILTIN_MAD(GENTYPE, BASE) \
    GENTYPE mad(const GENTYPE m, const GENTYPE a, const GENTYPE b) { return m * a + b; }
    
DEF_GENFTYPE(BUILTIN_MAD)
DEF_GENDTYPE(BUILTIN_MAD)

#undef BUILTIN_MAD