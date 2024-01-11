#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Constants.h"
#include "Config.h"
#include "Preprocessor.h"
#include "BVH.h"

////////////////////////////////////////////////////////////////////////////////
/// GPU STRUCTURES
////////////////////////////////////////////////////////////////////////////////
namespace GPU
{
	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer indices. */	
	meta_enum(UniformBufferIndices, int,
		UNIFORM_BUFFER_RENDER = 0,
		UNIFORM_BUFFER_CAMERA = 1,
		UNIFORM_BUFFER_GENERIC_1 = 2,
		UNIFORM_BUFFER_GENERIC_2 = 3,
		UNIFORM_BUFFER_GENERIC_3 = 4,
		UNIFORM_BUFFER_GENERIC_4 = 5,
		UNIFORM_BUFFER_GENERIC_5 = 6,
		UNIFORM_BUFFER_GENERIC_6 = 7,
		UNIFORM_BUFFER_GENERIC_7 = 8,
		UNIFORM_BUFFER_GENERIC_8 = 9,
		UNIFORM_BUFFER_GENERIC_9 = 10,
		UNIFORM_BUFFER_GENERIC_10 = 11,
		UNIFORM_BUFFER_GENERIC_11 = 12,
		UNIFORM_BUFFER_GENERIC_12 = 13,
		UNIFORM_BUFFER_GENERIC_13 = 14,
		UNIFORM_BUFFER_GENERIC_14 = 15,
		UNIFORM_BUFFER_GENERIC_15 = 16,
		UNIFORM_BUFFER_GENERIC_16 = 17
	);

	////////////////////////////////////////////////////////////////////////////////
	/** Vertex attribute indices. */
	meta_enum(VertexAttribIndices, int,
		VERTEX_ATTRIB_POSITION = 0,
		VERTEX_ATTRIB_NORMAL = 1,
		VERTEX_ATTRIB_TANGENT = 2,
		VERTEX_ATTRIB_BITANGENT = 3,
		VERTEX_ATTRIB_UV = 4,
		VERTEX_ATTRIB_COLOR = 5
	);

	////////////////////////////////////////////////////////////////////////////////
	/** Texture indices. */
	meta_enum(TextureIndices, int,
		TEXTURE_DEPTH = 0,
		TEXTURE_ALBEDO_MAP = 1,
		TEXTURE_NORMAL_MAP = 2,
		TEXTURE_SPECULAR_MAP = 3,
		TEXTURE_ALPHA_MAP = 4,
		TEXTURE_DISPLACEMENT_MAP = 5,
		TEXTURE_SHADOW_MAP = 6,
		TEXTURE_POST_PROCESS_1 = 7,
		TEXTURE_POST_PROCESS_2 = 8,
		TEXTURE_POST_PROCESS_3 = 9,
		TEXTURE_POST_PROCESS_4 = 10,
		TEXTURE_POST_PROCESS_5 = 11,
		TEXTURE_POST_PROCESS_6 = 12,
		TEXTURE_POST_PROCESS_7 = 13,
		TEXTURE_POST_PROCESS_8 = 14
	);

	////////////////////////////////////////////////////////////////////////////////
	/** Texture enums. */
	meta_enum(TextureEnums, int,
		TEXTURE_DEPTH_ENUM = GL_TEXTURE0,
		TEXTURE_ALBEDO_MAP_ENUM = GL_TEXTURE1,
		TEXTURE_NORMAL_MAP_ENUM = GL_TEXTURE2,
		TEXTURE_SPECULAR_MAP_ENUM = GL_TEXTURE3,
		TEXTURE_ALPHA_MAP_ENUM = GL_TEXTURE4,
		TEXTURE_DISPLACEMENT_MAP_ENUM = GL_TEXTURE5,
		TEXTURE_SHADOW_MAP_ENUM = GL_TEXTURE6,
		TEXTURE_POST_PROCESS_1_ENUM = GL_TEXTURE7,
		TEXTURE_POST_PROCESS_2_ENUM = GL_TEXTURE8,
		TEXTURE_POST_PROCESS_3_ENUM = GL_TEXTURE9,
		TEXTURE_POST_PROCESS_4_ENUM = GL_TEXTURE10,
		TEXTURE_POST_PROCESS_5_ENUM = GL_TEXTURE11,
		TEXTURE_POST_PROCESS_6_ENUM = GL_TEXTURE12,
		TEXTURE_POST_PROCESS_7_ENUM = GL_TEXTURE13,
		TEXTURE_POST_PROCESS_8_ENUM = GL_TEXTURE14
	);

