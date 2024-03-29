// Macro for defining a buffer with a single array
#define TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, TYPE) \
    layout (LAYOUT, binding = BINDING) FLAGS buffer NAME##Buffer \
    { TYPE sData[]; } NAME

// Pre-defined types of single-array buffers with common types
#define FLOAT_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, float)
#define VEC2_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, vec2)
#define VEC3_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, vec3)
#define VEC4_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, vec4)

#define DOUBLE_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, double)
#define DVEC2_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, dvec2)
#define DVEC3_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, dvec3)
#define DVEC4_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, dvec4)

#define INT_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, int)
#define IVEC2_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, ivec2)
#define IVEC3_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, ivec3)
#define IVEC4_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, ivec4)

#define UINT_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, uint)
#define UVEC2_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, uvec2)
#define UVEC3_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, uvec3)
#define UVEC4_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME) TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, uvec4)

// Custom macro that defines the underlying structure in-place
#define _STRUCT_NAME(NAME) NAME##Data
#define STRUCT_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, STRUCT_FIELDS) \
    struct _STRUCT_NAME(NAME) { STRUCT_FIELDS; }; \
    TYPED_ARRAY_BUFFER(LAYOUT, FLAGS, BINDING, NAME, _STRUCT_NAME(NAME))