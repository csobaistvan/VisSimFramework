#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "Profiler.h"
#include "Asset.h"
#include "Object.h"

////////////////////////////////////////////////////////////////////////////////
/// SCENE STRUCTURES
////////////////////////////////////////////////////////////////////////////////
namespace Scene
{
	////////////////////////////////////////////////////////////////////////////////
	/** Various resource types. */
	meta_enum(ResourceType, int,
		Shader,
		Texture,
		Mesh,
		GBuffer,
		GenericBuffer,
		Font,
		OcclusionQuery,
		PerfCounter,
		NeuralNetwork,
		Custom
	);

	////////////////////////////////////////////////////////////////////////////////
	/** A resource init callback. */
	using ResourceInitCallback = std::function<void(Scene&, Object*)>;

	////////////////////////////////////////////////////////////////////////////////
	/** A resource release callback. */
	using ResourceReleaseCallback = std::function<void(Scene&, Object*)>;

	////////////////////////////////////////////////////////////////////////////////
	//  SCENE MANAGEMENT
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	/** Represents a renderable scene. */
	struct Scene
	{
		// Default ctor.
		Scene();

		// Move ctor
		Scene(Scene&& other) = default;
		Scene& operator=(Scene&& other) = default;

		// Disable the copy ctor
		Scene(Scene& other) = delete;
		Scene& operator=(Scene& other) = delete;

		// Name of the scene
		std::string m_name;

		// Whether the scene is a demo scene with demo object setup
		bool m_isDemoScene = false;

		// The context that this object belongs to.
		Context::Context m_context;

		// The objects in the scene.
		std::unordered_map<std::string, Object> m_objects;

		// The firstobjects in the scene.
		std::unordered_map<int, Object*> m_firstObjects;

		////////////////////////////////////////////////////////////////////////////////

		// Contents of all the text files ever accessed.
		std::unordered_map<std::string, Asset::TextFile> m_textFiles;

		// List of available skybox names.
		std::vector<std::string> m_skyboxNames;

		// List of available color LUT names.
		std::vector<std::string> m_lutNames;

		// All the textures in use.
		std::unordered_map<std::string, GPU::Texture> m_textures;

		// All the meshes in use.
		std::unordered_map<std::string, GPU::Mesh> m_meshes;

		// All the materials in use.
		std::unordered_map<std::string, GPU::Material> m_materials;

		// The shaders in use.
		std::unordered_map<std::string, GPU::Shader> m_shaders;

		// The geometry buffers.
		GPU::GBuffer m_gbuffer[2];

		// The deferred voxel grid
		GPU::VoxelGrid m_voxelGrid;

		// The various GPU buffers.
		std::unordered_map<std::string, GPU::GenericBuffer> m_genericBuffers;

		// The various occlusion queries.
		std::unordered_map<std::string, GPU::OcclusionQuery> m_occlusionQueries;

		// The various perf counters.
		std::unordered_map<std::string, GPU::PerfCounter> m_perfCounters;

		// Imgui fonts.
		std::unordered_map<std::string, ImFont*> m_fonts;

		// Imgui styles.
		std::unordered_map<std::string, ImGuiStyle> m_guiStyles;

		// ImPlot color maps.
		std::unordered_map<std::string, ImPlotColormap> m_plotColorMaps;

		// TensorFlow models
		std::unordered_map<std::string, TensorFlow::Model> m_tfModels;

		////////////////////////////////////////////////////////////////////////////////

		struct ResourceInitializer
		{
			ResourceInitCallback m_callback;
			bool m_loaded = false;
		};

		// Resource initializers
		//  [0] Resource type
		//  [1] Object name
		//  [2] Resource sub-category
		std::unordered_map<ResourceType, std::unordered_map<std::string, std::unordered_map<std::string, ResourceInitializer>>> m_resourceInitializers;

		struct ResourceReleaser
		{
			ResourceReleaseCallback m_callback;
		};

		// Resource releasers
		//  [0] Resource type
		//  [1] Object name
		//  [2] Resource sub-category
		std::unordered_map<ResourceType, std::unordered_map<std::string, std::unordered_map<std::string, ResourceReleaser>>> m_resourceReleasers;

		////////////////////////////////////////////////////////////////////////////////

		// The various profile data to display.
		Profiler::ProfilerData m_profilerData;
		std::array<Profiler::ProfilerTree, 3> m_profilerTree;

		// Id of the debug buffer to write to
		int m_profilerBufferReadId = 0;
		int m_profilerBufferWriteId = 2;