	////////////////////////////////////////////////////////////////////////////////
	/** Trace configs. */
	meta_enum(TraceLevels, int,
		Off  = 0,
		Std = 1,
		Basic = 2,
		Full = 3
	);

	////////////////////////////////////////////////////////////////////////////////
	/** GL debug configs. */
	meta_enum(GLDebugConfigs, int,
		GLD_OFF = 0,
		GLD_ALL = 1
	);

	////////////////////////////////////////////////////////////////////////////////
	/** Name of the default renderer */
	std::string renderer();

	////////////////////////////////////////////////////////////////////////////////
	/** Should we allow optional extensions or not. */
	bool enableOptionalExtensions();

	////////////////////////////////////////////////////////////////////////////////
	/** Default resolution. */
	glm::ivec2 defaultResolution();

	////////////////////////////////////////////////////////////////////////////////
	/** Maximum resolution. */
	glm::ivec2 maxResolution();

	////////////////////////////////////////////////////////////////////////////////
	/** Number of gbuffer layers. */
	int numLayers();

	////////////////////////////////////////////////////////////////////////////////
	/** Number of MSAA samples. */
	int numMsaaSamples();

	////////////////////////////////////////////////////////////////////////////////
	/** Voxel grid size. */
	int voxelGridSize();

	////////////////////////////////////////////////////////////////////////////////
	/** Shadow map resolution. */
	int shadowMapResolution();

	////////////////////////////////////////////////////////////////////////////////
	/** Whether we should trace the GL api or not. */
	int glTraceLevel();

	////////////////////////////////////////////////////////////////////////////////
	/** Whether we should trace the GL api or not. */
	int glDebugConfig();

	////////////////////////////////////////////////////////////////////////////////
	void glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

	////////////////////////////////////////////////////////////////////////////////
	// Various GPU structure layout types available
	meta_enum(GpuStructLayout, int, std140, std430);

	////////////////////////////////////////////////////////////////////////////////
	template<typename StructType>
	struct GpuStructMember
	{
		std::string_view type;
		std::string_view name;
		size_t arr_size = {};
		size_t index = {};
		size_t alignment = {};
		size_t size = {};
	};

	////////////////////////////////////////////////////////////////////////////////
	template<typename StructTypeIn, size_t size>
	struct GpuStruct
	{
		using StructType = StructTypeIn;

		std::string_view name;
		GpuStructLayout layout = GpuStructLayout::std140;
		size_t alignment = {};
		std::array<GpuStructMember<StructType>, size> members = {};
	};

	////////////////////////////////////////////////////////////////////////////////
	namespace impl
	{
		////////////////////////////////////////////////////////////////////////////////
		struct GpuStructTag {};
		template<size_t Alignment> struct alignas(Alignment) GpuStructBase : public GpuStructTag
		{
			static const size_t _Alignment = Alignment;
		};

		////////////////////////////////////////////////////////////////////////////////
		// Helper struct for determining array sizes
		////////////////////////////////////////////////////////////////////////////////

		template<typename T> struct ArraySize { static constexpr size_t size = 0; };
		template<typename T, size_t s> struct ArraySize<T[s]> { static constexpr size_t size = s; };

		////////////////////////////////////////////////////////////////////////////////
		// Helper struct for declaring data types
		////////////////////////////////////////////////////////////////////////////////

		template<typename T> struct StructTypeBase { using member_type = T; };

		template<GpuStructLayout L, typename T, typename = void> struct StructType : public StructTypeBase<T> {};

		constexpr size_t roundToVec4(size_t s)
		{
			size_t sv4 = sizeof(glm::vec4);
			return std::min(((s + sv4 - 1) / sv4) * sv4, sv4);
		}

		template<typename T, size_t Alignment>
		struct alignas(Alignment) AlignedArrElement
		{
			T value;
			AlignedArrElement() : value() {}
			AlignedArrElement(T valueIn) : value(valueIn) {}
			operator T& () { return value; }
			operator const T& () const { return value; }
		};

		////////////////////////////////////////////////////////////////////////////////
		// Helper struct for defining layouts
		template<size_t A, size_t S> struct LayoutInfoBase
		{
			static constexpr size_t alignment = A;
			static constexpr size_t size = S;
		};

