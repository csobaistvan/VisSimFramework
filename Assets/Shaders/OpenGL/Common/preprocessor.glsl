// String expansion
#define EXPAND(X) X

// String concatenation
#define _CONCAT_IMPL2(S1, S2) S1 ## S2
#define _CONCAT_IMPL1(S1, S2) _CONCAT_IMPL2(S1, S2)
#define CONCAT(S1, S2)        _CONCAT_IMPL1(S1, S2)

// Inner for loop [exclusive - does not include N] - designed to be used from within FOR_LOOP
#define FOR_LOOP_INNER_0(n, a, P1, P2, P3)
#define FOR_LOOP_INNER_1(n, a, P1, P2, P3)  a(0,  n, P1, P2, P3)
#define FOR_LOOP_INNER_2(n, a, P1, P2, P3)  a(1,  n, P1, P2, P3) FOR_LOOP_INNER_1(n,  a, P1, P2, P3)
#define FOR_LOOP_INNER_3(n, a, P1, P2, P3)  a(2,  n, P1, P2, P3) FOR_LOOP_INNER_2(n,  a, P1, P2, P3)
#define FOR_LOOP_INNER_4(n, a, P1, P2, P3)  a(3,  n, P1, P2, P3) FOR_LOOP_INNER_3(n,  a, P1, P2, P3)
#define FOR_LOOP_INNER_5(n, a, P1, P2, P3)  a(4,  n, P1, P2, P3) FOR_LOOP_INNER_4(n,  a, P1, P2, P3)
#define FOR_LOOP_INNER_6(n, a, P1, P2, P3)  a(5,  n, P1, P2, P3) FOR_LOOP_INNER_5(n,  a, P1, P2, P3)
#define FOR_LOOP_INNER_7(n, a, P1, P2, P3)  a(6,  n, P1, P2, P3) FOR_LOOP_INNER_6(n,  a, P1, P2, P3)
#define FOR_LOOP_INNER_8(n, a, P1, P2, P3)  a(7,  n, P1, P2, P3) FOR_LOOP_INNER_7(n,  a, P1, P2, P3)
#define FOR_LOOP_INNER_9(n, a, P1, P2, P3)  a(8,  n, P1, P2, P3) FOR_LOOP_INNER_8(n,  a, P1, P2, P3)
#define FOR_LOOP_INNER_10(n, a, P1, P2, P3) a(9,  n, P1, P2, P3) FOR_LOOP_INNER_9(n,  a, P1, P2, P3)
#define FOR_LOOP_INNER_11(n, a, P1, P2, P3) a(10, n, P1, P2, P3) FOR_LOOP_INNER_10(n, a, P1, P2, P3)
#define FOR_LOOP_INNER_12(n, a, P1, P2, P3) a(11, n, P1, P2, P3) FOR_LOOP_INNER_11(n, a, P1, P2, P3)
#define FOR_LOOP_INNER_13(n, a, P1, P2, P3) a(13, n, P1, P2, P3) FOR_LOOP_INNER_12(n, a, P1, P2, P3)
#define FOR_LOOP_INNER_14(n, a, P1, P2, P3) a(13, n, P1, P2, P3) FOR_LOOP_INNER_13(n, a, P1, P2, P3)
#define FOR_LOOP_INNER_15(n, a, P1, P2, P3) a(14, n, P1, P2, P3) FOR_LOOP_INNER_14(n, a, P1, P2, P3)
#define FOR_LOOP_INNER_16(n, a, P1, P2, P3) a(15, n, P1, P2, P3) FOR_LOOP_INNER_15(n, a, P1, P2, P3)

#define FOR_LOOP_INNER_IMPL2(n, a, P1, P2, P3) CONCAT(FOR_LOOP_INNER_, n)(n, a, P1, P2, P3)
#define FOR_LOOP_INNER_IMPL1(n, a, P1, P2, P3) FOR_LOOP_INNER_IMPL2(n, a, P1, P2, P3)
#define FOR_LOOP_INNER(n, a, P1, P2, P3) FOR_LOOP_INNER_IMPL1(EXPAND(n), a, P1, P2, P3)

