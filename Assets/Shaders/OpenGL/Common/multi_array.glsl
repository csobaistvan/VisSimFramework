#include <Shaders/OpenGL/Common/preprocessor.glsl>

// Count and index components
#define COUNT_COMPONENT(I, N, P1, P2, P3) const uint CONCAT(n, I) COMMA_IF(I)
#define INDEX_COMPONENT(I, N, P1, P2, P3) const uint CONCAT(i, I) COMMA_IF(I)

// Index summation component
#define TOTAL_COUNT(I, N, P1, P2, P3) CONCAT(n, I) TOKEN_IF(I, *)
#define SUB_OFFSET_COMPONENT(I, N, P1, P2, P3) CONCAT(i, I) TOKEN_IF(I, *) FOR_LOOP_INNER(I, TOTAL_COUNT, 0, 0, 0) TOKEN_IF(I, +)

// Multi-array index calculator for ND arrays
#define MULTI_ARRAY_INDEX(N) \
    uint multiArrayIndex##N(FOR_LOOP(N, COUNT_COMPONENT, 0, 0, 0), FOR_LOOP(N, INDEX_COMPONENT, 0, 0, 0)) \
    { return FOR_LOOP(N, SUB_OFFSET_COMPONENT, 0, 0, 0); }

// Define the indexing functions
MULTI_ARRAY_INDEX(2)
MULTI_ARRAY_INDEX(3)
MULTI_ARRAY_INDEX(4)
MULTI_ARRAY_INDEX(5)
MULTI_ARRAY_INDEX(6)
MULTI_ARRAY_INDEX(7)
MULTI_ARRAY_INDEX(8)
MULTI_ARRAY_INDEX(9)
MULTI_ARRAY_INDEX(10)