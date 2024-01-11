#include "PCH.h"
#include "GPU.h"
#include "Config.h"
#include "Debug.h"
#include "StaticInitializer.h"

namespace GPU
{
	////////////////////////////////////////////////////////////////////////////////
	#define ENUM_TO_STRING(E, P) std::string(#E).substr(strlen(P))
	#define ENUM_STRING_PAIR(E, P) { E, std::string(#E).substr(strlen(P)) }

	////////////////////////////////////////////////////////////////////////////////
	/** Name of the default renderer */
	std::string renderer()
	{
		static std::string s_renderer = Config::AttribValue("renderer").get<std::string>();
		return s_renderer;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Should we allow optional extensions or not. */
	bool enableOptionalExtensions()
	{
		static bool s_enable = Config::AttribValue("glsl_extensions").get<int>() == 1;
		return s_enable;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Default resolution. */
	glm::ivec2 defaultResolution()
	{
		static glm::ivec2 s_resolution = Config::AttribValue("resolution").get<glm::ivec2>();
		return s_resolution;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Default resolution. */
	glm::ivec2 maxResolution()
	{
		static glm::ivec2 s_maxResolution = glm::max(
			Config::AttribValue("max_resolution").get<glm::ivec2>(), 
			Config::AttribValue("resolution").get<glm::ivec2>());
		return s_maxResolution;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Number of gbuffer layers. */
	int numLayers()
	{
		static int s_numLayers = glm::clamp(Config::AttribValue("layers").get<int>(), 0, (int)Constants::s_maxLayers);
		return s_numLayers;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Number of MSAA samples. */
	int numMsaaSamples()
	{
		static int s_msaa = Config::AttribValue("msaa").get<int>();
		return s_msaa;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Voxel grid size. */
	int voxelGridSize()
	{
		static int s_voxelGrid = Config::AttribValue("voxel_grid").get<int>();
		return s_voxelGrid;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Shadow map resolution. */
	int shadowMapResolution()
	{
		static int s_resolution = Config::AttribValue("shadow_map_resolution").get<int>();
		return s_resolution;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Whether we should trace the GL api or not. */
	int glTraceLevel()
	{
		static int s_traceLevel = glm::clamp(Config::AttribValue("gl_trace").get<size_t>(), (size_t)0, TraceLevels_meta.members.size());
		return s_traceLevel;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Whether we should trace the GL api or not. */
	int glDebugConfig()
	{
		static int s_config = glm::clamp(Config::AttribValue("gl_debug").get<size_t>(), (size_t)0, GLDebugConfigs_meta.members.size());
		return s_config;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** The various capability enums to query. */
	std::unordered_map<std::string, std::unordered_map<GLenum, CapabilityAttrib>> s_capabilityAttribs;
	STATIC_INITIALIZER()
	{
		#define CAPABILITY(CATEGORY, NAME, TYPE, ...) s_capabilityAttribs[CATEGORY][NAME] = { CATEGORY, NAME, TYPE, __VA_ARGS__ }

		// Debug output
		CAPABILITY("Debug Output", GL_MAX_DEBUG_LOGGED_MESSAGES, GL_INT, 1);
		CAPABILITY("Debug Output", GL_MAX_DEBUG_MESSAGE_LENGTH, GL_INT, 1);
		CAPABILITY("Debug Output", GL_MAX_DEBUG_GROUP_STACK_DEPTH, GL_INT, 1);
		CAPABILITY("Debug Output", GL_MAX_LABEL_LENGTH, GL_INT, 1);

		// Uniforms
		CAPABILITY("Programs", GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_IMAGE_SAMPLES, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_IMAGE_UNITS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_PROGRAM_TEXEL_OFFSET, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_SHADER_STORAGE_BLOCK_SIZE, GL_INT64_ARB, 1);
		CAPABILITY("Programs", GL_MAX_SUBROUTINES, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_UNIFORM_BLOCK_SIZE, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_UNIFORM_LOCATIONS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_VARYING_VECTORS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_VERTEX_ATTRIB_BINDINGS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_VERTEX_ATTRIB_STRIDE, GL_INT, 1);
		CAPABILITY("Programs", GL_MIN_PROGRAM_TEXEL_OFFSET, GL_INT, 1);

		CAPABILITY("Programs", GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_COMBINED_ATOMIC_COUNTERS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_COMBINED_IMAGE_UNIFORMS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, GL_INT, 1);
		CAPABILITY("Programs", GL_MAX_COMBINED_UNIFORM_BLOCKS, GL_INT, 1);

		// Buffer binding
		CAPABILITY("Buffer Binding", GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, GL_INT, 1);
		CAPABILITY("Buffer Binding", GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, GL_INT, 1);
		CAPABILITY("Buffer Binding", GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, GL_INT, 1);
		CAPABILITY("Buffer Binding", GL_MAX_UNIFORM_BUFFER_BINDINGS, GL_INT, 1);
		CAPABILITY("Buffer Binding", GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, GL_INT, 1);
		CAPABILITY("Buffer Binding", GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, GL_INT, 1);
		CAPABILITY("Buffer Binding", GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, GL_INT, 1);

		// Textures
		CAPABILITY("Textures", GL_MAX_3D_TEXTURE_SIZE, GL_INT, 1);
		CAPABILITY("Textures", GL_MAX_ARRAY_TEXTURE_LAYERS, GL_INT, 1);
		CAPABILITY("Textures", GL_MAX_CUBE_MAP_TEXTURE_SIZE, GL_INT, 1);
		CAPABILITY("Textures", GL_MAX_RECTANGLE_TEXTURE_SIZE, GL_INT, 1);
		CAPABILITY("Textures", GL_MAX_RENDERBUFFER_SIZE, GL_INT, 1);
		CAPABILITY("Textures", GL_MAX_TEXTURE_BUFFER_SIZE, GL_INT, 1);
		CAPABILITY("Textures", GL_MAX_TEXTURE_LOD_BIAS, GL_FLOAT, 1);
		CAPABILITY("Textures", GL_MAX_TEXTURE_SIZE, GL_INT, 1);

		// Framebuffers
		CAPABILITY("Framebuffers", GL_MAX_COLOR_ATTACHMENTS, GL_INT, 1);
		CAPABILITY("Framebuffers", GL_MAX_COLOR_TEXTURE_SAMPLES, GL_INT, 1);
		CAPABILITY("Framebuffers", GL_MAX_DEPTH_TEXTURE_SAMPLES, GL_INT, 1);
		CAPABILITY("Framebuffers", GL_MAX_DRAW_BUFFERS, GL_INT, 1);
		CAPABILITY("Framebuffers", GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, GL_INT, 1);
		CAPABILITY("Framebuffers", GL_MAX_FRAMEBUFFER_HEIGHT, GL_INT, 1);
		CAPABILITY("Framebuffers", GL_MAX_FRAMEBUFFER_LAYERS, GL_INT, 1);
		CAPABILITY("Framebuffers", GL_MAX_FRAMEBUFFER_SAMPLES, GL_INT, 1);
		CAPABILITY("Framebuffers", GL_MAX_FRAMEBUFFER_WIDTH, GL_INT, 1);
		CAPABILITY("Framebuffers", GL_MAX_SAMPLES, GL_INT, 1);

		// Transformation
		CAPABILITY("Transformation State", GL_MAX_CLIP_DISTANCES, GL_INT, 1);
		//CAPABILITY("Transformation State", GL_MAX_VIEWPORT_DIMS, GL_FLOAT, 2);
		CAPABILITY("Transformation State", GL_MAX_VIEWPORTS, GL_INT, 1);

		// Vertex arrays
		CAPABILITY("Vertex Arrays", GL_MAX_ELEMENT_INDEX, GL_INT64_ARB, 1);
		CAPABILITY("Vertex Arrays", GL_MAX_ELEMENTS_INDICES, GL_INT, 1);
		CAPABILITY("Vertex Arrays", GL_MAX_ELEMENTS_VERTICES, GL_INT, 1);

		// Tess. control shader
		CAPABILITY("Tessellation Control Shader", GL_MAX_PATCH_VERTICES, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_INPUT_COMPONENTS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_GEN_LEVEL, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_PATCH_COMPONENTS, GL_INT, 1);

		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS, GL_INT, 1);
		CAPABILITY("Tessellation Control Shader", GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS, GL_INT, 1);

		// Tess. eval shader
		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS, GL_INT, 1);
		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS, GL_INT, 1);

		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS, GL_INT, 1);
		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS, GL_INT, 1);
		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS, GL_INT, 1);
		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, GL_INT, 1);
		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS, GL_INT, 1);
		CAPABILITY("Tessellation Evaluation Shader", GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS, GL_INT, 1);

		// Vert. shader
		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_ATTRIBS, GL_INT, 1);
		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_OUTPUT_COMPONENTS, GL_INT, 1);
		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_UNIFORM_VECTORS, GL_INT, 1);

		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, GL_INT, 1);
		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_ATOMIC_COUNTERS, GL_INT, 1);
		CAPABILITY("Vertex Shader", GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_IMAGE_UNIFORMS, GL_INT, 1);
		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, GL_INT, 1);
		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, GL_INT, 1);
		CAPABILITY("Vertex Shader", GL_MAX_VERTEX_UNIFORM_BLOCKS, GL_INT, 1);

		// Geom. shader
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_INPUT_COMPONENTS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_OUTPUT_VERTICES, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_SHADER_INVOCATIONS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_VERTEX_STREAMS, GL_INT, 1);

		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_ATOMIC_COUNTERS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_IMAGE_UNIFORMS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, GL_INT, 1);
		CAPABILITY("Geometry Shader", GL_MAX_GEOMETRY_UNIFORM_BLOCKS, GL_INT, 1);

		// Frag. shader
		CAPABILITY("Fragment Shader", GL_MAX_FRAGMENT_INPUT_COMPONENTS, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MAX_FRAGMENT_UNIFORM_VECTORS, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, GL_INT, 1);

		CAPABILITY("Fragment Shader", GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MAX_FRAGMENT_ATOMIC_COUNTERS, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MAX_FRAGMENT_IMAGE_UNIFORMS, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MAX_TEXTURE_IMAGE_UNITS, GL_INT, 1);
		CAPABILITY("Fragment Shader", GL_MAX_FRAGMENT_UNIFORM_BLOCKS, GL_INT, 1);

		// Comp. shader
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, GL_INT, 1);
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, GL_INT, 1);
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_WORK_GROUP_SIZE, GL_INT, 3);
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_WORK_GROUP_COUNT, GL_INT, 3);

		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, GL_INT, 1);
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_ATOMIC_COUNTERS, GL_INT, 1);
		CAPABILITY("Compute Shader", GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_IMAGE_UNIFORMS, GL_INT, 1);
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, GL_INT, 1);
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_UNIFORM_COMPONENTS, GL_INT, 1);
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, GL_INT, 1);
		CAPABILITY("Compute Shader", GL_MAX_COMPUTE_UNIFORM_BLOCKS, GL_INT, 1);

		#undef CAPABILITY
	};

	////////////////////////////////////////////////////////////////////////////////
	std::unordered_map<GLenum, std::string> s_enumNames =
	{
		// Framebuffer status codes
		ENUM_STRING_PAIR(GL_FRAMEBUFFER_COMPLETE, "GL_"),
		ENUM_STRING_PAIR(GL_FRAMEBUFFER_UNDEFINED, "GL_"),
		ENUM_STRING_PAIR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, "GL_"),
		ENUM_STRING_PAIR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT, "GL_"),
		ENUM_STRING_PAIR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_FRAMEBUFFER_UNSUPPORTED, "GL_"),
		ENUM_STRING_PAIR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE, "GL_"),
		ENUM_STRING_PAIR(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS, "GL_"),

		// Texture types
		ENUM_STRING_PAIR(GL_TEXTURE_1D, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_1D_ARRAY, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_2D, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_2D_ARRAY, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_2D_MULTISAMPLE, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_3D, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_CUBE_MAP, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_CUBE_MAP_ARRAY, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_RECTANGLE, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_BUFFER, "GL_"),

		// Texture layouts
		ENUM_STRING_PAIR(GL_RED, "GL_"),
		ENUM_STRING_PAIR(GL_RG, "GL_"),
		ENUM_STRING_PAIR(GL_RGB, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA, "GL_"),
		
		// Texture formats
		ENUM_STRING_PAIR(GL_R8, "GL_"),
		ENUM_STRING_PAIR(GL_R8_SNORM, "GL_"),
		ENUM_STRING_PAIR(GL_R16, "GL_"),
		ENUM_STRING_PAIR(GL_R16_SNORM, "GL_"),
		ENUM_STRING_PAIR(GL_RG8, "GL_"),
		ENUM_STRING_PAIR(GL_RG8_SNORM, "GL_"),
		ENUM_STRING_PAIR(GL_RG16, "GL_"),
		ENUM_STRING_PAIR(GL_RG16_SNORM, "GL_"),
		ENUM_STRING_PAIR(GL_R3_G3_B2, "GL_"),
		ENUM_STRING_PAIR(GL_RGB4, "GL_"),
		ENUM_STRING_PAIR(GL_RGB5, "GL_"),
		ENUM_STRING_PAIR(GL_RGB8, "GL_"),
		ENUM_STRING_PAIR(GL_RGB8_SNORM, "GL_"),
		ENUM_STRING_PAIR(GL_RGB10, "GL_"),
		ENUM_STRING_PAIR(GL_RGB12, "GL_"),
		ENUM_STRING_PAIR(GL_RGB16_SNORM, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA2, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA4, "GL_"),
		ENUM_STRING_PAIR(GL_RGB5_A1, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA8, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA8_SNORM, "GL_"),
		ENUM_STRING_PAIR(GL_RGB10_A2, "GL_"),
		ENUM_STRING_PAIR(GL_RGB10_A2UI, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA12, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA16, "GL_"),
		ENUM_STRING_PAIR(GL_SRGB8, "GL_"),
		ENUM_STRING_PAIR(GL_SRGB8_ALPHA8, "GL_"),
		ENUM_STRING_PAIR(GL_R16F, "GL_"),
		ENUM_STRING_PAIR(GL_RG16F, "GL_"),
		ENUM_STRING_PAIR(GL_RGB16F, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA16F, "GL_"),
		ENUM_STRING_PAIR(GL_R32F, "GL_"),
		ENUM_STRING_PAIR(GL_RG32F, "GL_"),
		ENUM_STRING_PAIR(GL_RGB32F, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA32F, "GL_"),
		ENUM_STRING_PAIR(GL_R11F_G11F_B10F, "GL_"),
		ENUM_STRING_PAIR(GL_RGB9_E5, "GL_"),
		ENUM_STRING_PAIR(GL_R8I, "GL_"),
		ENUM_STRING_PAIR(GL_R8UI, "GL_"),
		ENUM_STRING_PAIR(GL_R16I, "GL_"),
		ENUM_STRING_PAIR(GL_R16UI, "GL_"),
		ENUM_STRING_PAIR(GL_R32I, "GL_"),
		ENUM_STRING_PAIR(GL_R32UI, "GL_"),
		ENUM_STRING_PAIR(GL_RG8I, "GL_"),
		ENUM_STRING_PAIR(GL_RG8UI, "GL_"),
		ENUM_STRING_PAIR(GL_RG16I, "GL_"),
		ENUM_STRING_PAIR(GL_RG16UI, "GL_"),
		ENUM_STRING_PAIR(GL_RG32I, "GL_"),
		ENUM_STRING_PAIR(GL_RG32UI, "GL_"),
		ENUM_STRING_PAIR(GL_RGB8I, "GL_"),
		ENUM_STRING_PAIR(GL_RGB8UI, "GL_"),
		ENUM_STRING_PAIR(GL_RGB16I, "GL_"),
		ENUM_STRING_PAIR(GL_RGB16UI, "GL_"),
		ENUM_STRING_PAIR(GL_RGB32I, "GL_"),
		ENUM_STRING_PAIR(GL_RGB32UI, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA8I, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA8UI, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA16I, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA16UI, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA32I, "GL_"),
		ENUM_STRING_PAIR(GL_RGBA32UI, "GL_"),
		ENUM_STRING_PAIR(GL_DEPTH_COMPONENT32F, "GL_"),
		ENUM_STRING_PAIR(GL_DEPTH_COMPONENT24, "GL_"),
		ENUM_STRING_PAIR(GL_DEPTH_COMPONENT16, "GL_"),
		ENUM_STRING_PAIR(GL_DEPTH32F_STENCIL8, "GL_"),
		ENUM_STRING_PAIR(GL_DEPTH24_STENCIL8, "GL_"),
		ENUM_STRING_PAIR(GL_STENCIL_INDEX8, "GL_"),

		// Texture warp mode
		ENUM_STRING_PAIR(GL_CLAMP_TO_EDGE, "GL_"),
		ENUM_STRING_PAIR(GL_CLAMP_TO_BORDER, "GL_"),
		ENUM_STRING_PAIR(GL_MIRRORED_REPEAT, "GL_"),
		ENUM_STRING_PAIR(GL_REPEAT, "GL_"),
		ENUM_STRING_PAIR(GL_MIRROR_CLAMP_TO_EDGE, "GL_"),
		ENUM_STRING_PAIR(GL_CLAMP_TO_EDGE, "GL_"),
		ENUM_STRING_PAIR(GL_CLAMP, "GL_"),

		// Texture filter modes
		ENUM_STRING_PAIR(GL_NEAREST, "GL_"),
		ENUM_STRING_PAIR(GL_LINEAR, "GL_"),
		ENUM_STRING_PAIR(GL_NEAREST_MIPMAP_NEAREST, "GL_"),
		ENUM_STRING_PAIR(GL_LINEAR_MIPMAP_NEAREST, "GL_"),
		ENUM_STRING_PAIR(GL_NEAREST_MIPMAP_LINEAR, "GL_"),
		ENUM_STRING_PAIR(GL_LINEAR_MIPMAP_LINEAR, "GL_"),

		// Buffer types
		ENUM_STRING_PAIR(GL_ARRAY_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_ATOMIC_COUNTER_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_COPY_READ_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_COPY_WRITE_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_DISPATCH_INDIRECT_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_DRAW_INDIRECT_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_ELEMENT_ARRAY_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_PIXEL_PACK_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_PIXEL_UNPACK_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_QUERY_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_SHADER_STORAGE_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_TEXTURE_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_TRANSFORM_FEEDBACK_BUFFER, "GL_"),
		ENUM_STRING_PAIR(GL_UNIFORM_BUFFER, "GL_"),

		// Log error codes
		ENUM_STRING_PAIR(GL_INVALID_ENUM, "GL_"),
		ENUM_STRING_PAIR(GL_INVALID_VALUE, "GL_"),
		ENUM_STRING_PAIR(GL_INVALID_OPERATION, "GL_"),
		ENUM_STRING_PAIR(GL_STACK_OVERFLOW, "GL_"),
		ENUM_STRING_PAIR(GL_STACK_UNDERFLOW, "GL_"),
		ENUM_STRING_PAIR(GL_OUT_OF_MEMORY, "GL_"),
		ENUM_STRING_PAIR(GL_INVALID_FRAMEBUFFER_OPERATION, "GL_"),
		ENUM_STRING_PAIR(GL_CONTEXT_LOST, "GL_"),
		ENUM_STRING_PAIR(GL_TABLE_TOO_LARGE, "GL_"),

		// GPU capabilities
		ENUM_STRING_PAIR(GL_MAX_DEBUG_LOGGED_MESSAGES, ""),
		ENUM_STRING_PAIR(GL_MAX_DEBUG_MESSAGE_LENGTH, ""),
		ENUM_STRING_PAIR(GL_MAX_DEBUG_GROUP_STACK_DEPTH, ""),
		ENUM_STRING_PAIR(GL_MAX_LABEL_LENGTH, ""),
		ENUM_STRING_PAIR(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_IMAGE_SAMPLES, ""),
		ENUM_STRING_PAIR(GL_MAX_IMAGE_UNITS, ""),
		ENUM_STRING_PAIR(GL_MAX_PROGRAM_TEXEL_OFFSET, ""),
		ENUM_STRING_PAIR(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_SUBROUTINES, ""),
		ENUM_STRING_PAIR(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, ""),
		ENUM_STRING_PAIR(GL_MAX_UNIFORM_BLOCK_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_UNIFORM_LOCATIONS, ""),
		ENUM_STRING_PAIR(GL_MAX_VARYING_VECTORS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_ATTRIB_BINDINGS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_ATTRIB_STRIDE, ""),
		ENUM_STRING_PAIR(GL_MIN_PROGRAM_TEXEL_OFFSET, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_ATOMIC_COUNTERS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_IMAGE_UNIFORMS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_UNIFORM_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, ""),
		ENUM_STRING_PAIR(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, ""),
		ENUM_STRING_PAIR(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_UNIFORM_BUFFER_BINDINGS, ""),
		ENUM_STRING_PAIR(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, ""),
		ENUM_STRING_PAIR(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_3D_TEXTURE_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_ARRAY_TEXTURE_LAYERS, ""),
		ENUM_STRING_PAIR(GL_MAX_CUBE_MAP_TEXTURE_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_RECTANGLE_TEXTURE_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_RENDERBUFFER_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_TEXTURE_BUFFER_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_TEXTURE_LOD_BIAS, ""),
		ENUM_STRING_PAIR(GL_MAX_TEXTURE_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_COLOR_ATTACHMENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_COLOR_TEXTURE_SAMPLES, ""),
		ENUM_STRING_PAIR(GL_MAX_DEPTH_TEXTURE_SAMPLES, ""),
		ENUM_STRING_PAIR(GL_MAX_DRAW_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAMEBUFFER_HEIGHT, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAMEBUFFER_LAYERS, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAMEBUFFER_SAMPLES, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAMEBUFFER_WIDTH, ""),
		ENUM_STRING_PAIR(GL_MAX_SAMPLES, ""),
		ENUM_STRING_PAIR(GL_MAX_CLIP_DISTANCES, ""),
		ENUM_STRING_PAIR(GL_MAX_VIEWPORT_DIMS, ""),
		ENUM_STRING_PAIR(GL_MAX_VIEWPORTS, ""),
		ENUM_STRING_PAIR(GL_MAX_ELEMENT_INDEX, ""),
		ENUM_STRING_PAIR(GL_MAX_ELEMENTS_INDICES, ""),
		ENUM_STRING_PAIR(GL_MAX_ELEMENTS_VERTICES, ""),
		ENUM_STRING_PAIR(GL_MAX_PATCH_VERTICES, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_INPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_GEN_LEVEL, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_PATCH_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS, ""),
		ENUM_STRING_PAIR(GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_ATTRIBS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_OUTPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_UNIFORM_VECTORS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_ATOMIC_COUNTERS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_IMAGE_UNIFORMS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_UNIFORM_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_INPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_OUTPUT_VERTICES, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_SHADER_INVOCATIONS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_VERTEX_STREAMS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_ATOMIC_COUNTERS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_IMAGE_UNIFORMS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, ""),
		ENUM_STRING_PAIR(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAGMENT_INPUT_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAGMENT_UNIFORM_VECTORS, ""),
		ENUM_STRING_PAIR(GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, ""),
		ENUM_STRING_PAIR(GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_TEXTURE_IMAGE_UNITS, ""),
		ENUM_STRING_PAIR(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_WORK_GROUP_SIZE, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_WORK_GROUP_COUNT, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_ATOMIC_COUNTERS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_IMAGE_UNIFORMS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, ""),
		ENUM_STRING_PAIR(GL_MAX_COMPUTE_UNIFORM_BLOCKS, ""),
	};

	////////////////////////////////////////////////////////////////////////////////
	std::string enumToRawString(GLenum e)
	{
		std::stringstream ss;
		ss << "0x" << std::setfill('0') << std::setw(sizeof(GLenum) * 2) << std::hex << e;
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	// Enum to string conversion
	std::string enumToString(GLenum e)
	{
		if (auto it = s_enumNames.find(e); it != s_enumNames.end())
			return it->second;
		return "Unknown (" + enumToRawString(e) + ")";
	}

	////////////////////////////////////////////////////////////////////////////////
	struct DataTypeProperties
	{
		std::string m_name;
		size_t m_size;
	};

	////////////////////////////////////////////////////////////////////////////////
	std::unordered_map<GLenum, DataTypeProperties> s_dataTypeProperties =
	{
		{ GL_FLOAT, { "float", sizeof(GLfloat) } },
		{ GL_FLOAT_VEC2, { "vec2", sizeof(glm::vec2) } },
		{ GL_FLOAT_VEC3, { "vec3", sizeof(glm::vec3) } },
		{ GL_FLOAT_VEC4, { "vec4", sizeof(glm::vec4) } },
		{ GL_DOUBLE, { "double", sizeof(GLdouble) } },
		{ GL_DOUBLE_VEC2, { "dvec2", sizeof(glm::dvec2) } },
		{ GL_DOUBLE_VEC3, { "dvec3", sizeof(glm::dvec3) } },
		{ GL_DOUBLE_VEC4, { "dvec4", sizeof(glm::dvec4) } },
		{ GL_INT, { "int", sizeof(GLint) } },
		{ GL_INT_VEC2, { "ivec2", sizeof(glm::ivec2) } },
		{ GL_INT_VEC3, { "ivec3", sizeof(glm::ivec3) } },
		{ GL_INT_VEC4, { "ivec4", sizeof(glm::ivec4) } },
		{ GL_UNSIGNED_INT, { "uint", sizeof(GLuint) } },
		{ GL_UNSIGNED_INT_VEC2, { "uvec2", sizeof(glm::uvec2) } },
		{ GL_UNSIGNED_INT_VEC3, { "uvec3", sizeof(glm::uvec3) } },
		{ GL_UNSIGNED_INT_VEC4, { "uvec4", sizeof(glm::uvec4) } },
		{ GL_BOOL, { "bool", sizeof(GLboolean) } },
		{ GL_BOOL_VEC2, { "bvec2", sizeof(glm::bvec2) } },
		{ GL_BOOL_VEC3, { "bvec3", sizeof(glm::bvec3) } },
		{ GL_BOOL_VEC4, { "bvec4", sizeof(glm::bvec4) } },
		{ GL_FLOAT_MAT2, { "mat2", sizeof(glm::mat2) } },
		{ GL_FLOAT_MAT3, { "mat3", sizeof(glm::mat3) } },
		{ GL_FLOAT_MAT4, { "mat4", sizeof(glm::mat4) } },
		{ GL_FLOAT_MAT2x3, { "mat2x3", sizeof(glm::mat2x3) } },
		{ GL_FLOAT_MAT2x4, { "mat2x4", sizeof(glm::mat2x4) } },
		{ GL_FLOAT_MAT3x2, { "mat3x2", sizeof(glm::mat3x2) } },
		{ GL_FLOAT_MAT3x4, { "mat3x4", sizeof(glm::mat3x4) } },
		{ GL_FLOAT_MAT4x2, { "mat4x2", sizeof(glm::mat4x2) } },
		{ GL_FLOAT_MAT4x3, { "mat4x3", sizeof(glm::mat4x3) } },
		{ GL_DOUBLE_MAT2, { "dmat2", sizeof(glm::dmat2) } },
		{ GL_DOUBLE_MAT3, { "dmat3", sizeof(glm::dmat3) } },
		{ GL_DOUBLE_MAT4, { "dmat4", sizeof(glm::dmat4) } },
		{ GL_DOUBLE_MAT2x3, { "dmat2x3", sizeof(glm::dmat2x3) } },
		{ GL_DOUBLE_MAT2x4, { "dmat2x4", sizeof(glm::dmat2x4) } },
		{ GL_DOUBLE_MAT3x2, { "dmat3x2", sizeof(glm::dmat3x2) } },
		{ GL_DOUBLE_MAT3x4, { "dmat3x4", sizeof(glm::dmat3x4) } },
		{ GL_DOUBLE_MAT4x2, { "dmat4x2", sizeof(glm::dmat4x2) } },
		{ GL_DOUBLE_MAT4x3, { "dmat4x3", sizeof(glm::dmat4x3) } },
		{ GL_SAMPLER_1D, { "sampler1D", 0 } },
		{ GL_SAMPLER_2D, { "sampler2D", 0 } },
		{ GL_SAMPLER_3D, { "sampler3D", 0 } },
		{ GL_SAMPLER_CUBE, { "samplerCube", 0 } },
		{ GL_SAMPLER_1D_SHADOW, { "sampler1DShadow", 0 } },
		{ GL_SAMPLER_2D_SHADOW, { "sampler2DShadow", 0 } },
		{ GL_SAMPLER_1D_ARRAY, { "sampler1DArray", 0 } },
		{ GL_SAMPLER_2D_ARRAY, { "sampler2DArray", 0 } },
		{ GL_SAMPLER_1D_ARRAY_SHADOW, { "sampler1DArrayShadow", 0 } },
		{ GL_SAMPLER_2D_ARRAY_SHADOW, { "sampler2DArrayShadow", 0 } },
		{ GL_SAMPLER_2D_MULTISAMPLE, { "sampler2DMS", 0 } },
		{ GL_SAMPLER_2D_MULTISAMPLE_ARRAY, { "sampler2DMSArray", 0 } },
		{ GL_SAMPLER_CUBE_SHADOW, { "samplerCubeShadow", 0 } },
		{ GL_SAMPLER_BUFFER, { "samplerBuffer", 0 } },
		{ GL_SAMPLER_2D_RECT, { "sampler2DRect", 0 } },
		{ GL_SAMPLER_2D_RECT_SHADOW, { "sampler2DRectShadow", 0 } },
		{ GL_INT_SAMPLER_1D, { "isampler1D", 0 } },
		{ GL_INT_SAMPLER_2D, { "isampler2D", 0 } },
		{ GL_INT_SAMPLER_3D, { "isampler3D", 0 } },
		{ GL_INT_SAMPLER_CUBE, { "isamplerCube", 0 } },
		{ GL_INT_SAMPLER_1D_ARRAY, { "isampler1DArray", 0 } },
		{ GL_INT_SAMPLER_2D_ARRAY, { "isampler2DArray", 0 } },
		{ GL_INT_SAMPLER_2D_MULTISAMPLE, { "isampler2DMS", 0 } },
		{ GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, { "isampler2DMSArray", 0 } },
		{ GL_INT_SAMPLER_BUFFER, { "isamplerBuffer", 0 } },
		{ GL_INT_SAMPLER_2D_RECT, { "isampler2DRect", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_1D, { "usampler1D", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_2D, { "usampler2D", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_3D, { "usampler3D", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_CUBE, { "usamplerCube", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_1D_ARRAY, { "usampler1DArray", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_2D_ARRAY, { "usampler2DArray", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE, { "usampler2DMS", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, { "usampler2DMSArray", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_BUFFER, { "usamplerBuffer", 0 } },
		{ GL_UNSIGNED_INT_SAMPLER_2D_RECT, { "usampler2DRect", 0 } },
		{ GL_IMAGE_1D, { "image1D", 0 } },
		{ GL_IMAGE_2D, { "image2D", 0 } },
		{ GL_IMAGE_3D, { "image3D", 0 } },
		{ GL_IMAGE_2D_RECT, { "image2DRect", 0 } },
		{ GL_IMAGE_CUBE, { "imageCube", 0 } },
		{ GL_IMAGE_BUFFER, { "imageBuffer", 0 } },
		{ GL_IMAGE_1D_ARRAY, { "image1DArray", 0 } },
		{ GL_IMAGE_2D_ARRAY, { "image2DArray", 0 } },
		{ GL_IMAGE_CUBE_MAP_ARRAY, { "imageCubeArray", 0 } },
		{ GL_IMAGE_2D_MULTISAMPLE, { "image2DMS", 0 } },
		{ GL_IMAGE_2D_MULTISAMPLE_ARRAY, { "image2DMSArray", 0 } },
		{ GL_INT_IMAGE_1D, { "iimage1D", 0 } },
		{ GL_INT_IMAGE_2D, { "iimage2D", 0 } },
		{ GL_INT_IMAGE_3D, { "iimage3D", 0 } },
		{ GL_INT_IMAGE_2D_RECT, { "iimage2DRect", 0 } },
		{ GL_INT_IMAGE_CUBE, { "iimageCube", 0 } },
		{ GL_INT_IMAGE_BUFFER, { "iimageBuffer", 0 } },
		{ GL_INT_IMAGE_1D_ARRAY, { "iimage1DArray", 0 } },
		{ GL_INT_IMAGE_2D_ARRAY, { "iimage2DArray", 0 } },
		{ GL_INT_IMAGE_CUBE_MAP_ARRAY, { "iimageCubeArray", 0 } },
		{ GL_INT_IMAGE_2D_MULTISAMPLE, { "iimage2DMS", 0 } },
		{ GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY, { "iimage2DMSArray", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_1D, { "uimage1D", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_2D, { "uimage2D", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_3D, { "uimage3D", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_2D_RECT, { "uimage2DRect", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_CUBE, { "uimageCube", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_BUFFER, { "uimageBuffer", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_1D_ARRAY, { "uimage1DArray", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_2D_ARRAY, { "uimage2DArray", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY, { "uimageCubeArray", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE, { "uimage2DMS", 0 } },
		{ GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY, { "uimage2DMSArray", 0 } },
		{ GL_UNSIGNED_INT_ATOMIC_COUNTER, { "atomic_uint", 0 } },
	};

	////////////////////////////////////////////////////////////////////////////////
	std::string dataTypeName(GLenum s)
	{
		auto it = s_dataTypeProperties.find(s);
		return it != s_dataTypeProperties.end() ? it->second.m_name : "Unknown (" + enumToRawString(s) + ")";
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string dataTypeName(GLenum s, size_t arraySize)
	{
		return arraySize == 1 ? dataTypeName(s) :
			arraySize == 0 ? dataTypeName(s) + "[]" :
			dataTypeName(s) + "[" + std::to_string(arraySize) + "]";
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t dataTypeSize(GLenum s)
	{
		auto it = s_dataTypeProperties.find(s);
		return it != s_dataTypeProperties.end() ? it->second.m_size : 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t dataTypeSize(GLenum s, size_t arraySize)
	{
		return arraySize == 1 ? dataTypeSize(s) :
			arraySize == 0 ? dataTypeSize(s) :
			dataTypeSize(s) * arraySize;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t dataTypeSize(GLenum s, size_t arraySize, size_t arrayStride)
	{
		arrayStride = arrayStride ? arrayStride : dataTypeSize(s);
		arraySize = arraySize > 1 ? arraySize : 1;

		return arraySize * arrayStride;
	}

	////////////////////////////////////////////////////////////////////////////////
	struct TextureFormatProperties
	{
		size_t m_channels;
		size_t m_texelSize;
	};

	////////////////////////////////////////////////////////////////////////////////
	std::unordered_map<GLenum, TextureFormatProperties> s_textureFormatProperties =
	{		
		{ GL_R8,                  { 1, 8 } },
		{ GL_R8_SNORM,            { 1, 8 } },
		{ GL_R16,                 { 1, 16 } },
		{ GL_R16_SNORM,           { 1, 16 } },
		{ GL_RG8,                 { 2, 2 * 8 } },
		{ GL_RG8_SNORM,           { 2, 2 * 8 } },
		{ GL_RG16,                { 2, 2 * 16 } },
		{ GL_RG16_SNORM,          { 2, 2 * 16 } },
		{ GL_R3_G3_B2,            { 3, 3 + 3 + 2 } },
		{ GL_RGB4,                { 3, 3 * 4 } },
		{ GL_RGB5,                { 3, 3 * 5 } },
		{ GL_RGB8,                { 3, 3 * 8 } },
		{ GL_RGB8_SNORM,          { 3, 3 * 8 } },
		{ GL_RGB10,               { 3, 3 * 10 } },
		{ GL_RGB12,               { 3, 3 * 12 } },
		{ GL_RGB16_SNORM,         { 4, 3 * 16 } },
		{ GL_RGBA2,               { 4, 4 * 2 } },
		{ GL_RGBA4,               { 4, 4 * 4 } },
		{ GL_RGB5_A1,             { 4, 3 * 5 + 1 } },
		{ GL_RGBA8,               { 4, 4 * 8 } },
		{ GL_RGBA8_SNORM,         { 4, 4 * 8 } },
		{ GL_RGB10_A2,            { 4, 3 * 10 + 2 } },
		{ GL_RGB10_A2UI,          { 4, 3 * 10 + 2 } },
		{ GL_RGBA12,              { 4, 4 * 12 } },
		{ GL_RGBA16,              { 4, 4 * 16 } },
		{ GL_SRGB8,               { 3, 3 * 8 } },
		{ GL_SRGB8_ALPHA8,        { 3, 3 * 8 + 8 } },
		{ GL_R16F,                { 1, 16 } },
		{ GL_RG16F,               { 2, 2 * 16 } },
		{ GL_RGB16F,              { 3, 3 * 16 } },
		{ GL_RGBA16F,             { 4, 4 * 16 } },
		{ GL_R32F,                { 1, 32 } },
		{ GL_RG32F,               { 2, 2 * 32 } },
		{ GL_RGB32F,              { 3, 3 * 32 } },
		{ GL_RGBA32F,             { 4, 4 * 32 } },
		{ GL_R11F_G11F_B10F,      { 3, 11 + 11 + 10 } },
		{ GL_RGB9_E5,             { 4, 3 * 9 + 5 } },
		{ GL_R8I,                 { 1, 8 } },
		{ GL_R8UI,                { 1, 8 } },
		{ GL_R16I,                { 1, 16 } },
		{ GL_R16UI,               { 1, 16 } },
		{ GL_R32I,                { 1, 32 } },
		{ GL_R32UI,               { 1, 32 } },
		{ GL_RG8I,                { 2, 2 * 8 } },
		{ GL_RG8UI,               { 2, 2 * 8 } },
		{ GL_RG16I,               { 2, 2 * 16 } },
		{ GL_RG16UI,              { 2, 2 * 16 } },
		{ GL_RG32I,               { 2, 2 * 32 } },
		{ GL_RG32UI,              { 2, 2 * 32 } },
		{ GL_RGB8I,               { 3, 3 * 8 } },
		{ GL_RGB8UI,              { 3, 3 * 8 } },
		{ GL_RGB16I,              { 3, 3 * 16 } },
		{ GL_RGB16UI,             { 3, 3 * 16 } },
		{ GL_RGB32I,              { 3, 3 * 32 } },
		{ GL_RGB32UI,             { 3, 3 * 32 } },
		{ GL_RGBA8I,              { 4, 4 * 8 } },
		{ GL_RGBA8UI,             { 4, 4 * 8 } },
		{ GL_RGBA16I,             { 4, 4 * 16 } },
		{ GL_RGBA16UI,            { 4, 4 * 16 } },
		{ GL_RGBA32I,             { 4, 4 * 32 } },
		{ GL_RGBA32UI,            { 4, 4 * 32 } },
		{ GL_DEPTH_COMPONENT32F,  { 1, 32 } },
		{ GL_DEPTH_COMPONENT24,   { 1, 24 } },
		{ GL_DEPTH_COMPONENT16,   { 1, 16 } },
		{ GL_DEPTH32F_STENCIL8,   { 2, 32 + 8 } },
		{ GL_DEPTH24_STENCIL8,    { 2, 24 + 8 } },
		{ GL_STENCIL_INDEX8,      { 1, 8 } },
	};

	////////////////////////////////////////////////////////////////////////////////
	size_t textureFormatChannels(GLenum format)
	{
		auto it = s_textureFormatProperties.find(format);
		return it != s_textureFormatProperties.end() ? it->second.m_channels : 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t textureFormatTexelSize(GLenum format)
	{
		auto it = s_textureFormatProperties.find(format);
		return it != s_textureFormatProperties.end() ? it->second.m_texelSize : 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Output message type names
	std::unordered_map<GLenum, std::string> s_logSeverityNames =
	{
		ENUM_STRING_PAIR(GL_DEBUG_SEVERITY_HIGH, "GL_DEBUG_SEVERITY_"),
		ENUM_STRING_PAIR(GL_DEBUG_SEVERITY_MEDIUM, "GL_DEBUG_SEVERITY_"),
		ENUM_STRING_PAIR(GL_DEBUG_SEVERITY_LOW, "GL_DEBUG_SEVERITY_"),
		ENUM_STRING_PAIR(GL_DEBUG_SEVERITY_NOTIFICATION, "GL_DEBUG_SEVERITY_"),
	};

	////////////////////////////////////////////////////////////////////////////////
	// Output message sources
	std::unordered_map<GLenum, std::string> s_logMessageSources =
	{
		{ GL_DEBUG_SOURCE_API,             "API" },
		{ GL_DEBUG_SOURCE_WINDOW_SYSTEM,   "Window System" },
		{ GL_DEBUG_SOURCE_SHADER_COMPILER, "Shader Compiler" },
		{ GL_DEBUG_SOURCE_THIRD_PARTY,     "Third Party" },
		{ GL_DEBUG_SOURCE_APPLICATION,     "Application" },
		{ GL_DEBUG_SOURCE_OTHER,           "Other" },
	};

	std::string logMessageSource(GLenum s)
	{
		auto it = s_logMessageSources.find(s);
		return it != s_logMessageSources.end() ? it->second : "Unknown";
	}

	////////////////////////////////////////////////////////////////////////////////
	// Output message type names
	std::unordered_map<GLenum, std::string> s_logTypeNames =
	{
		{ GL_DEBUG_TYPE_ERROR, "Error" },
		{ GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "Deprecated Behavior" },
		{ GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "Undefined Behavior" },
		{ GL_DEBUG_TYPE_PORTABILITY, "Portability" },
		{ GL_DEBUG_TYPE_PERFORMANCE, "Performance" },
		{ GL_DEBUG_TYPE_MARKER, "Marker" },
		{ GL_DEBUG_TYPE_PUSH_GROUP, "Push Group" },
		{ GL_DEBUG_TYPE_POP_GROUP, "Pop Group" },
		{ GL_DEBUG_TYPE_OTHER, "Other" },
	};

	std::string logTypeName(GLenum s)
	{
		auto it = s_logTypeNames.find(s);
		return it != s_logTypeNames.end() ? it->second : "Unknown";
	}

	////////////////////////////////////////////////////////////////////////////////
	// Output ID to string mapping
	std::string logIdName(GLenum s)
	{
		return enumToString(s);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool shouldLogGLMessage(GLenum severity)
	{
		if      (severity == GL_DEBUG_SEVERITY_HIGH         && glDebugConfig() > 0) return true;
		else if (severity == GL_DEBUG_SEVERITY_MEDIUM       && glDebugConfig() > 1) return true;
		else if (severity == GL_DEBUG_SEVERITY_LOW          && glDebugConfig() > 2) return true;
		else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION && glDebugConfig() > 3) return true;
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	Debug::LogOutputRef makeGLLogOutput(GLenum severity)
	{
		if      (severity == GL_DEBUG_SEVERITY_HIGH         && glDebugConfig() > 0)   return Debug::log_error();
		else if (severity == GL_DEBUG_SEVERITY_MEDIUM       && glDebugConfig() > 1)   return Debug::log_warning();
		else if (severity == GL_DEBUG_SEVERITY_LOW          && glDebugConfig() > 2)   return Debug::log_info();
		else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION && glDebugConfig() > 3)   return Debug::log_debug();

		return Debug::log_null();
	}

	////////////////////////////////////////////////////////////////////////////////
	void glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		if (shouldLogGLMessage(severity))
		{
			Debug::DebugRegion region({ "OpenGL", logMessageSource(source), logTypeName(type), logIdName(id) });
			makeGLLogOutput(severity) << message << Debug::end;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T mipDimensionsFloorImpl(T textureSize, int lodLevel)
	{
		return glm::max(T(1), textureSize / (1 << lodLevel));
	}
	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T mipDimensionsCeilImpl(T textureSize, int lodLevel)
	{
		return glm::max(T(1), (textureSize + (1 << lodLevel) - 1) / (1 << lodLevel));
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	glm::ivec2 mipDimensionsFloor(glm::ivec2 textureSize, int lodLevel)
	{
		return mipDimensionsFloorImpl(textureSize, lodLevel);
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	glm::ivec3 mipDimensionsFloor(glm::ivec3 textureSize, int lodLevel)
	{
		return mipDimensionsFloorImpl(textureSize, lodLevel);
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	glm::ivec3 mipDimensionsFloor(Texture const& texture, int lodLevel)
	{
		return mipDimensionsFloorImpl(glm::ivec3(texture.m_width, texture.m_height, texture.m_depth), lodLevel);
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	glm::ivec2 mipDimensionsCeil(glm::ivec2 textureSize, int lodLevel)
	{
		return mipDimensionsCeilImpl(textureSize, lodLevel);
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	glm::ivec3 mipDimensionsCeil(glm::ivec3 textureSize, int lodLevel)
	{
		return mipDimensionsCeilImpl(textureSize, lodLevel);
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	glm::ivec3 mipDimensionsCeil(Texture const& texture, int lodLevel)
	{
		return mipDimensionsCeilImpl(texture.m_dimensions, lodLevel);
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	glm::ivec2 mipDimensions(glm::ivec2 textureSize, int lodLevel)
	{
		return mipDimensionsFloor(textureSize, lodLevel);
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	glm::ivec3 mipDimensions(glm::ivec3 textureSize, int lodLevel)
	{
		return mipDimensionsFloor(textureSize, lodLevel);
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	glm::ivec3 mipDimensions(Texture const& texture, int lodLevel)
	{
		return mipDimensionsFloor(texture, lodLevel);
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	int numMipLevels(int textureSize)
	{
		return glm::ceil(glm::log2((float)textureSize));
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	int numMipLevels(glm::ivec2 textureSize)
	{
		return glm::ceil(glm::log2((float)glm::max(textureSize.x, textureSize.y)));
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	int numMipLevels(glm::ivec3 textureSize)
	{
		return glm::ceil(glm::log2((float)glm::max(glm::max(textureSize.x, textureSize.y), textureSize.z)));
	}

	////////////////////////////////////////////////////////////////////////////////
	/* Returns the number of mip levels corresponding to the parameter texture size. */
	int numMipLevels(Texture const& texture)
	{
		return numMipLevels(texture.m_dimensions);
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t numTexels(glm::ivec3 dimensions, const int baseLevel, const int numLevels, const int maxLevel)
	{
		size_t result = 0;
		for (int mipLevel = baseLevel; (mipLevel < baseLevel + numLevels) && mipLevel <= maxLevel; ++mipLevel)
		{
			const glm::ivec3 mipDims = mipDimensions(dimensions, mipLevel);
			result += size_t(mipDims.x) * size_t(mipDims.y) * size_t(mipDims.z);
		}
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t numTexels(Texture const& texture, const int baseLevel, const int numLevels)
	{
		const size_t mipLevels = texture.m_mipmapped ? numMipLevels(texture) : 1;
		return numTexels(texture.m_dimensions, baseLevel, numLevels <= 0 ? mipLevels : numLevels, mipLevels);
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t textureSizeBytes(glm::ivec3 dimensions, GLenum format, const int baseLevel, const int numLevels)
	{
		return numTexels(dimensions, baseLevel, numLevels, numMipLevels(dimensions)) * ((textureFormatTexelSize(format) + 7) / 8);
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t textureSizeBytes(Texture const& texture, const int baseLevel, const int numLevels)
	{
		return numTexels(texture, baseLevel, numLevels) * ((textureFormatTexelSize(texture.m_format) + 7) / 8);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionUniform> inspectProgramUniforms(GLuint program)
	{
		std::vector<ShaderInspectionUniform> result;

		// Print all the regular uniforms
		GLint numUniforms = 0;
		glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms);
		const GLenum properties[] = { GL_BLOCK_INDEX, GL_TYPE, GL_NAME_LENGTH, GL_LOCATION, GL_ARRAY_SIZE, GL_ARRAY_STRIDE };

		for (int unif = 0; unif < numUniforms; ++unif)
		{
			// Query the properties of the uniform
			GLint values[ARRAY_SIZE(properties)];
			glGetProgramResourceiv(program, GL_UNIFORM, unif, ARRAY_SIZE(properties), properties, ARRAY_SIZE(values), NULL, values);

			// Skip any uniforms that are in a block.
			if (values[0] != -1)
				continue;

			// Get the name
			std::vector<char> nameData(values[2]);
			glGetProgramResourceName(program, GL_UNIFORM, unif, nameData.size(), NULL, &nameData[0]);
			
			ShaderInspectionUniform uniform;
			uniform.m_name = std::string(nameData.begin(), nameData.end() - 1);
			uniform.m_location = values[3];
			uniform.m_type = values[1];
			uniform.m_isArray = values[4] != 1;
			uniform.m_arraySize = values[4];
			uniform.m_arrayStride = values[5];
			result.push_back(uniform);
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionUniformBlock> inspectProgramUniformBlocks(GLuint program)
	{
		// Print all the uniform blocks
		GLint numBlocks = 0;
		glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numBlocks);
		const GLenum blockProperties[] = { GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH, GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE };
		const GLenum activeUnifProp[] = { GL_ACTIVE_VARIABLES };
		const GLenum unifProperties[] = { GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE, GL_OFFSET, GL_ARRAY_STRIDE };

		std::vector<ShaderInspectionUniformBlock> result(numBlocks);

		for (int blockIx = 0; blockIx < numBlocks; ++blockIx)
		{
			// Query the properties of the block
			GLint blockValues[ARRAY_SIZE(blockProperties)];
			glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, blockIx, ARRAY_SIZE(blockProperties), blockProperties, ARRAY_SIZE(blockValues), NULL, blockValues);

			// Qury the name of the block
			std::vector<char> nameData(blockValues[1]);
			glGetProgramResourceName(program, GL_UNIFORM_BLOCK, blockIx, nameData.size(), NULL, &nameData[0]);
			std::string name(nameData.begin(), nameData.end() - 1);

			result[blockIx].m_name = std::string(nameData.begin(), nameData.end() - 1);
			result[blockIx].m_binding = blockValues[2];
			result[blockIx].m_dataSize = blockValues[3];

			// Skip the rest if the block is empty
			if (blockValues[0] == 0)
				continue;

			std::vector<GLint> blockUnifs(blockValues[0]);
			glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, blockIx, 1, activeUnifProp, blockValues[0], NULL, &blockUnifs[0]);

			result[blockIx].m_variables = std::vector<ShaderInspectionUniformBlock::Variable>(blockValues[0]);
			for (int unifIx = 0; unifIx < blockValues[0]; ++unifIx)
			{
				GLint unifValues[ARRAY_SIZE(unifProperties)];
				glGetProgramResourceiv(program, GL_UNIFORM, blockUnifs[unifIx], ARRAY_SIZE(unifProperties), unifProperties, ARRAY_SIZE(unifValues), NULL, unifValues);

				// Get the name.
				std::vector<char> nameData(unifValues[0]);
				glGetProgramResourceName(program, GL_UNIFORM, blockUnifs[unifIx], nameData.size(), NULL, &nameData[0]);
				std::string name(nameData.begin(), nameData.end() - 1);

				result[blockIx].m_variables[unifIx].m_name = std::string(nameData.begin(), nameData.end() - 1);
				result[blockIx].m_variables[unifIx].m_type = unifValues[1];
				result[blockIx].m_variables[unifIx].m_byteOffset = unifValues[3];
				result[blockIx].m_variables[unifIx].m_isArray = unifValues[2] != 1;
				result[blockIx].m_variables[unifIx].m_arraySize = unifValues[2];
				result[blockIx].m_variables[unifIx].m_arrayStride = unifValues[4];
			}

			// Sort them by start offset, such that we can compute padding
			std::sort(result[blockIx].m_variables.begin(), result[blockIx].m_variables.end(), [](auto const& a, auto const& b) { return a.m_byteOffset < b.m_byteOffset; });

			// Compute the padding values
			for (int unifIx = 0; unifIx < blockValues[0] - 1; ++unifIx)
			{
				// Current variable
				ShaderInspectionUniformBlock::Variable& variable = result[blockIx].m_variables[unifIx];

				// What is the next expected start position
				GLuint expectedNextOffset = variable.m_byteOffset + GPU::dataTypeSize(variable.m_type, variable.m_arraySize, variable.m_arrayStride);

				// How much padding is added
				variable.m_padding = result[blockIx].m_variables[unifIx + 1].m_byteOffset - expectedNextOffset;
			}
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionShaderStorageBlock> inspectProgramShaderStorageBlocks(GLuint program)
	{
		// Print all the uniform blocks
		GLint numBlocks = 0;
		glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &numBlocks);
		const GLenum blockProperties[] = { GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH, GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE };
		const GLenum activeUnifProp[] = { GL_ACTIVE_VARIABLES };
		const GLenum unifProperties[] = { GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE, GL_OFFSET, GL_ARRAY_STRIDE, GL_TOP_LEVEL_ARRAY_SIZE, GL_TOP_LEVEL_ARRAY_STRIDE };

		std::vector<ShaderInspectionShaderStorageBlock> result(numBlocks);

		for (int blockIx = 0; blockIx < numBlocks; ++blockIx)
		{
			// Query the properties of the block
			GLint blockValues[ARRAY_SIZE(blockProperties)];
			glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, blockIx, ARRAY_SIZE(blockProperties), blockProperties, ARRAY_SIZE(blockValues), NULL, blockValues);

			// Qury the name of the block
			std::vector<char> nameData(blockValues[1]);
			glGetProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, blockIx, nameData.size(), NULL, &nameData[0]);
			std::string name(nameData.begin(), nameData.end() - 1);

			result[blockIx].m_name = std::string(nameData.begin(), nameData.end() - 1);
			result[blockIx].m_binding = blockValues[2];
			result[blockIx].m_dataSize = blockValues[3];

			// Skip the rest if the block is empty
			if (blockValues[0] == 0)
				continue;

			std::vector<GLint> blockUnifs(blockValues[0]);
			glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, blockIx, 1, activeUnifProp, blockValues[0], NULL, &blockUnifs[0]);

			result[blockIx].m_variables = std::vector<ShaderInspectionShaderStorageBlock::Variable>(blockValues[0]);
			for (int unifIx = 0; unifIx < blockValues[0]; ++unifIx)
			{
				GLint unifValues[ARRAY_SIZE(unifProperties)];
				glGetProgramResourceiv(program, GL_BUFFER_VARIABLE, blockUnifs[unifIx], ARRAY_SIZE(unifProperties), unifProperties, ARRAY_SIZE(unifValues), NULL, unifValues);

				// Get the name.
				std::vector<char> nameData(unifValues[0]);
				glGetProgramResourceName(program, GL_BUFFER_VARIABLE, blockUnifs[unifIx], nameData.size(), NULL, &nameData[0]);
				std::string name(nameData.begin(), nameData.end() - 1);

				result[blockIx].m_variables[unifIx].m_name = std::string(nameData.begin(), nameData.end() - 1);
				result[blockIx].m_variables[unifIx].m_type = unifValues[1];
				result[blockIx].m_variables[unifIx].m_byteOffset = unifValues[3];
				result[blockIx].m_variables[unifIx].m_isArray = unifValues[2] != 1;
				result[blockIx].m_variables[unifIx].m_arraySize = unifValues[2];
				result[blockIx].m_variables[unifIx].m_arrayStride = unifValues[4];
				result[blockIx].m_variables[unifIx].m_topLevelArraySize = unifValues[5];
				result[blockIx].m_variables[unifIx].m_topLevelArrayStride = unifValues[6];
			}

			// Sort them by start offset, such that we can compute padding
			std::sort(result[blockIx].m_variables.begin(), result[blockIx].m_variables.end(), [](auto const& a, auto const& b) { return a.m_byteOffset < b.m_byteOffset; });

			// Compute the padding values
			for (int unifIx = 0; unifIx < blockValues[0] - 1; ++unifIx)
			{
				// Current variable
				ShaderInspectionShaderStorageBlock::Variable& variable = result[blockIx].m_variables[unifIx];

				// What is the next expected start position
				GLuint expectedNextOffset = variable.m_byteOffset + GPU::dataTypeSize(variable.m_type, variable.m_arraySize, variable.m_arrayStride);

				// How much padding is added
				variable.m_padding = result[blockIx].m_variables[unifIx + 1].m_byteOffset - expectedNextOffset;
			}
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionProgramInput> inspectProgramInputs(GLuint program)
	{
		std::vector<ShaderInspectionProgramInput> result;

		// Print all the regular uniforms
		GLint numInputs = 0;
		glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &numInputs);
		const GLenum properties[] = 
		{ 
			GL_TYPE, GL_NAME_LENGTH, GL_LOCATION, GL_ARRAY_SIZE, 
			GL_REFERENCED_BY_TESS_CONTROL_SHADER, GL_REFERENCED_BY_TESS_EVALUATION_SHADER, 
			GL_REFERENCED_BY_VERTEX_SHADER, GL_REFERENCED_BY_GEOMETRY_SHADER, 
			GL_REFERENCED_BY_FRAGMENT_SHADER, GL_REFERENCED_BY_COMPUTE_SHADER
		};

		for (int iid = 0; iid < numInputs; ++iid)
		{
			// Query the properties of the variable
			GLint values[ARRAY_SIZE(properties)];
			glGetProgramResourceiv(program, GL_PROGRAM_INPUT, iid, ARRAY_SIZE(properties), properties, ARRAY_SIZE(values), NULL, values);

			// Get the name
			std::vector<char> nameData(values[1]);
			glGetProgramResourceName(program, GL_PROGRAM_INPUT, iid, nameData.size(), NULL, &nameData[0]);

			ShaderInspectionProgramInput input;
			input.m_name = std::string(nameData.begin(), nameData.end() - 1);
			input.m_location = values[2];
			input.m_type = values[0];
			input.m_isArray = values[3] != 1;
			input.m_arraySize = values[3];
			input.m_referectedByTCS = values[4] == 1;
			input.m_referectedByTES = values[5] == 1;
			input.m_referectedByVS = values[6] == 1;
			input.m_referectedByGS = values[7] == 1;
			input.m_referectedByFS = values[8] == 1;
			input.m_referectedByCS = values[9] == 1;
			result.push_back(input);
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionProgramOutput> inspectProgramOutputs(GLuint program)
	{
		std::vector<ShaderInspectionProgramOutput> result;

		// Print all the regular uniforms
		GLint numOutputs = 0;
		glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &numOutputs);
		const GLenum properties[] =
		{
			GL_TYPE, GL_NAME_LENGTH, GL_LOCATION, GL_ARRAY_SIZE,
			GL_REFERENCED_BY_TESS_CONTROL_SHADER, GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
			GL_REFERENCED_BY_VERTEX_SHADER, GL_REFERENCED_BY_GEOMETRY_SHADER,
			GL_REFERENCED_BY_FRAGMENT_SHADER, GL_REFERENCED_BY_COMPUTE_SHADER
		};

		for (int oid = 0; oid < numOutputs; ++oid)
		{
			// Query the properties of the variable
			GLint values[ARRAY_SIZE(properties)];
			glGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, oid, ARRAY_SIZE(properties), properties, ARRAY_SIZE(values), NULL, values);

			// Get the name
			std::vector<char> nameData(values[1]);
			glGetProgramResourceName(program, GL_PROGRAM_OUTPUT, oid, nameData.size(), NULL, &nameData[0]);

			ShaderInspectionProgramOutput output;
			output.m_name = std::string(nameData.begin(), nameData.end() - 1);
			output.m_location = values[2];
			output.m_type = values[0];
			output.m_isArray = values[3] != 1;
			output.m_arraySize = values[3];
			output.m_referectedByTCS = values[4] == 1;
			output.m_referectedByTES = values[5] == 1;
			output.m_referectedByVS = values[6] == 1;
			output.m_referectedByGS = values[7] == 1;
			output.m_referectedByFS = values[8] == 1;
			output.m_referectedByCS = values[9] == 1;
			result.push_back(output);
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	using CapabilityExtractor = std::function<void(CapabilityAttrib&)>;

	////////////////////////////////////////////////////////////////////////////////
	template<typename V, typename S, typename I>
	CapabilityExtractor makeExtractorFn(S singleFn, I indexedFn)
	{
		return CapabilityExtractor([=](CapabilityAttrib& capability)
		{
			bool single = capability.m_count <= 1;
			capability.m_values.resize(glm::max(capability.m_count, 1));
			for (size_t i = 0; i < capability.m_values.size(); ++i)
			{
				V value;				
				if (single)  singleFn(capability.m_enum, &value);
				else         indexedFn(capability.m_enum, i, &value);
				capability.m_values[i] = value;
			}
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	void queryGpuCapabilites()
	{
		////////////////////////////////////////////////////////////////////////////////
		std::unordered_map<GLenum, CapabilityExtractor> capabilityExtractors =
		{
			{ GL_BOOL, makeExtractorFn<GLboolean>(glGetBooleanv, glGetBooleani_v) },
			{ GL_INT, makeExtractorFn<GLint>(glGetIntegerv, glGetIntegeri_v) },
			{ GL_INT64_ARB, makeExtractorFn<GLint64>(glGetInteger64v, glGetInteger64i_v) },
			{ GL_FLOAT, makeExtractorFn<GLfloat>(glGetFloatv, glGetFloati_v) },
			{ GL_DOUBLE, makeExtractorFn<GLdouble>(glGetDoublev, glGetDoublei_v) },
		};

		// Process each category
		for (auto& capabilityCategoryIt : GPU::s_capabilityAttribs)
		{
			auto& [category, capabilities] = capabilityCategoryIt;

			// Process each capability
			for (auto& capabilityIt : capabilities)
			{
				auto& [capabilityId, capability] = capabilityIt;

				Debug::log_trace() << "Querying capability: " << enumToString(capability.m_enum) << Debug::end;

				// Invoke the appropriate extractor, if any
				if (auto extractorIt = capabilityExtractors.find(capability.m_type); extractorIt != capabilityExtractors.end())
				{
					auto const& extractor = extractorIt->second;
					extractor(capability);
				}
				else
				{
					Debug::log_warning() << "Encountered an unknown capability type: " << capability.m_type << Debug::end;
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Queries a capability attribute. */
	void init()
	{
		// Process the capability attributes
		queryGpuCapabilites();
	}
}