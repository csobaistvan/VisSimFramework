#version 440

// Includes
#include <Shaders/OpenGL/Common/common.glsl>
#include <Shaders/OpenGL/PostProcessing/DebugVisualization/VoxelGrid/common.glsl>

// receive voxels points position
layout(points) in;

// outputs voxels as cubes
layout(triangle_strip, max_vertices = 24) out;

// Inputs and outputs
in vec4 albedo[];
out vec4 voxelColor;

// Cube vertices and indices
const vec3 cubeVertices[8] = vec3[8] 
(
	vec3( 0.5,  0.5,  0.5),
	vec3( 0.5,  0.5, -0.5),
	vec3( 0.5, -0.5,  0.5),
	vec3( 0.5, -0.5, -0.5),
	vec3(-0.5,  0.5,  0.5),
	vec3(-0.5,  0.5, -0.5),
	vec3(-0.5, -0.5,  0.5),
	vec3(-0.5, -0.5, -0.5)
);

const int cubeIndices[24] = int[24] 
(
	0, 2, 1, 3, // right
	6, 4, 7, 5, // left
	5, 4, 1, 0, // up
	6, 7, 2, 3, // down
	4, 6, 0, 2, // front
	1, 3, 5, 7  // back
);

void main()
{
	// Skip invisible voxels
	if (albedo[0].a == 0.0)  return;

	// Compute all the projected vertices
	vec4 projectedVertices[8];
	for (int i = 0; i < 8; ++i)
	{
		const vec3 vertex = voxelToWorld(gl_in[0].gl_Position.xyz + cubeVertices[i]);
		projectedVertices[i] = sCameraData.mProjection * sCameraData.mView * vec4(vertex, 1.0);
	}

	// Write out all the faces
	for (int face = 0; face < 6; ++face)
	{
		for (int vertex = 0; vertex < 4; ++vertex)
		{
			gl_Position = projectedVertices[cubeIndices[face * 4 + vertex]];
			voxelColor = vec4(albedo[0].rgb, 1.0);
			EmitVertex();
		}

		EndPrimitive();
	}
}