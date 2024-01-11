// Macro for generating generic int definitions
#define DEF_GENITYPE(BODY) \
    BODY(int, int) \
    BODY(ivec2, int) \
    BODY(ivec3, int) \
    BODY(ivec4, int) \

// Macro for generating generic int definitions (vector only)
#define DEF_GENIVTYPE(BODY) \
    BODY(ivec2, int) \
    BODY(ivec3, int) \
    BODY(ivec4, int) \

// Macro for generating generic uint definitions
#define DEF_GENUTYPE(BODY) \
    BODY(uint, uint) \
    BODY(uvec2, uint) \
    BODY(uvec3, uint) \
    BODY(uvec4, uint) \

// Macro for generating generic uint definitions (vector only)
#define DEF_GENUVTYPE(BODY) \
    BODY(uvec2, uint) \
    BODY(uvec3, uint) \
    BODY(uvec4, uint) \

// Macro for generating generic bool definitions
#define DEF_GENBTYPE(BODY) \
    BODY(bool, bool) \
    BODY(bvec2, bool) \
    BODY(bvec3, bool) \
    BODY(bvec4, bool) \

// Macro for generating generic bool definitions (vector only)
#define DEF_GENBVTYPE(BODY) \
    BODY(bvec2, bool) \
    BODY(bvec3, bool) \
    BODY(bvec4, bool) \

// Macro for generating generic float definitions
#define DEF_GENFTYPE(BODY) \
    BODY(float, float) \
    BODY(vec2, float) \
    BODY(vec3, float) \
    BODY(vec4, float) \

// Macro for generating generic float definitions (vector only)
#define DEF_GENFVTYPE(BODY) \
    BODY(vec2, float) \
    BODY(vec3, float) \
    BODY(vec4, float) \

// Macro for generating generic double definitions
#define DEF_GENDTYPE(BODY) \
    BODY(double, double) \
    BODY(dvec2, double) \
    BODY(dvec3, double) \
    BODY(dvec4, double) \

// Macro for generating generic double definitions (vector only)
#define DEF_GENDVTYPE(BODY) \
    BODY(dvec2, double) \
    BODY(dvec3, double) \
    BODY(dvec4, double) \

// Macro for generating generic float matrix definitions
#define DEF_GENFMAT(BODY) \
    BODY(mat2, float) \
    BODY(mat3, float) \
    BODY(mat4, float) \

// Macro for generating generic double matrix definitions
#define DEF_GENDMAT(BODY) \
    BODY(dmat2, double) \
    BODY(dmat3, double) \
    BODY(dmat4, double) \

// Macro for generating generic vec2 definitions
#define DEF_GENVEC2(BODY) \
    BODY(vec2, float) \
    BODY(dvec2, double) \
    BODY(ivec2, int) \
    BODY(uvec2, uint) \

// Macro for generating generic vec3 definitions
#define DEF_GENVEC3(BODY) \
    BODY(vec3, float) \
    BODY(dvec3, double) \
    BODY(ivec3, int) \
    BODY(uvec3, uint) \

// Macro for generating generic vec4 definitions
#define DEF_GENVEC4(BODY) \
    BODY(vec4, float) \
    BODY(dvec4, double) \
    BODY(ivec4, int) \
    BODY(uvec4, uint) \