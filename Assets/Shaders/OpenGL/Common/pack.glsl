
// Generates a bitmask for a bit size of S
#define PACK_BITMASK(S) ((1 << (S)) - 1)

/*******************************************************************************
*
*  packFloat4x8
*  -------------
*
*  Packs four 8-bit floats into a single uint value.
*
*******************************************************************************/

uint packFloat4x8(const vec4 val)
{
    return (uint(val.w) & 0x000000FF) << 24U | 
		   (uint(val.z) & 0x000000FF) << 16U | 
    	   (uint(val.y) & 0x000000FF) << 8U | 
    	   (uint(val.x) & 0x000000FF);
}

vec4 unpackFloat4x8(const uint val)
{
    return vec4(float((val & 0x000000FF)), 
    			float((val & 0x0000FF00) >> 8U), 
    			float((val & 0x00FF0000) >> 16U), 
   				float((val & 0xFF000000) >> 24U));
}

/*******************************************************************************
*
*  packHalf1x16Quad2x8
*  -------------------
*
*  Packs 3 floating point values into a single uint. The last two values need
*  to be normalized
*
*******************************************************************************/
uint packHalf1x16Quad2x8(vec3 value)
{
	// Ensure values are in [0..1] and make NaNs become zeros.
	value.yz = min(max(value.yz, 0.0), 1.0);

	// Each component gets 8 bit.
	value.yz = value.yz * 255 + 0.5;	
	value.yz = floor(value.yz);

	// Pack into one 32 bit uint.
	return	(packHalf2x16(vec2(value.x)) & 0x0000ffff |
			((uint(value.y))<<16) |
			((uint(value.z))<<24));
}

// Unpacks the 4 values
vec3 unpackHalf1x16Quad2x8(uint value)
{
	return vec3(unpackHalf2x16(value & 0x0000ffff)[0],
				float((value>>16) & 0x000000ff) / 255,
				float((value>>24) & 0x000000ff) / 255);
}

/*******************************************************************************
*
*  packQuad4x8
*  -----------
*
*  Packs 4 normalized floating point values into a single uint.
*
*******************************************************************************/
uint packQuad4x8(vec4 value)
{
	// Ensure values are in [0..1] and make NaNs become zeros.
	value = min(max(value, 0.0), 1.0);

	// Each component gets 8 bit.
	value = value * 255 + 0.5;	
	value = floor(value);

	// Pack into one 32 bit uint.
	return	((uint(value.x))	  |
			((uint(value.y))<< 8) |
			((uint(value.z))<<16) |
			((uint(value.w))<<24));
}

// Unpacks the 4 values
vec4 unpackQuad4x8(uint value)
{
	return vec4(float((value	) & 0x000000ff) / 255,
				float((value>> 8) & 0x000000ff) / 255,
				float((value>>16) & 0x000000ff) / 255,
				float((value>>24) & 0x000000ff) / 255);
}

/*******************************************************************************
*
*  packUint2xXY
*  ------------
*
*  Packs two uints with a bit size of X and Y into a single value.
*
*******************************************************************************/

// Packs an X-bit and an Y-bit uint into a single 32-bit value.
// s denotes the size of v1 and v2, respectively.
uint packUint2xXY(uvec2 v, uvec2 s)
{
    return (v.y << s.x) | v.x;
}

// Unpacks the first (x) component of a packed uvec2 value.
uint unpackUint2xXY_v1(uint v, uvec2 s)
{
    return v & PACK_BITMASK(s.x);
}

// Unpacks the second (y) component of a packed uvec2 value.
uint unpackUint2xXY_v2(uint v, uvec2 s)
{
    return (v >> s.x) & PACK_BITMASK(s.y);
}

// Unpacks an X-bit and an Y-bit uint packed into a single 32-bit value.
uvec2 unpackUint2xXY(uint v, uvec2 s)
{
    return uvec2(unpackUint2xXY_v1(v, s), unpackUint2xXY_v2(v, s));
}

/*******************************************************************************
*
*  packUint3xXYZ
*  -------------
*
*  Packs three uints with a bit size of X, Y and Z into a single value.
*
*******************************************************************************/

// Packs an X-bit, an Y-bit and Z-bit uint into a single 32-bit value.
// s denotes the size of v1, v2 and v3, respectively.
uint packUint3xXYZ(uvec3 v, uvec3 s)
{
    return (v.z << (s.x + s.y)) | (v.y << s.x) | v.x;
}

// Unpacks the first (x) component of a packed uvec3 value.
uint unpackUint3xXYZ_v1(uint v, uvec3 s)
{
    return v & PACK_BITMASK(s.x);
}

// Unpacks the second (y) component of a packed uvec3 value.
uint unpackUint3xXYZ_v2(uint v, uvec3 s)
{
    return (v >> s.x) & PACK_BITMASK(s.y);
}

// Unpacks the third (z) component of a packed uvec3 value.
uint unpackUint3xXYZ_v3(uint v, uvec3 s)
{
    return (v >> (s.x + s.y)) & PACK_BITMASK(s.z);
}

// Unpacks an X-bit, an Y-bit and a Z-bit uint packed into a single 32-bit value.
uvec3 unpackUint3xXYZ(uint v, uvec3 s)
{
    return uvec3(unpackUint3xXYZ_v1(v, s), unpackUint3xXYZ_v2(v, s), unpackUint3xXYZ_v3(v, s));
}