		// Layout info template base
		template<GpuStructLayout L, typename T, typename Enable = void>
		struct LayoutInfo {};

		////////////////////////////////////////////////////////////////////////////////
		// Layout info for std140
		////////////////////////////////////////////////////////////////////////////////

		// Aligned array element type
		template<typename T, size_t s> struct StructType<std140, T[s], typename std::enable_if<std::is_fundamental<T>::value, void>::type> :
			public StructTypeBase<std::array<AlignedArrElement<T, roundToVec4(sizeof(T))>, s>> {};

		template<typename T, size_t s> struct StructType<std140, T[s], typename std::enable_if<std::is_base_of<GpuStructTag, T>::value, void>::type> :
			public StructTypeBase<std::array<T, s>> {};

		// Structs
		template<typename T> struct LayoutInfo<std140, T, typename std::enable_if<std::is_base_of<GpuStructTag, T>::value, void>::type> :
			public LayoutInfoBase<T::_Alignment, sizeof(T)> {};

		// Scalars
		template<typename T>
		struct LayoutInfo<std140, T, typename std::enable_if<std::is_fundamental<T>::value, void>::type> : public LayoutInfoBase<sizeof(T), sizeof(T)> {};

		// Vectors
		#define LAYOUT_INFO_std140_VECTOR(L, T, A) \
			template<typename V> struct LayoutInfo<L, T<V>, void> : public LayoutInfoBase<sizeof(A<V>), sizeof(T<V>)> {}

		LAYOUT_INFO_std140_VECTOR(std140, glm::tvec1, glm::tvec1);
		LAYOUT_INFO_std140_VECTOR(std140, glm::tvec2, glm::tvec2);
		LAYOUT_INFO_std140_VECTOR(std140, glm::tvec3, glm::tvec4);
		LAYOUT_INFO_std140_VECTOR(std140, glm::tvec4, glm::tvec4);

		// Arrays
		template<typename T, size_t s> struct LayoutInfo<std140, T[s], void> :
			public LayoutInfoBase<roundToVec4(LayoutInfo<std140, T>::alignment), roundToVec4(sizeof(T))* s> {};

		// Matrices
		#define LAYOUT_INFO_std140_MATRIX(L, T, A, C, R) \
			template<typename V> struct LayoutInfo<L, T<V>, void> : public LayoutInfo<L, BOOST_PP_CAT(glm::tvec, C)<V>[R]> {}

		LAYOUT_INFO_std140_MATRIX(std140, glm::tmat2x2, glm::tmat2x2, 2, 2);
		LAYOUT_INFO_std140_MATRIX(std140, glm::tmat2x3, glm::tmat2x3, 2, 3);
		LAYOUT_INFO_std140_MATRIX(std140, glm::tmat2x4, glm::tmat2x4, 2, 4);
		LAYOUT_INFO_std140_MATRIX(std140, glm::tmat3x2, glm::tmat3x2, 3, 2);
		LAYOUT_INFO_std140_MATRIX(std140, glm::tmat3x3, glm::tmat3x3, 3, 3);
		LAYOUT_INFO_std140_MATRIX(std140, glm::tmat3x4, glm::tmat3x4, 3, 4);
		LAYOUT_INFO_std140_MATRIX(std140, glm::tmat4x2, glm::tmat4x2, 4, 2);
		LAYOUT_INFO_std140_MATRIX(std140, glm::tmat4x3, glm::tmat4x3, 4, 3);
		LAYOUT_INFO_std140_MATRIX(std140, glm::tmat4x4, glm::tmat4x4, 4, 4);

		////////////////////////////////////////////////////////////////////////////////
		// Metadata generation
		////////////////////////////////////////////////////////////////////////////////

		constexpr size_t nextMemberStart(size_t start, std::string_view memberString)
		{
			for (; start < memberString.size() && (memberString[start] != '('); ++start)
				;
			return start;
		}

		constexpr size_t nextMemberEnd(size_t start, std::string_view memberString)
		{
			for (; start < memberString.size() && (memberString[start] != ')'); ++start)
				;
			return start;
		}

		constexpr size_t nextMemberComma(size_t start, std::string_view memberString)
		{
			for (; start < memberString.size() && (memberString[start] != ','); ++start)
				;
			return start;
		}

