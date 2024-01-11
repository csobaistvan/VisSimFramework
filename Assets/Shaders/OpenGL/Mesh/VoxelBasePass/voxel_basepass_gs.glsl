#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/Mesh/VoxelBasePass/common.glsl>

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// Also pass through the other attribs
in VertexData
{
    vec3 vPosition;
    vec3 vNormal;
    vec2 vUv;
    mat3 mTBN;
} g_in[];

out GeometryData
{
    vec3 vPosition;
    vec3 vPositionCS;
    vec3 vNormal;
    vec2 vUv;
    mat3 mTBN;
    vec3 vVoxelPosition;
    flat vec4 vTriangleAABB;
} g_out;

vec3 calcTrianglePlane(const vec4 v0, const vec4 v1, const vec2 halfVoxel)
{
	const vec3 plane = cross(v0.xyw - v1.xyw, v1.xyw);
	return plane - vec3(0, 0, dot(halfVoxel, abs(plane.xy)));
}

vec3 calcTriangleIntersection(const vec3 p0, const vec3 p1)
{
	const vec3 intersection = cross(p0, p1);
	return intersection / intersection.z;
}

vec3 calcDilatedVertex(const vec4 trianglePlane, const vec3 p0, const vec3 p1)
{
	const vec3 intersection = calcTriangleIntersection(p0, p1);
	const float z = -dot(intersection, trianglePlane.xyw) / trianglePlane.z;
	return vec3(intersection.xy, z);
}

////////////////////////////////////////////////////////////////////////////////
void main()
{
	const vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	const vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	const vec3 faceNormal = cross(p1, p2);
	const uint selectedIndex = maxIndex(abs(faceNormal));
	const mat4 viewProjection = mVoxelViewProjections[selectedIndex];
	const mat4 viewProjectionI = mInverseVoxelViewProjections[selectedIndex];

    // Texture coords
	vec2 texCoord[3] = vec2[3]
	(
		g_in[0].vUv,
		g_in[1].vUv,
		g_in[2].vUv
	);

	// Transform vertices to clip space
	vec4 pos[3] = vec4[3]
	(
		viewProjection * gl_in[0].gl_Position,
		viewProjection * gl_in[1].gl_Position,
		viewProjection * gl_in[2].gl_Position
	);

	// Construct the plane in which the triangle lies
	vec4 trianglePlane;
	trianglePlane.xyz = normalize(cross(pos[1].xyz - pos[0].xyz, pos[2].xyz - pos[0].xyz));
	trianglePlane.w = -dot(pos[0].xyz, trianglePlane.xyz);

	// Skip perpendicular triangles
    if (trianglePlane.z == 0.0) return;

    // Change winding order for back faces
    if (dot(trianglePlane.xyz, vec3(0.0, 0.0, 1.0)) < 0.0)
    {
        swap(pos[1], pos[2]);
        swap(texCoord[1], texCoord[2]);
    }
    
	// Size of one half of a voxel
	const vec2 halfVoxel = 0.5 * vec2(1.0 / float(sRenderData.uiNumVoxels));

	// Aabb for the triangle
	g_out.vTriangleAABB.xy = min(pos[2].xy, min(pos[1].xy, pos[0].xy));
	g_out.vTriangleAABB.zw = max(pos[2].xy, max(pos[1].xy, pos[0].xy));

	// Apply dilation, if requested
	if (sRenderData.uiVoxelDilationMode == VoxelDilationMethod_NormalDilation)
	{
		// Calculate the plane through each edge of the triangle in normal		
		const vec3 planes[3] =
		{
			calcTrianglePlane(pos[0], pos[2], halfVoxel),
			calcTrianglePlane(pos[1], pos[0], halfVoxel),
			calcTrianglePlane(pos[2], pos[1], halfVoxel),
		};
		
		// Calculate the dilated vertices
		pos[0].xyz = calcDilatedVertex(trianglePlane, planes[0], planes[1]);
		pos[1].xyz = calcDilatedVertex(trianglePlane, planes[1], planes[2]);
		pos[2].xyz = calcDilatedVertex(trianglePlane, planes[2], planes[0]);
	
		// Enlarge the AABB by a half voxel
		g_out.vTriangleAABB.xy -= halfVoxel;
		g_out.vTriangleAABB.zw += halfVoxel;
	}

	for (int i = 0; i < 3; ++i)
	{
        // Compute the voxel grid location
		const vec4 voxelPos = viewProjectionI * pos[i];
		const vec3 voxelGridLocation = worldToVoxel(voxelPos.xyz / voxelPos.w);

		// Write out the fragment attributes
		gl_Position = pos[i];
		g_out.vPosition = g_in[i].vPosition;
		g_out.vPositionCS = pos[i].xyz;
		g_out.vNormal = g_in[i].vNormal;
		g_out.vUv = texCoord[i];
		g_out.mTBN = g_in[i].mTBN;
		g_out.vVoxelPosition = voxelGridLocation;
		EmitVertex();
	}

	EndPrimitive();
}