// For loop [exclusive - does not include N]
#define FOR_LOOP_EXC_0(n, a, P1, P2, P3)
#define FOR_LOOP_EXC_1(n, a, P1, P2, P3)  a(0,  n, P1, P2, P3)
#define FOR_LOOP_EXC_2(n, a, P1, P2, P3)  a(1,  n, P1, P2, P3) FOR_LOOP_EXC_1(n,  a, P1, P2, P3)
#define FOR_LOOP_EXC_3(n, a, P1, P2, P3)  a(2,  n, P1, P2, P3) FOR_LOOP_EXC_2(n,  a, P1, P2, P3)
#define FOR_LOOP_EXC_4(n, a, P1, P2, P3)  a(3,  n, P1, P2, P3) FOR_LOOP_EXC_3(n,  a, P1, P2, P3)
#define FOR_LOOP_EXC_5(n, a, P1, P2, P3)  a(4,  n, P1, P2, P3) FOR_LOOP_EXC_4(n,  a, P1, P2, P3)
#define FOR_LOOP_EXC_6(n, a, P1, P2, P3)  a(5,  n, P1, P2, P3) FOR_LOOP_EXC_5(n,  a, P1, P2, P3)
#define FOR_LOOP_EXC_7(n, a, P1, P2, P3)  a(6,  n, P1, P2, P3) FOR_LOOP_EXC_6(n,  a, P1, P2, P3)
#define FOR_LOOP_EXC_8(n, a, P1, P2, P3)  a(7,  n, P1, P2, P3) FOR_LOOP_EXC_7(n,  a, P1, P2, P3)
#define FOR_LOOP_EXC_9(n, a, P1, P2, P3)  a(8,  n, P1, P2, P3) FOR_LOOP_EXC_8(n,  a, P1, P2, P3)
#define FOR_LOOP_EXC_10(n, a, P1, P2, P3) a(9,  n, P1, P2, P3) FOR_LOOP_EXC_9(n,  a, P1, P2, P3)
#define FOR_LOOP_EXC_11(n, a, P1, P2, P3) a(10, n, P1, P2, P3) FOR_LOOP_EXC_10(n, a, P1, P2, P3)
#define FOR_LOOP_EXC_12(n, a, P1, P2, P3) a(11, n, P1, P2, P3) FOR_LOOP_EXC_11(n, a, P1, P2, P3)
#define FOR_LOOP_EXC_13(n, a, P1, P2, P3) a(13, n, P1, P2, P3) FOR_LOOP_EXC_12(n, a, P1, P2, P3)
#define FOR_LOOP_EXC_14(n, a, P1, P2, P3) a(13, n, P1, P2, P3) FOR_LOOP_EXC_13(n, a, P1, P2, P3)
#define FOR_LOOP_EXC_15(n, a, P1, P2, P3) a(14, n, P1, P2, P3) FOR_LOOP_EXC_14(n, a, P1, P2, P3)
#define FOR_LOOP_EXC_16(n, a, P1, P2, P3) a(15, n, P1, P2, P3) FOR_LOOP_EXC_15(n, a, P1, P2, P3)

#define FOR_LOOP_EXC_IMPL2(n, a, P1, P2, P3) CONCAT(FOR_LOOP_EXC_, n)(n, a, P1, P2, P3)
#define FOR_LOOP_EXC_IMPL1(n, a, P1, P2, P3) FOR_LOOP_EXC_IMPL2(n, a, P1, P2, P3)
#define FOR_LOOP(n, a, P1, P2, P3) FOR_LOOP_EXC_IMPL1(EXPAND(n), a, P1, P2, P3)

// Booleans
#define PP_BOOL_0 0
#define PP_BOOL_1 1
#define PP_BOOL_2 1
#define PP_BOOL_3 1
#define PP_BOOL_4 1
#define PP_BOOL_5 1
#define PP_BOOL_6 1
#define PP_BOOL_7 1
#define PP_BOOL_8 1
#define PP_BOOL_9 1
#define PP_BOOL_10 1
#define PP_BOOL_11 1
#define PP_BOOL_12 1
#define PP_BOOL_13 1
#define PP_BOOL_14 1
#define PP_BOOL_15 1
#define PP_BOOL_16 1

#define PP_BOOL(I) CONCAT(PP_BOOL_, I)

// Token
#define TOKEN_0(t) 
#define TOKEN_1(t) t
#define TOKEN_IF(i, t) CONCAT(TOKEN_, PP_BOOL(i))(t)

// Comma
#define COMMA_0 
#define COMMA_1 ,
#define COMMA_IF(i) CONCAT(COMMA_, PP_BOOL(i))

// If expression
#define PP_IF_0(t, f) f
#define PP_IF_1(t, f) t
#define PP_IF(i, t, f) CONCAT(PP_IF_, PP_BOOL(i))(t, f)

// Integer division, with the result rounded upwards
#define ROUNDED_DIV(NUMBER, BASE) (((NUMBER) + (BASE) - 1) / (BASE))

// Integer rounding to next multiple
#define NEXT_MULTIPLE(NUMBER, BASE) ((ROUNDED_DIV(NUMBER, BASE)) * (BASE))