		constexpr bool isAllowedTypeChar(char c)
		{
			return (c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				c == '_' || c == ':';
		}

		constexpr bool isAllowedArrayChar(char c)
		{
			return (c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				c == '_' || c == ':' || c == '[' || c == ']';
		}

		constexpr bool isAllowedNameChar(char c)
		{
			return (c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				c == '_';
		}

		constexpr size_t parseAlignment(std::initializer_list<size_t> alignments)
		{
			return *std::max_element(alignments.begin(), alignments.end());
		}

		template <typename StructType, size_t size>
		constexpr GpuStruct<StructType, size> parseStruct(std::string_view name, std::string_view in, GpuStructLayout layout,
			std::array<size_t, size> const& arraySizes, std::array<size_t, size> const& alignments, std::array<size_t, size> const& sizes)
		{
			GpuStruct<StructType, size> result;
			result.name = name;
			result.layout = layout;
			result.alignment = StructType::_Alignment;

			std::array<std::string_view, size> memberStrings;
			size_t amountFilled = 0;

			size_t currentStringStart = 0;
			size_t currentStringEnd = 0;
			size_t currentStringSize = 0;

			while (amountFilled < size)
			{
				currentStringStart = nextMemberStart(currentStringStart, in);
				currentStringEnd = nextMemberEnd(currentStringStart + 1, in);
				currentStringSize = currentStringEnd - currentStringStart + 1;

				memberStrings[amountFilled] = std::string_view(in.data() + currentStringStart, currentStringSize);
				currentStringStart += currentStringSize;
				++amountFilled;
			}

			for (size_t i = 0; i < size; ++i)
			{
				result.members[i].index = i;
				result.members[i].arr_size = arraySizes[i];
				result.members[i].alignment = alignments[i];
				result.members[i].size = sizes[i];

				std::string_view memberString = memberStrings[i];

				// Parse the member type
				size_t typeStart = 0;
				while (!isAllowedTypeChar(*(memberString.data() + typeStart)))
					++typeStart;

				size_t typeSize = 0;
				while (isAllowedTypeChar(*(memberString.data() + typeStart + typeSize)))
					++typeSize;

				result.members[i].type = std::string_view(memberString.data() + typeStart, typeSize);

				// Parse the member array size
				size_t arraySizeStart = 0;
				while (!isAllowedArrayChar(*(memberString.data() + arraySizeStart)))
					++arraySizeStart;

				size_t arraySizeSize = 0;
				while (isAllowedArrayChar(*(memberString.data() + arraySizeStart + arraySizeSize)))
					++arraySizeSize;

				// Parse the member name
				size_t nameStart = arraySizeStart + arraySizeSize + 1;
				while (!isAllowedNameChar(*(memberString.data() + nameStart)))
					++nameStart;

				size_t nameSize = 0;
				while (isAllowedNameChar(*(memberString.data() + nameStart + nameSize)))
					++nameSize;

				result.members[i].name = std::string_view(memberString.data() + nameStart, nameSize);
			}

			return result;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	// Turns the input zipped sequence to a correct sequence of tuples
	// Based on: https://stackoverflow.com/questions/26475453/how-to-use-boostpreprocessor-to-unzip-a-sequence
	#define __AUXILIARY_0(...) ((__VA_ARGS__)) __AUXILIARY_1
	#define __AUXILIARY_1(...) ((__VA_ARGS__)) __AUXILIARY_0
	#define __AUXILIARY_0_END
	#define __AUXILIARY_1_END
	#define __REMOVE_PARENTHESES(...) __VA_ARGS__
	#define __INPUT_SEQ(zipped) BOOST_PP_SEQ_POP_FRONT(BOOST_PP_CAT(__AUXILIARY_0(0)zipped,_END))
	#define __TYPE(member) BOOST_PP_TUPLE_ELEM(2, 0, member)
	#define __NAME(member) BOOST_PP_TUPLE_ELEM(2, 1, member)
	#define __LAYOUT_INFO(layout, type, name) GPU::impl::LayoutInfo<layout, type>
	#define __ARRAY_SIZE(type, name) GPU::impl::ArraySize<type>::size
	#define __EXTRACT_ALIGNMENT(r, layout, i, member) BOOST_PP_COMMA_IF(i) __LAYOUT_INFO(layout, __TYPE(member), __NAME(member))::alignment
	#define __EXTRACT_ALIGNMENTS(layout, members) { BOOST_PP_SEQ_FOR_EACH_I(__EXTRACT_ALIGNMENT, layout, members) }
	#define __EXTRACT_SIZE(r, layout, i, member) BOOST_PP_COMMA_IF(i) __LAYOUT_INFO(layout, __TYPE(member), __NAME(member))::size
	#define __EXTRACT_SIZES(layout, members) { BOOST_PP_SEQ_FOR_EACH_I(__EXTRACT_SIZE, layout, members) }
	#define __EXTRACT_ARRAY_SIZE(r, layout, i, member) BOOST_PP_COMMA_IF(i) __ARRAY_SIZE(__TYPE(member), __NAME(member))
	#define __EXTRACT_ARRAY_SIZES(layout, members) { BOOST_PP_SEQ_FOR_EACH_I(__EXTRACT_ARRAY_SIZE, layout, members) }

	////////////////////////////////////////////////////////////////////////////////
	#define _make_struct_layout(layout) BOOST_PP_CAT(GPU::, layout)
	#define _make_struct_alignment(layout, members) GPU::impl::parseAlignment(__EXTRACT_ALIGNMENTS(layout, members))
	#define _make_struct_element_impl(type, name, layout) alignas(__LAYOUT_INFO(layout, type, name)::alignment) GPU::impl::StructType<layout, type>::member_type name;
	#define _make_struct_element_cb(r, layout, member) _make_struct_element_impl(__TYPE(member), __NAME(member), layout)
	#define _make_struct_elements(layout, members) BOOST_PP_SEQ_FOR_EACH(_make_struct_element_cb, layout, members)

	////////////////////////////////////////////////////////////////////////////////
	#define make_gpu_struct(NAME, LAYOUT, ...) \
		struct NAME : public GPU::impl::GpuStructBase<_make_struct_alignment(_make_struct_layout(LAYOUT), __INPUT_SEQ(__VA_ARGS__))> \
		{ \
            _make_struct_elements(_make_struct_layout(LAYOUT), __INPUT_SEQ(__VA_ARGS__)) \
		}; \
    constexpr static auto NAME##_meta = GPU::impl::parseStruct<NAME, BOOST_PP_SEQ_SIZE(__VA_ARGS__)>(\
        #NAME, #__VA_ARGS__, _make_struct_layout(LAYOUT), \
        __EXTRACT_ARRAY_SIZES(_make_struct_layout(LAYOUT), __INPUT_SEQ(__VA_ARGS__)), \
        __EXTRACT_ALIGNMENTS(_make_struct_layout(LAYOUT), __INPUT_SEQ(__VA_ARGS__)), \
        __EXTRACT_SIZES(_make_struct_layout(LAYOUT), __INPUT_SEQ(__VA_ARGS__))) \

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a capability attribute. */
	struct CapabilityAttrib
	{
		// Typedef for the attrib value type
		using AttribValue = std::variant<GLboolean, GLint, GLint64, GLfloat, GLdouble>;

		// Category of the capability
		std::string m_category;

		// Corresponding GL enum
		GLenum m_enum;

		// Type of the capability
		GLenum m_type = GL_INT;

		// Number of values associated to this capability
		GLint m_count = 1;

		// Value of the capability
		std::vector<AttribValue> m_values;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** The various capability enums to query. */
	extern std::unordered_map<std::string, std::unordered_map<GLenum, CapabilityAttrib>> s_capabilityAttribs;

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a material used to render a mesh. */
	struct Material
	{
		// Various blend modes avaiable
		meta_enum(BlendMode, int, Opaque, Masked, Translucent);

		/** Name of the material. */
		std::string m_name;

		/** Blending mode. */
		BlendMode m_blendMode = Opaque;

		/** Diffuse texture. */
		std::string m_diffuseMap = "default_diffuse_map";

		/** Normal map. */
		std::string m_normalMap = "default_normal_map";

		/** Specular map. */
		std::string m_specularMap = "default_specular_map";

		/** Alpha map. */
		std::string m_alphaMap = "default_alpha_map";

		/** Displacement map. */
		std::string m_displacementMap = "default_displacement_map";

		/** Diffuse tint color. */
		glm::vec3 m_diffuse = glm::vec3(1.0f);

		/** Emissive color. */
		glm::vec3 m_emissive = glm::vec3(1.0f);

		/** Opacity scaling. */
		float m_opacity = 1.0f;

		/** Metallicness. */
		float m_metallic = 0.0f;

		/** Roughness. */
		float m_roughness = 1.0f;

		/** Specular scale factor. */
		float m_specular = 1.0f;

		/** Displacement scale factor. */
		float m_displacementScale = 0.0f;

		/** Strength of the normal map. */
		float m_normalMapStrength = 1.0f;

		/** Whether the material is twosided or not. */
		bool m_twoSided = false;

		/** Mask terms used to interpret the specular map. */
		glm::vec4 m_specularMask = { 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec4 m_roughnessMask = { 0.0f, 1.0f, 0.0f, 0.0f };
		glm::vec4 m_metallicMask = { 0.0f, 0.0f, 1.0f, 0.0f };
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a sub-mesh, a.k.a. a smaller part of a mesh. */
	struct SubMesh
	{
		/** Name of the submesh. */
		std::string m_name;

		// AABB of the submesh
		BVH::AABB m_aabb;

		/** Start indices for the vertex and index buffers. */
		unsigned m_vertexStartID = 0;
		unsigned m_indexStartID = 0;

		/** The number of vertices and indices the submesh contains. */
		unsigned m_vertexCount = 0;
		unsigned m_indexCount = 0;

		/** The material that the submesh uses. */
		unsigned m_materialId = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a 3D static mesh. */
	struct Mesh
	{
		/** The submeshes that make up the mesh. */
		std::vector<SubMesh> m_subMeshes;

		/** The materials related to the mesh. */
		std::vector<Material> m_materials;

		/** AABB of the entire mesh */
		BVH::AABB m_aabb;

		/** The vertex array used to render the sub mesh. */
		GLuint m_vao = 0;

		/** Monolithic vbos */
		GLuint m_vboPosition = 0;
		GLuint m_vboNormal = 0;
		GLuint m_vboTangent = 0;
		GLuint m_vboBitangent = 0;
		GLuint m_vboUV = 0;

		/** Monolithic ibo */
		GLuint m_ibo = 0;

		/** Monolithic material index buffer */
		GLuint m_mbo = 0;

		/** The number of vertices and indices the mesh contains. */
		unsigned m_vertexCount = 0;
		unsigned m_indexCount = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a texture. */
	struct Texture
	{
		// Texture dimensions.
		glm::ivec3 m_dimensions{ 0, 0, 0 };
		int m_width = 0;
		int m_height = 0;
		int m_depth = 0;
		int m_numDimensions = 0;

		// Type of the texture
		GLenum m_type = 0;

		// Texture data format
		GLenum m_format = 0;

		// Texture layout
		GLenum m_layout = 0;

		// Sampler settings
		GLenum m_wrapMode = 0;
		GLenum m_minFilter = 0;
		GLenum m_magFilter = 0;
		GLfloat m_anisotropy = 0.0f;
		bool m_mipmapped = false;

		// Type of the texture
		GLenum m_bindingId = 0;

		// The corresponding texture object.
		GLuint m_texture = 0;

		// Framebuffer object to render into the texture
		GLuint m_framebuffer;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a geometry buffer for deferred shading. */
	struct GBuffer
	{
		// Texture dimensions.
		glm::ivec2 m_dimensions{ 0, 0 };
		int m_width = 0;
		int m_height = 0;
		int m_numLayers = 0;
		int m_numBuffers = 0;
		int m_samples = 0;

		// The current write buffer.
		int m_readBuffer = 0;
		int m_writeBuffer = 1;

		// The corresponding texture objects.
		GLuint m_colorTextures[2];
		GLuint m_depthTexture;
		GLuint m_normalTexture;
		GLuint m_specularTexture;

		GLuint m_colorTextureMsaa;
		GLuint m_depthTextureMsaa;
		GLuint m_normalTextureMsaa;
		GLuint m_specularTextureMsaa;

		// The G-Buffer FBOs
		GLuint m_gbuffer;                                         // Regular GBuffer: color[0], normal, specular, with a depth buffer
		GLuint m_gbufferMsaa;                                     // Regular GBuffer with MSAA textures
		GLuint m_gbufferPerLayer[Constants::s_maxLayers];         // Regular GBuffer, with access to the individual layers

		// FBO's with the two color buffers for post processing.
		GLuint m_colorBuffersPerLayer[2][Constants::s_maxLayers]; // Framebuffer for each individual layer, with color attachment #i and a depth buffer
		GLuint m_colorBuffersLayered[2];                          // Framebuffer with access to each layer, with color attachment #i and a depth buffer

		// Other FBO's
		GLuint m_dualColorBufferPerLayer[Constants::s_maxLayers]; // Framebuffer for each individual layer, with acces to each layer, with color[0], color[1] and a depth buffer attached
		GLuint m_dualColorBufferLayered;                          // Framebuffer with acces to each layer, with color[0], color[1] and a depth buffer attached
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a voxel grid for deferred voxel shading. */
	struct VoxelGrid
	{
		// Texture dimensions.
		int m_width = 0;
		int m_height = 0;
		int m_depth = 0;
		glm::ivec3 m_dimensions{ 0, 0, 0 };
		GLenum m_gbufferDataFormat = GL_RGBA8;
		GLenum m_radianceDataFormat = GL_RGBA32F;
		bool m_anisotropic = false;

		// The corresponding texture objects.
		GLuint m_albedoTexture = 0;
		GLuint m_normalTexture = 0;
		GLuint m_specularTexture = 0;
		GLuint m_radianceTexture = 0;
		GLuint m_radianceMipmaps[6] = { 0 };
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a generic GPU buffer. */
	struct GenericBuffer
	{
		// Whether the buffer is indexed (GL_UNIFORM_BUFFER, etc.) or not (GL_DISPATCH_INDIRECT_BUFFER)
		bool m_indexed = true;

		// Whether the buffer has immutable storage or not
		bool m_immutable = false;

		// Whether the buffer is persistently mapped or not
		bool m_persistentlyMapped = false;

		// Id of the binding point
		GLuint m_bindingId = 0;

		// Type of the buffer (GL_*)
		GLenum m_bufferType = GL_UNIFORM_BUFFER;

		// Buffer flags
		GLenum m_flags;

		// Size of the allocated storage
		GLsizeiptr m_size = 0;

		// Element size of the contained objects
		GLsizeiptr m_elementSize = 0;

		// Total size of the buffer - includes triple buffering, etc.
		GLsizeiptr m_totalSize = 0;

		// The buffer object.
		GLuint m_buffer = 0;

		// Buffer regions, in case of persistently mapped buffers
		void* m_persistentRegions[3];

		// The synchronization objects for triple buffering, if using persistently mapped buffers
		GLuint m_fences[3];
	};

	////////////////////////////////////////////////////////////////////////////////
	/** A shader object. */
	struct Shader
	{
		// The corresponding shader program object.
		GLuint m_program = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a GPU occlusion query. */
	struct OcclusionQuery
	{
		// The query object.
		GLuint m_query = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a GPU perf counter. */
	struct PerfCounter
	{
		// The counter object.
		GLuint m_counters[2] = { 0, 0 };
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a dispatch indirect command. */
	struct DispatchIndirectCommand 
	{
		GLuint m_numGroupsX;
		GLuint m_numGroupsY;
		GLuint m_numGroupsZ;
	};

	////////////////////////////////////////////////////////////////////////////////
	/*  Returns the dimensions of the nth mip level of a texture of target size, using 'floor' to round. */
	glm::ivec2 mipDimensionsFloor(glm::ivec2 textureSize, int lodLevel);
	glm::ivec3 mipDimensionsFloor(glm::ivec3 textureSize, int lodLevel);
	glm::ivec3 mipDimensionsFloor(Texture const& texture, int lodLevel);

	////////////////////////////////////////////////////////////////////////////////
	/*  Returns the dimensions of the nth mip level of a texture of target size, using 'ceil' to round. */
	glm::ivec2 mipDimensionsCeil(glm::ivec2 textureSize, int lodLevel);
	glm::ivec3 mipDimensionsCeil(glm::ivec3 textureSize, int lodLevel);
	glm::ivec3 mipDimensionsCeil(Texture const& texture, int lodLevel);

	////////////////////////////////////////////////////////////////////////////////
	/*  Returns the dimensions of the nth mip level of a texture of target size. */
	glm::ivec2 mipDimensions(glm::ivec2 textureSize, int lodLevel);
	glm::ivec3 mipDimensions(glm::ivec3 textureSize, int lodLevel);
	glm::ivec3 mipDimensions(Texture const& texture, int lodLevel);

	////////////////////////////////////////////////////////////////////////////////
	/*  Returns the number of mip levels corresponding to the parameter texture size. */
	int numMipLevels(int textureSize);
	int numMipLevels(glm::ivec2 textureSize);
	int numMipLevels(glm::ivec3 textureSize);
	int numMipLevels(Texture const& texture);

	////////////////////////////////////////////////////////////////////////////////
	size_t numTexels(glm::ivec3 dimensions, const int baseLevel, const int numLevels, const int maxLevel);
	size_t numTexels(Texture const& texture, const int baseLevel = 0, const int numLevels = 1);

	////////////////////////////////////////////////////////////////////////////////
	// Enum to string conversion
	std::string enumToString(GLenum s);

	////////////////////////////////////////////////////////////////////////////////
	// Data type to string conversion
	std::string dataTypeName(GLenum s);
	std::string dataTypeName(GLenum s, size_t arraySize);

	////////////////////////////////////////////////////////////////////////////////
	// Data type size query
	size_t dataTypeSize(GLenum s);
	size_t dataTypeSize(GLenum s, size_t arraySize, size_t arrayStride);

	////////////////////////////////////////////////////////////////////////////////
	// Texture format queries
	size_t textureFormatChannels(GLenum format);
	size_t textureFormatTexelSize(GLenum format);

	////////////////////////////////////////////////////////////////////////////////
	size_t textureSizeBytes(glm::ivec3 dimensions, GLenum format, const int baseLevel = 0, const int numLevels = 1);
	size_t textureSizeBytes(Texture const& texture, const int baseLevel = 0, const int numLevels = 1);

	////////////////////////////////////////////////////////////////////////////////
	struct ShaderInspectionUniform
	{
		std::string m_name;
		GLenum m_type;
		GLint m_location = 0;
		bool m_isArray = false;
		GLuint m_arraySize = 0;
		GLuint m_arrayStride = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionUniform> inspectProgramUniforms(GLuint program);

	////////////////////////////////////////////////////////////////////////////////
	struct ShaderInspectionUniformBlock
	{
		struct Variable
		{
			GLuint m_byteOffset = 0;
			std::string m_name;
			GLenum m_type;
			bool m_isArray = false;
			GLuint m_arraySize = 0;
			GLuint m_arrayStride = 0;
			GLuint m_padding = 0;
		};

		std::string m_name;
		GLuint m_binding;
		GLuint m_dataSize;
		std::vector<Variable> m_variables;
	};

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionUniformBlock> inspectProgramUniformBlocks(GLuint program);

	////////////////////////////////////////////////////////////////////////////////
	struct ShaderInspectionShaderStorageBlock
	{
		struct Variable
		{
			GLuint m_byteOffset = 0;
			std::string m_name;
			GLenum m_type;
			bool m_isArray = false;
			GLuint m_arraySize = 0;
			GLuint m_arrayStride = 0;
			GLuint m_padding = 0;
			GLuint m_topLevelArraySize = 0;
			GLuint m_topLevelArrayStride = 0;
		};

		std::string m_name;
		GLuint m_binding;
		GLuint m_dataSize;
		std::vector<Variable> m_variables;
	};

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionShaderStorageBlock> inspectProgramShaderStorageBlocks(GLuint program);

	////////////////////////////////////////////////////////////////////////////////
	struct ShaderInspectionProgramInput
	{
		std::string m_name;
		GLenum m_type;
		GLint m_location = 0;
		bool m_isArray = false;
		GLuint m_arraySize = 0;
		bool m_referectedByTCS = false;
		bool m_referectedByTES = false;
		bool m_referectedByVS = false;
		bool m_referectedByGS = false;
		bool m_referectedByFS = false;
		bool m_referectedByCS = false;
	};

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionProgramInput> inspectProgramInputs(GLuint program);

	////////////////////////////////////////////////////////////////////////////////
	struct ShaderInspectionProgramOutput
	{
		std::string m_name;
		GLenum m_type;
		GLint m_location = 0;
		bool m_isArray = false;
		GLuint m_arraySize = 0;
		bool m_referectedByTCS = false;
		bool m_referectedByTES = false;
		bool m_referectedByVS = false;
		bool m_referectedByGS = false;
		bool m_referectedByFS = false;
		bool m_referectedByCS = false;
	};

	////////////////////////////////////////////////////////////////////////////////
	std::vector<ShaderInspectionProgramOutput> inspectProgramOutputs(GLuint program);

	////////////////////////////////////////////////////////////////////////////////
	/** Queries a capability attribute. */
	void init();
}