#include <Shaders/OpenGL/Common/gentype.glsl>

// linstep
#define BUILTIN_SWAP(GENTYPE, BASE) \
    void swap(inout GENTYPE a, inout GENTYPE b) { const GENTYPE tmp = a; a = b; b = tmp; }

DEF_GENFTYPE(BUILTIN_SWAP)
DEF_GENDTYPE(BUILTIN_SWAP)
DEF_GENITYPE(BUILTIN_SWAP)
DEF_GENUTYPE(BUILTIN_SWAP)
DEF_GENBTYPE(BUILTIN_SWAP)
DEF_GENFMAT(BUILTIN_SWAP)
DEF_GENDMAT(BUILTIN_SWAP)

#undef BUILTIN_SWAP