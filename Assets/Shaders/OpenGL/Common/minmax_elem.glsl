#include <Shaders/OpenGL/Common/gentype.glsl>

////////////////////////////////////////////////////////////////////////////////
// maxElem
#define BUILTIN_MAXELEM2(GENTYPE, BASE) BASE maxElem(const GENTYPE v){ return max(v.x, v.y); }
#define BUILTIN_MAXELEM3(GENTYPE, BASE) BASE maxElem(const GENTYPE v){ return max(max(v.x, v.y), v.z); }
#define BUILTIN_MAXELEM4(GENTYPE, BASE) BASE maxElem(const GENTYPE v){ return max(max(v.x, v.y), max(v.z, v.w)); }

DEF_GENVEC2(BUILTIN_MAXELEM2)
DEF_GENVEC3(BUILTIN_MAXELEM3)
DEF_GENVEC4(BUILTIN_MAXELEM4)

#undef BUILTIN_MAXELEM2
#undef BUILTIN_MAXELEM3
#undef BUILTIN_MAXELEM4

////////////////////////////////////////////////////////////////////////////////
// minElem
#define BUILTIN_MINELEM2(GENTYPE, BASE) BASE minElem(const GENTYPE v){ return min(v.x, v.y); }
#define BUILTIN_MINELEM3(GENTYPE, BASE) BASE minElem(const GENTYPE v){ return min(min(v.x, v.y), v.z); }
#define BUILTIN_MINELEM4(GENTYPE, BASE) BASE minElem(const GENTYPE v){ return min(min(v.x, v.y), min(v.z, v.w)); }

DEF_GENVEC2(BUILTIN_MINELEM2)
DEF_GENVEC3(BUILTIN_MINELEM3)
DEF_GENVEC4(BUILTIN_MINELEM4)

#undef BUILTIN_MINELEM2
#undef BUILTIN_MINELEM3
#undef BUILTIN_MINELEM4

////////////////////////////////////////////////////////////////////////////////
// maxIndex
#define BUILTIN_MAXINDEX2(GENTYPE, BASE) uint maxIndex(const GENTYPE v){ return v.y > v.x ? 1 : 0; }
#define BUILTIN_MAXINDEX3(GENTYPE, BASE) uint maxIndex(const GENTYPE v){ return v.y > v.x ? ( v.z > v.y ? 2 : 1 ) : ( v.z > v.x ? 2 : 0 ); }

DEF_GENVEC2(BUILTIN_MAXINDEX2)
DEF_GENVEC3(BUILTIN_MAXINDEX3)

#undef BUILTIN_MAXINDEX2
#undef BUILTIN_MAXINDEX3

////////////////////////////////////////////////////////////////////////////////
// minIndex
#define BUILTIN_MININDEX2(GENTYPE, BASE) uint minIndex(const GENTYPE v){ return v.y < v.x ? 1 : 0; }
#define BUILTIN_MININDEX3(GENTYPE, BASE) uint minIndex(const GENTYPE v){ return v.y < v.x ? ( v.z < v.y ? 2 : 1 ) : ( v.z < v.x ? 2 : 0 ); }

DEF_GENVEC2(BUILTIN_MININDEX2)
DEF_GENVEC3(BUILTIN_MININDEX3)

#undef BUILTIN_MININDEX2
#undef BUILTIN_MININDEX3