		// Iterator where the debug data should be written to
		Profiler::ProfilerTreeIterator m_profilerWritePosition;
	};

	////////////////////////////////////////////////////////////////////////////////
	//  OBJECT MANAGEMENT
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	void invokeDefaultObjectInitializer(Scene& scene, Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void invokeDefaultObjectReleaser(Scene& scene, Object& object);

	////////////////////////////////////////////////////////////////////////////////
	template<typename Fn>
	auto extendDefaultObjectInitializerBefore(Fn fn)
	{
		return [=](Scene& scene, Object& object)
		{
			fn(scene, object);
			invokeDefaultObjectInitializer(scene, object);
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Fn>
	auto extendDefaultObjectInitializerAfter(Fn fn)
	{
		return [=](Scene& scene, Object& object)
		{
			invokeDefaultObjectInitializer(scene, object);
			fn(scene, object);
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename FnBefore, typename FnAfter>
	auto extendDefaultObjectInitializer(FnBefore fnBefore, FnAfter fnAfter)
	{
		return [=](Scene& scene, Object& object)
		{
			fnBefore(scene, object);
			invokeDefaultObjectInitializer(scene, object);
			fnAfter(scene, object);
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	Object& createObject(Scene& scene, const std::string& baseName, unsigned long long components, ObjectInitializer initializer = &invokeDefaultObjectInitializer);

	////////////////////////////////////////////////////////////////////////////////
	Object& createObject(Scene& scene, unsigned long long components, ObjectInitializer initializer = &invokeDefaultObjectInitializer);

	////////////////////////////////////////////////////////////////////////////////
	bool removeObject(Scene& scene, Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void filterObjects(Scene& scene, std::vector<Object*>& results, unsigned long long mask, bool exactMatch = true, bool includeDisabled = false, bool thisGroupOnly = false);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<Object*> filterObjects(Scene& scene, unsigned long long mask, bool exactMatch = true, bool includeDisabled = false, bool thisGroupOnly = false);

	////////////////////////////////////////////////////////////////////////////////
	std::vector<Object*> filterObjects(Scene& scene, std::initializer_list<ComponentId> components, bool exactMatch = true, bool includeDisabled = false, bool thisGroupOnly = false);

	////////////////////////////////////////////////////////////////////////////////
	template<typename Fn>
	void filterObjects(Scene& scene, std::vector<Object*>& results, unsigned long long mask, Fn const& pred, bool exactMatch = true, bool includeDisabled = false, bool thisGroupOnly = false)
	{
		// Group settings helper object
		auto groupsSettings = scene.m_firstObjects.find(OBJECT_TYPE_SIMULATION_SETTINGS) != scene.m_firstObjects.end() ? scene.m_firstObjects[OBJECT_TYPE_SIMULATION_SETTINGS] : nullptr;

		// Traverse the list of objects
		for (auto& objectIt : scene.m_objects)
		{
			// Extract the object itself
			Object* object = &objectIt.second;

			// Filter condition for the current object
			bool thisFilter = true;

			// Check if it is enabled or not
			thisFilter &= includeDisabled || SimulationSettings::isObjectEnabled(scene, groupsSettings, object);

			// Check if it matches the component mask or not
			thisFilter &= exactMatch ? (object->m_componentList == mask) : (object->m_componentList & mask) == mask;

			// Append to the result if the object matched all filters
			if (thisFilter && pred(object)) results.push_back(object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Fn>
	std::vector<Object*> filterObjects(Scene& scene, unsigned long long mask, Fn const& pred, bool exactMatch = true, bool includeDisabled = false, bool thisGroupOnly = false)
	{
		std::vector<Object*> result;
		filterObjects(scene, result, mask, pred, exactMatch, includeDisabled, thisGroupOnly);
		return result;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	template<typename Fn>
	std::vector<Object*> filterObjects(Scene& scene, std::initializer_list<ComponentId> components, Fn const& pred, bool exactMatch = true, bool includeDisabled = false, bool thisGroupOnly = false)
	{
		return filterObjects(scene, std::bit_mask(components), pred, exactMatch, includeDisabled, thisGroupOnly);
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObject(Scene& scene, std::string const& group, std::initializer_list<ComponentId> components);

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObject(Scene& scene, std::string const& group, unsigned long long mask);

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObject(Scene& scene, std::initializer_list<ComponentId> components);

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObject(Scene& scene, unsigned long long mask);

	////////////////////////////////////////////////////////////////////////////////
	Object* findOriginalObject(Scene& scene, std::string const& name);

	////////////////////////////////////////////////////////////////////////////////
	Object* findObject(Scene& scene, std::string const& name);

	////////////////////////////////////////////////////////////////////////////////
	Object* renameObject(Scene& scene, Object* object, std::string const& newName);

	////////////////////////////////////////////////////////////////////////////////
    //  SCENE LIFECYCLE
	////////////////////////////////////////////////////////////////////////////////
	
	////////////////////////////////////////////////////////////////////////////////
	void setupScene(Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void teardownScene(Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void updateScene(Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void renderScene(Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void mainLoop(Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	//  RESOURCE MANAGEMENT
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	void appendResourceInitializer(Scene& scene, ResourceType resourceType, ResourceInitCallback callback, std::string const& label = "");

	////////////////////////////////////////////////////////////////////////////////
	void appendResourceReleaser(Scene& scene, ResourceType resourceType, ResourceReleaseCallback callback, std::string const& label = "");

	////////////////////////////////////////////////////////////////////////////////
	void removeResourceInitializer(Scene& scene, ResourceType resourceType, std::string const& label = "");

	////////////////////////////////////////////////////////////////////////////////
	void removeResourceReleaser(Scene& scene, ResourceType resourceType, std::string const& label = "");

	////////////////////////////////////////////////////////////////////////////////
	void appendResourceInitializer(Scene& scene, std::string const& objectName, ResourceType resourceType, ResourceInitCallback callback, std::string const& label = "");

	////////////////////////////////////////////////////////////////////////////////
	void appendResourceReleaser(Scene& scene, std::string const& objectName, ResourceType resourceType, ResourceReleaseCallback callback, std::string const& label = "");

	////////////////////////////////////////////////////////////////////////////////
	void removeResourceInitializer(Scene& scene, std::string const& objectName, ResourceType resourceType, std::string const& label = "");

	////////////////////////////////////////////////////////////////////////////////
	void removeResourceReleaser(Scene& scene, std::string const& objectName, ResourceType resourceType, std::string const& label = "");

	////////////////////////////////////////////////////////////////////////////////
	void loadResources(Scene& scene, ResourceType resourceType, std::string const& objectName);

	////////////////////////////////////////////////////////////////////////////////
	void loadResources(Scene& scene, ResourceType resourceType);

	////////////////////////////////////////////////////////////////////////////////
	void loadResources(Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void unloadResources(Scene& scene, ResourceType resourceType, std::string const& objectName);

	////////////////////////////////////////////////////////////////////////////////
	void unloadResources(Scene& scene, ResourceType resourceType);

	////////////////////////////////////////////////////////////////////////////////
	void unloadResources(Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	void reloadResources(Scene& scene, ResourceType resourceType);

	////////////////////////////////////////////////////////////////////////////////
    //  GPU OBJECT CONSTRUCTORS
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	bool createTexture(Scene& scene, const std::string& textureName, GLenum textureType, int width, int height, int depth, 
		GLenum format, GLenum layout, GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR, 
		GLenum wrapMode = GL_CLAMP_TO_EDGE, GLenum bindingId = GL_TEXTURE0);

	////////////////////////////////////////////////////////////////////////////////
	bool createTexture(Scene& scene, const std::string& textureName, GLenum textureType, int width, int height, 
		GLenum format, GLenum layout, GLenum minFilter, GLenum magFilter, GLenum wrapMode, GLenum depthFormat, 
		GLenum bindingId = GL_TEXTURE0);

	////////////////////////////////////////////////////////////////////////////////
	bool createTexture(Scene& scene, const std::string& textureName, GLenum textureType, int width, int height, int depth, 
		GLenum format, GLenum layout, GLenum minFilter, GLenum magFilter, GLenum wrapMode, GLenum dataFormat, const void* data, 
		GLenum bindingId = GL_TEXTURE0);

	////////////////////////////////////////////////////////////////////////////////
	bool renderTexture(Scene& scene, const std::string& textureName, GLenum textureType, int width, int height, int depth, 
		GLenum format, GLenum layout, GLenum minFilter, GLenum magFilter, GLenum wrapMode, std::string const& shaderName, 
		GLenum bindingId = GL_TEXTURE0);

	////////////////////////////////////////////////////////////////////////////////
	bool deleteTexture(Scene& scene, const std::string& textureName);

	////////////////////////////////////////////////////////////////////////////////
	bool createGBuffer(Scene& scene, int numLayers, unsigned bufferWidth, unsigned bufferHeight, 
		GLenum depthFormat, GLenum dataFormat, int samples);

	////////////////////////////////////////////////////////////////////////////////
	bool createVoxelGrid(Scene& scene, unsigned bufferWidth, unsigned bufferHeight, unsigned bufferDepth, 
		GLenum gbufferDataFormat, GLenum radianceFormat, bool anisotropic);
	
	////////////////////////////////////////////////////////////////////////////////
	bool createGPUBuffer(Scene& scene, const std::string& name, GLenum bufferType, bool immutable = false, bool indexed = false, 
		GLuint bindingId = 0, GLenum flags = 0, GLsizeiptr storageSize = 0, GLsizeiptr elementSize = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool resizeGPUBuffer(Scene& scene, const std::string& bufferName, size_t newSize, bool growOnly = false);

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	bool createGPUBuffer(Scene& scene, const std::string& bufferName, GLenum bufferType, bool immutable, bool indexed, 
		GLuint bindingId, GLenum flags, std::vector<T> const& data)
	{
		const size_t elementSize = sizeof(data[0]);
		const size_t dataSize = data.size() * elementSize;
		bool result = createGPUBuffer(scene, bufferName, bufferType, immutable, indexed, bindingId, flags, dataSize, elementSize);
		bindBuffer(scene, bufferName);
		uploadBufferData(scene, bufferName, dataSize, data.data());
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool deleteGPUBuffer(Scene& scene, const std::string& bufferName);

	////////////////////////////////////////////////////////////////////////////////
	bool createPerfCounter(Scene& scene, const std::string& counterName);

	////////////////////////////////////////////////////////////////////////////////
	bool createGpuOcclusionQuery(Scene& scene, const std::string& queryName);

	////////////////////////////////////////////////////////////////////////////////
	//  GPU OBJECT BINDING
	////////////////////////////////////////////////////////////////////////////////
	
	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding shaders. */
	void bindShader(Scene& scene, std::string const& shaderName);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding shaders. */
	void bindShader(Scene& scene, std::string const& categoryName, std::string const& shaderName);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding textures. */
	void bindTexture(GPU::Texture const& texture, GPU::TextureIndices index);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding textures. */
	void bindTexture(Scene& scene, std::string const& textureName, GPU::TextureIndices index);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding textures. */
	void bindTexture(Scene& scene, std::string const& textureName);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding framebuffers. */
	void bindFramebuffer(Scene& scene, std::string const& framebufferName);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void bindBuffer(GPU::GenericBuffer const& buffer, GPU::UniformBufferIndices index);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void bindBuffer(Scene& scene, std::string const& uboName);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void bindBuffer(Scene& scene, std::string const& uboName, GPU::UniformBufferIndices index);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for unbinding buffers. */
	void unbindBuffer(GPU::GenericBuffer const& buffer, GPU::UniformBufferIndices index);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void unbindBuffer(Scene& scene, std::string const& uboName);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void unbindBuffer(Scene& scene, std::string const& uboName, GPU::UniformBufferIndices index);

	////////////////////////////////////////////////////////////////////////////////
	//  GPU DATA UPLOADING
	////////////////////////////////////////////////////////////////////////////////
	
	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for updating uniforms. */
	void uploadBufferData(GPU::GenericBuffer& buffer, std::string const& bufferName, size_t size, const void* data);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for updating uniforms. */
	void uploadBufferData(Scene& scene, std::string const& uboName, size_t size, const void* data);

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for updating uniforms. */
	template<typename T, typename std::enable_if<std::is_container<T>::value, bool>::type = true>
	void uploadBufferData(GPU::GenericBuffer& buffer, std::string const& bufferName, T const& data)
	{
		uploadBufferData(buffer, bufferName, data.size() * sizeof(data[0]), data.data());
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for updating uniforms. */
	template<typename T, typename std::enable_if<!std::is_container<T>::value && !std::is_pointer<T>::value, bool>::type = true>
	void uploadBufferData(GPU::GenericBuffer& buffer, std::string const& bufferName, T const& data)
	{
		uploadBufferData(buffer, bufferName, sizeof(T), &data);
	}
	
	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for updating uniforms. */
	template<typename T>
	void uploadBufferData(Scene& scene, std::string const& uboName, T const& data)
	{
		GPU::GenericBuffer& ubo = scene.m_genericBuffers[uboName];

		bindBuffer(ubo, GPU::UniformBufferIndices(ubo.m_bindingId));
		uploadBufferData(ubo, uboName, data);
	}

	////////////////////////////////////////////////////////////////////////////////
	//  GPU SYNCHRONIZATION
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	bool waitForGpu(Scene& scene, const size_t maxWaitPeriod = 0);
}