/*******************************************************************************
*
*  packUint4xXYZW
*  --------------
*
*  Packs four uints with a bit size of X, Y, Z and W into a single value.
*
*******************************************************************************/

// Packs an X-bit, an Y-bit, a Z-bit and a W-bit uint into a single 32-bit value.
// s denotes the size of v1, v2, v3 and v4, respectively.
uint packUint4xXYZW(uvec4 v, uvec4 s)
{
    return (v.w << (s.x + s.y + s.z)) | (v.z << (s.x + s.y)) | (v.y << s.x) | v.x;
}

// Unpacks the first (x) component of a packed uvec4 value.
uint unpackUint4xXYZW_v1(uint v, uvec4 s)
{
    return v & PACK_BITMASK(s.x);
}

// Unpacks the second (y) component of a packed uvec4 value.
uint unpackUint4xXYZW_v2(uint v, uvec4 s)
{
    return (v >> s.x) & PACK_BITMASK(s.y);
}

// Unpacks the third (z) component of a packed uvec4 value.
uint unpackUint4xXYZW_v3(uint v, uvec4 s)
{
    return (v >> (s.x + s.y)) & PACK_BITMASK(s.z);
}

// Unpacks the third (z) component of a packed uvec4 value.
uint unpackUint4xXYZW_v4(uint v, uvec4 s)
{
    return (v >> (s.x + s.y + s.z)) & PACK_BITMASK(s.w);
}

// Unpacks an X-bit, an Y-bit, a Z-bit and a W-bit uint packed into a single 32-bit value.
uvec4 unpackUint4xXYZW(uint v, uvec4 s)
{
    return uvec4(unpackUint4xXYZW_v1(v, s), unpackUint4xXYZW_v2(v, s), unpackUint4xXYZW_v3(v, s), unpackUint4xXYZW_v4(v, s));
}

/*******************************************************************************
*
*  packUint2x16
*  ------------
*
*  Packs two 16-bit uints into a single value.
*
*******************************************************************************/

// Packs two 16-bit uints into a single 32-bit value.
uint packUint2x16(uvec2 v)
{
    return (v.y << 16 | (v.x & 0xFFFFFFFF));
}

// Unpacks the first (x) component of a packed uvec2 value.
uint unpackUint2x16_v1(uint v)
{
    return (v << 16) >> 16;
}

// Unpacks the second (y) component of a packed uvec2 value.
uint unpackUint2x16_v2(uint v)
{
    return v >> 16;
}

// Unpacks two 16-bit uints packed into a single 32-bit value.
uvec2 unpackUint2x16(uint v)
{
    return uvec2((v << 16) >> 16, v >> 16);
}

/*******************************************************************************
*
*  packHalf1x16Uint1x16
*  --------------------
*
*  Packs a 16-bit float and a 16-bit uint into a single 32-bit value.
*
*******************************************************************************/

// Packs a 16-bit float and a 16-bit uint into a single 32-bit value.
// The uint is stored in the least significant bits.
uint packHalf1x16Uint1x16(float v1, uint v2)
{
    return (packHalf2x16(vec2(v1, v1)) << 16) | v2;
}

// Unpacks the float of a packed float-uint doublet.
float unpackHalf1x16Uint1x16_f(uint v)
{
    return unpackHalf2x16(v).y;
}

// Unpacks the uint of a packed float-uint doublet.
uint unpackHalf1x16Uint1x16_ui(uint v)
{
    return (v << 16) >> 16;
}

/*******************************************************************************
*
*  packHalf1x16Uint2xXY
*  --------------------
*
*  Packs a 16 half precision float and two uints with a bit size of X and Y into a single value.
*
*******************************************************************************/

// Packs two 16-bit uints into a single 32-bit value.
uint packHalf1x16Uint2xXY(float f, uvec2 v, uvec2 s)
{
	return packHalf2x16(vec2(f)) << 16 | (v.y << s.x) | v.x;
}

// Unpacks the first 16 bit float component
float unpackHalf1x16Uint2xXY_f(uint v, uvec2 s)
{
    return unpackHalf2x16(v).y;
}

// Unpacks the second (y) component of a packed uvec2 value.
uint unpackHalf1x16Uint2xXY_u1(uint v, uvec2 s)
{
    return v & PACK_BITMASK(s.x);
}

// Unpacks two 16-bit uints packed into a single 32-bit value.
uint unpackHalf1x16Uint2xXY_u2(uint v, uvec2 s)
{
    return (v >> s.x) & PACK_BITMASK(s.y);
}

/*******************************************************************************
*
*  packHalf3x16Uint1x16
*  --------------------
*
*  Packs a three-component float vector and a uint into two 32-bit uints.
*
*******************************************************************************/

// Packs a three-component float vector and a uint into two 32-bit uints.
// v.x and v.y are stored in the least and most significant 16 bits of the first uint value.
// ui and v.z are stored in the least and most significant 16 bits of the second uint value.
uvec2 packHalf3x16Uint1x16(vec3 v123, uint v4)
{
    return uvec2(packHalf2x16(v123.rg), (packHalf2x16(v123.bb) << 16) | v4);
}

// Unpacks the three floats of a packed vec3-uint doublet.
vec3 unpackHalf3x16Uint1x16_v3(uvec2 v)
{
    return vec3(unpackHalf2x16(v[0]), unpackHalf2x16(v[1]).y);
}

// Unpacks the uint of a packed vec3-uint doublet.
uint unpackHalf3x16Uint1x16_ui(uvec2 v)
{
    return (v.y << 16) >> 16;
}