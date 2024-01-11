#include "PCH.h"
#include "Scene.h"
#include "Components/Settings/SimulationSettings.h"
#include "Components/Rendering/Camera.h"

namespace Scene
{
	////////////////////////////////////////////////////////////////////////////////
	//  OBJECT MANAGEMENT
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	void invokeDefaultObjectInitializer(Scene& scene, Object& object)
	{
		// Try to load the corresponding initializer
		if (auto it = objectInitializers().find((ObjectType)object.m_componentList); it != objectInitializers().end())
		{
			// Extract the callback
			ObjectInitializer initializer = it->second;

			// If there is an initializer, invoke it
			if (initializer) initializer(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void invokeDefaultObjectReleaser(Scene& scene, Object& object)
	{
		// Try to load the corresponding initializer
		if (auto it = objectReleasers().find((ObjectType)object.m_componentList); it != objectReleasers().end())
		{
			// Extract the callback
			ObjectInitializer releaser = it->second;

			// If there is an initializer, invoke it
			if (releaser) releaser(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename V>
	std::string generateUniqueName(Scene& scene, std::unordered_map<std::string, V> const& values, std::string const& baseName, int startNumber, bool canKeepOriginal)
	{
		// Try the base name, if we are allowed
		if (canKeepOriginal && values.find(baseName) == values.end())
			return baseName;

		// Resulting variable
		std::string result;

		// Generate a unique name
		do
		{
			result = baseName + " " + std::to_string(startNumber++);
		} while (values.find(result) != values.end());

		// Return the result
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	Object& createObject(Scene& scene, const std::string& baseName, unsigned long long components, ObjectInitializer initializer)
	{
		// Generate a unique name
		std::string name = generateUniqueName(scene, scene.m_objects, baseName, 1, true);

		// Enter a debug region
		Debug::DebugRegion debugRegion(name);

		// Initialize the object for the scene.
		Object& object = scene.m_objects[name] = Object{};

		// Store a pointer to the owner
		object.m_owner = &scene;

		// Store its name.
		object.m_name = name;

		// Store its components.
		object.m_componentList = components;

		// Instantiate the object's components
		Debug::log_debug() << "Adding components to object of type " << objectNames()[components] << " (with mask " << object.m_componentList << ")" << Debug::end;

		for (auto componentType : componentTypes())
		{
			if ((object.m_componentList & std::bit_mask(componentType)) != 0)
			{
				Debug::log_debug() 
					<< " -  " 
					<< componentNames()[componentType] 
					<< " (ID #" << componentType 
					<< ", mask " << std::bit_mask(componentType) << ") " 
					<< Debug::end;

				componentConstructors()[componentType](object);
			}
		}

		// Call the supplied initializer
		initializer(scene, object);

		// Store it as the first object when first created
		if (object.m_enabled)
		{
			for (auto objectType : objectTypes())
			{
				if (components == objectType && scene.m_firstObjects.find(objectType) == scene.m_firstObjects.end())
				{
					scene.m_firstObjects[objectType] = &object;
				}
			}
		}

		// Return the now initialized object
		return object;
	}

	////////////////////////////////////////////////////////////////////////////////
	Object& createObject(Scene& scene, unsigned long long components, ObjectInitializer initializer)
	{
		return createObject(scene, objectNames()[(ObjectType) components], components, initializer);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool removeObject(Scene& scene, Object& object)
	{
		// Name of the object to remove
		std::string objectName = object.m_name;

		// Make sure the object is valid
		auto it = scene.m_objects.find(object.m_name);

		// Make sure it exists
		if (it == scene.m_objects.end())
			return false;

		// Invoke the object releaser
		invokeDefaultObjectReleaser(scene, object);

		// Remove the release initializers and releasers
		for (auto category : ResourceType_meta.members)
		{
			// Unload the object's resources
			unloadResources(scene, category.value, objectName);

			if (auto it = scene.m_resourceInitializers[category.value].find(object.m_name); it != scene.m_resourceInitializers[category.value].end())
			{
				scene.m_resourceInitializers[category.value].erase(it);
			}

			if (auto it = scene.m_resourceReleasers[category.value].find(object.m_name); it != scene.m_resourceReleasers[category.value].end())
			{
				scene.m_resourceReleasers[category.value].erase(it);
			}
		}
				
		// Remove the object
		scene.m_objects.erase(it);

		// Remove alias objects
		while (true)
		{
			// Loop through every object
			bool nothingFound = true;
			for (auto& objectIt : scene.m_objects)
			{
				if (objectIt.second.m_alias == objectName)
				{
					removeObject(scene, objectIt.second);
					nothingFound = false;
					break;
				}
			}

			// Stop if nothing was found
			if (nothingFound) break;
		}
					   			
		// Success
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	void filterObjects(Scene& scene, std::vector<Object*>& result, unsigned long long mask, bool exactMatch, bool includeDisabled, bool thisGroupOnly)
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

			// Check if the object is in the currently active groups
			thisFilter &= !thisGroupOnly || SimulationSettings::isObjectEnabledByGroups(scene, object);

			// Check if it matches the component mask or not
			thisFilter &= exactMatch ? (object->m_componentList == mask) : (object->m_componentList & mask) == mask;

			// Append to the result if the object matched all filters
			if (thisFilter) result.push_back(object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<Object*> filterObjects(Scene& scene, unsigned long long mask, bool exactMatch, bool includeDisabled, bool thisGroupOnly)
	{
		std::vector<Object*> result;
		filterObjects(scene, result, mask, exactMatch, includeDisabled, thisGroupOnly);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<Object*> filterObjects(Scene& scene, std::initializer_list<ComponentId> components, bool exactMatch, bool includeDisabled, bool thisGroupOnly)
	{
		return filterObjects(scene, std::bit_mask(components), exactMatch, includeDisabled, thisGroupOnly);
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObjectSlow(Scene& scene, std::string const& group, unsigned long long objectType)
	{
		auto const& objects = filterObjects(scene, objectType, [&](Object* object)
			{
				return SimulationSettings::isObjectInGroup(scene, object, group);
			},
			true, false, false);
		return objects.empty() ? nullptr : objects[0];
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObjectSlow(Scene& scene, unsigned long long objectType)
	{
		auto const& objects = filterObjects(scene, objectType, true, false, true);
		return objects.empty() ? nullptr : objects[0];
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObjectFast(Scene& scene, unsigned long long mask)
	{
		return scene.m_firstObjects[mask];
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObjectImpl(Scene& scene, unsigned long long mask)
	{
		if (auto it = scene.m_firstObjects.find(mask); it != scene.m_firstObjects.end())
			return it->second;
		return findFirstObjectSlow(scene, mask);
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObject(Scene& scene, std::initializer_list<ComponentId> components)
	{
		return findFirstObjectImpl(scene, std::bit_mask(components));
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObject(Scene& scene, unsigned long long mask)
	{
		return findFirstObjectImpl(scene, mask);
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObject(Scene& scene, std::string const& group, std::initializer_list<ComponentId> components)
	{
		return findFirstObjectSlow(scene, group, std::bit_mask(components));
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findFirstObject(Scene& scene, std::string const& group, unsigned long long mask)
	{
		return findFirstObjectSlow(scene, group, mask);
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findOriginalObject(Scene& scene, std::string const& name)
	{
		// Start from the parameter object
		Object* result = &scene.m_objects[name];

		// Loop until we find a parent object
		bool keepLooping = true;
		while (keepLooping)
		{
			// Assume that this is the last element
			keepLooping = false;

			// Go through each object
			for (auto& object : scene.m_objects)
			{
				// Check if the object marks our current result as its alias
				if (object.second.m_alias == result->m_name)
				{
					// Keep looping
					keepLooping = true;
					result = &(object.second);
					break;
				}
			}
		}
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* findObject(Scene& scene, std::string const& name)
	{
		// Check if the object actually exists
		if (auto it = scene.m_objects.find(name); it == scene.m_objects.end())
			return nullptr;

		// Extract the object pointer and keep resolving the aliases in a loop
		Object* result = &scene.m_objects[name];
		while (result->m_alias.empty() == false)
			result = &scene.m_objects[result->m_alias];

		// Return the resulting object
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	Object* renameObject(Scene& scene, Object* object, std::string const& newName)
	{
		// Preserve the old name of the object
		std::string oldName = object->m_name;

		// Copy over the object
		scene.m_objects[newName] = std::move(scene.m_objects[oldName]);
		scene.m_objects[newName].m_name = newName;

		// Reset the old object and store a reference to the new name
		scene.m_objects[oldName] = Object();
		scene.m_objects[oldName].m_alias = newName;

		// Return the new object
		return &scene.m_objects[newName];
	}

	////////////////////////////////////////////////////////////////////////////////
	//  RESOURCE MANAGEMENT
	////////////////////////////////////////////////////////////////////////////////
	
	////////////////////////////////////////////////////////////////////////////////
	void appendResourceInitializer(Scene& scene, std::string const& objectName, ResourceType resourceType, ResourceInitCallback callback, std::string const& label)
	{
		if (scene.m_resourceInitializers[resourceType][objectName].find(label) != scene.m_resourceInitializers[resourceType][objectName].end()) 
			return;

		Scene::ResourceInitializer initializer;
		initializer.m_callback = callback;
		initializer.m_loaded = false;
		scene.m_resourceInitializers[resourceType][objectName][label] = initializer;
	}

	////////////////////////////////////////////////////////////////////////////////
	void appendResourceReleaser(Scene& scene, std::string const& objectName, ResourceType resourceType, ResourceReleaseCallback callback, std::string const& label)
	{
		if (scene.m_resourceReleasers[resourceType][objectName].find(label) != scene.m_resourceReleasers[resourceType][objectName].end())
			return;

		Scene::ResourceReleaser releaser;
		releaser.m_callback = callback;
		scene.m_resourceReleasers[resourceType][objectName][label] = releaser;
	}

	////////////////////////////////////////////////////////////////////////////////
	void removeResourceInitializer(Scene& scene, std::string const& objectName, ResourceType resourceType, std::string const& label)
	{
		if (auto it = scene.m_resourceInitializers[resourceType].find(findOriginalObject(scene, objectName)->m_name + label); it != scene.m_resourceInitializers[resourceType].end())
		{
			scene.m_resourceInitializers[resourceType].erase(it);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void removeResourceReleaser(Scene& scene, std::string const& objectName, ResourceType resourceType, std::string const& label)
	{
		if (auto it = scene.m_resourceReleasers[resourceType].find(findOriginalObject(scene, objectName)->m_name + label); it != scene.m_resourceReleasers[resourceType].end())
		{
			scene.m_resourceReleasers[resourceType].erase(it);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void appendResourceInitializer(Scene& scene, ResourceType resourceType, ResourceInitCallback callback, std::string const& label)
	{
		appendResourceInitializer(scene, "", resourceType, callback, label);
	}

	////////////////////////////////////////////////////////////////////////////////
	void appendResourceReleaser(Scene& scene, ResourceType resourceType, ResourceReleaseCallback callback, std::string const& label)
	{
		appendResourceReleaser(scene, "", resourceType, callback, label);
	}

	////////////////////////////////////////////////////////////////////////////////
	void removeResourceInitializer(Scene& scene, ResourceType resourceType, std::string const& label)
	{
		removeResourceInitializer(scene, "", resourceType, label);
	}

	////////////////////////////////////////////////////////////////////////////////
	void removeResourceReleaser(Scene& scene, ResourceType resourceType, std::string const& label)
	{
		removeResourceReleaser(scene, "", resourceType, label);
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadResources(Scene& scene, ResourceType resourceType, std::string const& objectName)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, objectName);

		// Look up the simulation settings object
		Object* simulationSettings = scene.m_firstObjects.find(OBJECT_TYPE_SIMULATION_SETTINGS) != scene.m_firstObjects.end() ? scene.m_firstObjects[OBJECT_TYPE_SIMULATION_SETTINGS] : nullptr;

		if (simulationSettings == nullptr) return;

		// Is this generator for the scene or not
		const bool isScene = objectName.empty() || objectName == scene.m_name;

		// Find the corresponding object
		Object* object = isScene ? nullptr : findObject(scene, objectName);

		// The object is gone; continue
		if (isScene == false && object == nullptr) return;

		Debug::DebugRegion region({ "Resource Loading", std::to_string(ResourceType_value_to_string(resourceType)), objectName });

		// Go through the labelled initializers
		for (auto& initializer : scene.m_resourceInitializers[resourceType][objectName])
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, initializer.first);

			// Skip initializers for objects that are disabled
			if (object != nullptr && !SimulationSettings::isObjectEnabled(scene, simulationSettings, object))
				continue;

			// Skip already loaded resources
			if (initializer.second.m_loaded)
				continue;

			Debug::log_trace() << "Loading resource: " << initializer.first << Debug::end;

			// Invoke the initializer
			initializer.second.m_callback(scene, object);

			// Mark the resource as loaded
			initializer.second.m_loaded = true;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadResources(Scene& scene, std::string const& objectName)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Loading Resources for " + objectName);

		Debug::log_trace() << "Loading resources for object: " << objectName << Debug::end;

		// Go through the list of initializers per object name
		for (auto const& resourceType : ResourceType_meta.members)
		for (auto const& initializers : scene.m_resourceInitializers[resourceType.value])
		{
			if (initializers.first == objectName)
				loadResources(scene, resourceType.value, initializers.first);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadResources(Scene& scene, ResourceType resourceType)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Loading " + std::string(ResourceType_meta_from_index(resourceType)->name) + "s");

		Debug::log_trace() << "Loading resource type: " << std::string(ResourceType_meta_from_index(resourceType)->name) << Debug::end;

		// Go through the list of initializers per object name
		for (auto const& initializers : scene.m_resourceInitializers[resourceType])
		{
			loadResources(scene, resourceType, initializers.first);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadResources(Scene& scene)
	{
		Debug::log_trace() << "Loading resources" << Debug::end;

		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Resource Loading");

		// Go through each resource type
		for (auto const& resourceType : ResourceType_meta.members)
		{
			loadResources(scene, resourceType.value);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void unloadResources(Scene& scene, ResourceType resourceType, std::string const& objectName)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, objectName);

		// Look up the simulation settings object
		Object* simulationSettings = scene.m_firstObjects.find(OBJECT_TYPE_SIMULATION_SETTINGS) != scene.m_firstObjects.end() ? scene.m_firstObjects[OBJECT_TYPE_SIMULATION_SETTINGS] : nullptr;

		// Is this generator for the scene or not
		bool isScene = objectName.empty() || objectName == scene.m_name;

		// Find the corresponding object
		Object* object = isScene ? nullptr : findObject(scene, objectName);

		// The object is gone; continue
		if (isScene == false && object == nullptr) return;

		Debug::DebugRegion region({ "Resource Unloading", std::to_string(ResourceType_value_to_string(resourceType)), objectName });

		// Go through the labelled initializers
		for (auto& releaser : scene.m_resourceReleasers[resourceType][objectName])
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, releaser.first);

			// Skip initializers for objects that are disabled
			if (object != nullptr && !SimulationSettings::isObjectEnabled(scene, simulationSettings, object))
				continue;

			Debug::log_trace() << "Unoading resource: " << releaser.first << Debug::end;

			// Invoke the releaser
			releaser.second.m_callback(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void unloadResources(Scene& scene, ResourceType resourceType)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Unloading " + std::string(ResourceType_meta_from_index(resourceType)->name) + "s");

		Debug::log_trace() << "Unloading resource type: " << std::string(ResourceType_meta_from_index(resourceType)->name) << Debug::end;

		// Go through the list of initializers per object name
		for (auto const& releasers : scene.m_resourceReleasers[resourceType])
		{
			unloadResources(scene, resourceType, releasers.first);
		}

		// Go through the labelled initializers and mark them unloaded
		for (auto& initializers : scene.m_resourceInitializers[resourceType])
		for (auto& initializer : initializers.second)
		{
			initializer.second.m_loaded = false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void unloadResources(Scene& scene)
	{
		Debug::log_trace() << "Unloading resources" << Debug::end;

		Profiler::ScopedCpuPerfCounter perfCounter(scene, "Resource Unloading");

		// Go through each resource type
		for (auto const& resourceType : ResourceType_meta.members)
		{
			unloadResources(scene, resourceType.value);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void reloadResources(Scene& scene, ResourceType resourceType)
	{
		Debug::log_trace() << "Reloading resources (" << ResourceType_value_to_string(resourceType) << ")" << Debug::end;

		// Invoke the releasers
		unloadResources(scene, resourceType);

		// Invoke the initializers
		loadResources(scene, resourceType);

		Debug::log_trace() << "Successfully reloaded resources (" << ResourceType_value_to_string(resourceType) << ")" << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	//  GPU OBJECT CONSTRUCTORS
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	bool generateTextureImpl(Scene& scene, const std::string& textureName, GLenum textureType, int width, int height, int depth, GLenum format, GLenum layout,
		GLenum minFilter, GLenum magFilter, GLenum wrapMode, GLenum bindingId, std::string const& depthTextureName, GLenum dataFormat, const void* data, std::string const& shaderName)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, textureName);

		// Unload any previous version
		if (scene.m_textures.find(textureName) != scene.m_textures.end())
		{
			Debug::log_trace() << "Unloading previous texture: " << textureName << Debug::end;
			glDeleteTextures(1, &scene.m_textures[textureName].m_texture);
		}

		// The created texture object.
		GPU::Texture texture;

		// Store the texture dimensions.
		texture.m_type = textureType;
		texture.m_bindingId = bindingId;
		texture.m_dimensions = glm::ivec3(width, height, depth);
		texture.m_width = width;
		texture.m_height = height;
		texture.m_depth = depth;
		texture.m_format = format;
		texture.m_layout = layout;
		texture.m_wrapMode = wrapMode;
		texture.m_minFilter = minFilter;
		texture.m_magFilter = magFilter;
		texture.m_anisotropy = 16.0f;
		texture.m_mipmapped = minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR ||
			minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_LINEAR;

		// Upload the texture data.
		glGenTextures(1, &texture.m_texture);
		glBindTexture(textureType, texture.m_texture);
		if (height <= 1 && depth <= 1)
		{
			texture.m_numDimensions = 1;
			glTexImage1D(textureType, 0, texture.m_format, texture.m_width, 0, texture.m_layout, dataFormat, data);
		}
		else if (depth <= 1)
		{
			texture.m_numDimensions = 2;
			glTexImage2D(textureType, 0, texture.m_format, texture.m_width, texture.m_height, 0, texture.m_layout, dataFormat, data);
		}
		else
		{
			texture.m_numDimensions = 3;
			glTexImage3D(textureType, 0, texture.m_format, texture.m_width, texture.m_height, texture.m_depth, 0, texture.m_layout, dataFormat, data);
		}
		glm::vec4 borderColor(0.0f, 0.0f, 0.0f, 0.0f);
		glTexParameteri(textureType, GL_TEXTURE_WRAP_S, texture.m_wrapMode);
		glTexParameteri(textureType, GL_TEXTURE_WRAP_T, texture.m_wrapMode);
		glTexParameteri(textureType, GL_TEXTURE_WRAP_R, texture.m_wrapMode);
		glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, texture.m_minFilter);
		glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, texture.m_magFilter);
		glTexParameterf(textureType, GL_TEXTURE_MAX_ANISOTROPY_EXT, texture.m_anisotropy);
		glTexParameterfv(textureType, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(borderColor));
		glBindTexture(textureType, 0);

		// Associate the proper label to it
		glObjectLabel(GL_TEXTURE, texture.m_texture, textureName.length(), textureName.c_str());

		// Generate mipmaps for non-empty textures
		if (texture.m_mipmapped && data != nullptr)
		{
			glBindTexture(textureType, texture.m_texture);
			glGenerateMipmap(textureType);
			glBindTexture(textureType, 0);
		}

		if (texture.m_type == GL_TEXTURE_2D && texture.m_layout != GL_DEPTH_COMPONENT)
		{
			// Create the FBO
			glGenFramebuffers(1, &texture.m_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, texture.m_framebuffer);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.m_texture, 0);
			if (!depthTextureName.empty()) glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, scene.m_textures[depthTextureName].m_texture, 0);

			// Associate the proper label to it
			glObjectLabel(GL_FRAMEBUFFER, texture.m_framebuffer, textureName.length(), textureName.c_str());

			// Configure the draw buffers
			GLuint drawBuffers[] =
			{
				GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
				GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7,
				GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
			};
			glDrawBuffers(1, drawBuffers);

			// Make sure it is complete
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				Debug::log_error() << "Texture framebuffer for texture '" << textureName << "' not complete, cause: " << GPU::enumToString(status) << Debug::end;
			}

			// Render into the texture if we received a shader name
			if (!shaderName.empty())
			{
				// Bind the shader and set the viewport
				glViewport(0, 0, width, height);
				bindShader(scene, shaderName);

				// Render the plane mesh
				glBindVertexArray(scene.m_meshes["plane.obj"].m_vao);
				glDrawElements(GL_TRIANGLES, scene.m_meshes["plane.obj"].m_indexCount, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			// Unbind the framebuffer and texture
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		if (texture.m_type == GL_TEXTURE_2D_ARRAY && texture.m_layout != GL_DEPTH_COMPONENT)
		{
			// Create the FBO
			glGenFramebuffers(1, &texture.m_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, texture.m_framebuffer);
			for (size_t i = 0; i < texture.m_depth; ++i)
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, texture.m_texture, 0, i);
			if (!depthTextureName.empty()) glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, scene.m_textures[depthTextureName].m_texture, 0);

			// Associate the proper label to it
			glObjectLabel(GL_FRAMEBUFFER, texture.m_framebuffer, textureName.length(), textureName.c_str());

			// Configure the draw buffers
			GLuint drawBuffers[] =
			{
				GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
				GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7,
				GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
			};
			glDrawBuffers(texture.m_depth, drawBuffers);

			// Make sure it is complete
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				Debug::log_error() << "Texture framebuffer for texture '" << textureName << "' not complete, cause: " << GPU::enumToString(status) << Debug::end;
			}

			// Unbind the framebuffer and texture
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// Generate mipmaps for non-empty textures
		if (texture.m_mipmapped && !shaderName.empty())
		{
			glBindTexture(textureType, texture.m_texture);
			glGenerateMipmap(textureType);
			glBindTexture(textureType, 0);
		}

		// Store the texture.
		scene.m_textures[textureName] = texture;

		Debug::log_trace() << "Successfully generated texture: " << textureName << Debug::end;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool createTexture(Scene& scene, const std::string& textureName, GLenum textureType, int width, int height, int depth, GLenum format, GLenum layout,
		GLenum minFilter, GLenum magFilter, GLenum wrapMode, GLenum bindingId)
	{
		Debug::log_debug() << "Generating texture: " << textureName << Debug::end;
		Debug::log_debug() << "- Type: " << GPU::enumToString(textureType) << Debug::end;
		Debug::log_debug() << "- Dimensions: " << width << "x" << height << "x" << depth << Debug::end;
		Debug::log_debug() << "- Layout: " << GPU::enumToString(layout) << Debug::end;
		Debug::log_debug() << "- Format: " << GPU::enumToString(format) << Debug::end;
		Debug::log_debug() << "- Min Filter: " << GPU::enumToString(minFilter) << Debug::end;
		Debug::log_debug() << "- Mag Filter: " << GPU::enumToString(magFilter) << Debug::end;
		Debug::log_debug() << "- Wrap: " << GPU::enumToString(wrapMode) << Debug::end;
		Debug::log_debug() << "- Memory: " << Units::bytesToString(GPU::textureSizeBytes(glm::ivec3(width, height, depth), format)) << Debug::end;

		return generateTextureImpl(scene, textureName, textureType, width, height, depth, format, layout, minFilter, magFilter, wrapMode, bindingId, "", GL_UNSIGNED_BYTE, nullptr, "");
	}

	////////////////////////////////////////////////////////////////////////////////
	bool createTexture(Scene& scene, const std::string& textureName, GLenum textureType, int width, int height, int depth, GLenum format, GLenum layout,
		GLenum minFilter, GLenum magFilter, GLenum wrapMode, GLenum dataFormat, const void* data, GLenum bindingId)
	{
		Debug::log_debug() << "Generating texture: " << textureName << Debug::end;
		Debug::log_debug() << "- Type: " << GPU::enumToString(textureType) << Debug::end;
		Debug::log_debug() << "- Dimensions: " << width << "x" << height << "x" << depth << Debug::end;
		Debug::log_debug() << "- Layout: " << GPU::enumToString(layout) << Debug::end;
		Debug::log_debug() << "- Format: " << GPU::enumToString(format) << Debug::end;
		Debug::log_debug() << "- Min Filter: " << GPU::enumToString(minFilter) << Debug::end;
		Debug::log_debug() << "- Mag Filter: " << GPU::enumToString(magFilter) << Debug::end;
		Debug::log_debug() << "- Wrap: " << GPU::enumToString(wrapMode) << Debug::end;
		Debug::log_debug() << "- Data: " << data << Debug::end;
		Debug::log_debug() << "- Memory: " << Units::bytesToString(GPU::textureSizeBytes(glm::ivec3(width, height, depth), format)) << Debug::end;

		return generateTextureImpl(scene, textureName, textureType, width, height, depth, format, layout, minFilter, magFilter, wrapMode, bindingId, "", dataFormat, data, "");
	}

	////////////////////////////////////////////////////////////////////////////////
	bool createTexture(Scene& scene, const std::string& textureName, GLenum textureType, int width, int height, GLenum format, GLenum layout,
		GLenum minFilter, GLenum magFilter, GLenum wrapMode, GLenum depthFormat, GLenum bindingId)
	{
		Debug::log_debug() << "Generating texture: " << textureName << Debug::end;
		Debug::log_debug() << "- Type: " << GPU::enumToString(textureType) << Debug::end;
		Debug::log_debug() << "- Dimensions: " << width << "x" << height << Debug::end;
		Debug::log_debug() << "- Layout: " << GPU::enumToString(layout) << Debug::end;
		Debug::log_debug() << "- Format: " << GPU::enumToString(format) << Debug::end;
		Debug::log_debug() << "- Depth Format: " << GPU::enumToString(wrapMode) << Debug::end;
		Debug::log_debug() << "- Min Filter: " << GPU::enumToString(minFilter) << Debug::end;
		Debug::log_debug() << "- Mag Filter: " << GPU::enumToString(magFilter) << Debug::end;
		Debug::log_debug() << "- Wrap: " << GPU::enumToString(wrapMode) << Debug::end;
		Debug::log_debug() << "- Memory: " << Units::bytesToString(GPU::textureSizeBytes(glm::ivec3(width, height, 1), format)) << Debug::end;

		bool result = true;
		std::string depthTextureName = textureName + "_Depth";
		result &= generateTextureImpl(scene, depthTextureName, textureType, width, height, 1, depthFormat, GL_DEPTH_COMPONENT, minFilter, magFilter, wrapMode, bindingId, "", GL_UNSIGNED_BYTE, nullptr, "");
		result &= generateTextureImpl(scene, textureName, textureType, width, height, 1, format, layout, minFilter, magFilter, wrapMode, bindingId, depthTextureName, GL_UNSIGNED_BYTE, nullptr, "");
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderTexture(Scene& scene, const std::string& textureName, GLenum textureType, int width, int height, int depth, GLenum format, GLenum layout,
		GLenum minFilter, GLenum magFilter, GLenum wrapMode, std::string const& shaderName, GLenum bindingId)
	{
		Debug::log_debug() << "Generating texture: " << textureName << Debug::end;
		Debug::log_debug() << "- Dimensions: " << width << "x" << height << "x" << depth << Debug::end;
		Debug::log_debug() << "- Layout: " << GPU::enumToString(layout) << Debug::end;
		Debug::log_debug() << "- Format: " << GPU::enumToString(format) << Debug::end;
		Debug::log_debug() << "- Min Filter: " << GPU::enumToString(minFilter) << Debug::end;
		Debug::log_debug() << "- Mag Filter: " << GPU::enumToString(magFilter) << Debug::end;
		Debug::log_debug() << "- Wrap: " << GPU::enumToString(wrapMode) << Debug::end;
		Debug::log_debug() << "- Memory: " << Units::bytesToString(GPU::textureSizeBytes(glm::ivec3(width, height, depth), format)) << Debug::end;
		Debug::log_debug() << "- Shader: " << shaderName << Debug::end;

		return generateTextureImpl(scene, textureName, textureType, width, height, depth, format, layout, minFilter, magFilter, wrapMode, bindingId, GL_NONE, GL_UNSIGNED_BYTE, nullptr, shaderName);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool deleteTexture(Scene& scene, const std::string& textureName)
	{
		if (auto it = scene.m_textures.find(textureName); it != scene.m_textures.end())
		{
			glDeleteTextures(1, &it->second.m_texture);
			if (it->second.m_framebuffer != 0) glDeleteFramebuffers(1, &it->second.m_texture);
			scene.m_textures.erase(it);
			return true;
		}
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool createGBuffer(Scene& scene, int numLayers, unsigned bufferWidth, unsigned bufferHeight, GLenum depthFormat, GLenum dataFormat, int samples)
	{
		Debug::log_debug() << "Creating GBuffer [" << bufferWidth << "X" << bufferHeight << "], " << numLayers << " layers, " << samples << " samples." << Debug::end;

		GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		GLuint drawBuffersDepth[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

		for (int i = 0; i < 2; ++i)
		{
			GPU::GBuffer gbuffer;

			/////////////////////////////////////////
			// Normal textures
			/////////////////////////////////////////

			// Store the buffer dimensions for later.
			gbuffer.m_width = bufferWidth;
			gbuffer.m_height = bufferHeight;
			gbuffer.m_dimensions = glm::ivec2(bufferWidth, bufferHeight);
			gbuffer.m_numLayers = numLayers;
			gbuffer.m_numBuffers = 3; // color + normal + specular
			gbuffer.m_samples = samples;

			// Create the depth buffer
			glGenTextures(1, &gbuffer.m_depthTexture);
			glBindTexture(GL_TEXTURE_2D_ARRAY, gbuffer.m_depthTexture);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, depthFormat, bufferWidth, bufferHeight, numLayers, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

			// Create the color textures.
			glGenTextures(2, gbuffer.m_colorTextures);
			for (int j = 0; j < 2; ++j)
			{
				glBindTexture(GL_TEXTURE_2D_ARRAY, gbuffer.m_colorTextures[j]);
				glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, dataFormat, bufferWidth, bufferHeight, numLayers, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			}

			// Create the normal buffer
			glGenTextures(1, &gbuffer.m_normalTexture);
			glBindTexture(GL_TEXTURE_2D_ARRAY, gbuffer.m_normalTexture);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, dataFormat, bufferWidth, bufferHeight, numLayers, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

			// Create the specular buffer
			glGenTextures(1, &gbuffer.m_specularTexture);
			glBindTexture(GL_TEXTURE_2D_ARRAY, gbuffer.m_specularTexture);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, dataFormat, bufferWidth, bufferHeight, numLayers, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

			// Create the gbuffer FBO
			glGenFramebuffers(1, &gbuffer.m_gbuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.m_gbuffer);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer.m_colorTextures[0], 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gbuffer.m_normalTexture, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gbuffer.m_specularTexture, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gbuffer.m_depthTexture, 0);
			glDrawBuffers(sizeof(drawBuffers) / sizeof(GLuint), drawBuffers);

			// Make sure it is complete
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
				Debug::log_error() << "GBuffer [normal] not complete, cause: " << GPU::enumToString(status) << Debug::end;

			// Create the per-layer FBOs
			glGenFramebuffers(gbuffer.m_numLayers, gbuffer.m_gbufferPerLayer);
			for (int k = 0; k < gbuffer.m_numLayers; ++k)
			{
				glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.m_gbufferPerLayer[k]);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer.m_colorTextures[0], 0, k);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gbuffer.m_normalTexture, 0, k);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gbuffer.m_specularTexture, 0, k);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gbuffer.m_depthTexture, 0, k);

				GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
				glDrawBuffers(sizeof(drawBuffers) / sizeof(GLuint), drawBuffers);

				// Make sure it is complete
				status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE)
					Debug::log_error() << "GBuffer [layer #" << k << "] not complete, cause: " << GPU::enumToString(status) << Debug::end;
			}

			// Create the layered color FBO's
			glGenFramebuffers(2, gbuffer.m_colorBuffersLayered);
			for (int j = 0; j < 2; ++j)
			{
				// Attach the corresponding textures
				glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.m_colorBuffersLayered[j]);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer.m_colorTextures[j], 0);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gbuffer.m_depthTexture, 0);

				// Make sure it is complete
				status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE)
					Debug::log_error() << "Gbuffer [layered #" << j << "] not complete, cause: " << GPU::enumToString(status) << Debug::end;
			}

			// Create the per-layer color FBO's
			for (int j = 0; j < 2; ++j)
			{
				// Generate the FBO's
				glGenFramebuffers(gbuffer.m_numLayers, gbuffer.m_colorBuffersPerLayer[j]);

				// Configure all of them
				for (int k = 0; k < gbuffer.m_numLayers; ++k)
				{
					// Attach the corresponding texture layers
					glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.m_colorBuffersPerLayer[j][k]);
					glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer.m_colorTextures[j], 0, k);
					glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gbuffer.m_depthTexture, 0, k);

					// Make sure it is complete
					GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
					if (status != GL_FRAMEBUFFER_COMPLETE)
					{
						Debug::log_error() << "GBuffer color buffer [layer #" << k << "] not complete, cause: " << GPU::enumToString(status) << Debug::end;
					}
				}
			}

			/////////////////////////////////////////
			// MSAA textures
			/////////////////////////////////////////

			// Create the depth buffer
			glGenTextures(1, &gbuffer.m_depthTextureMsaa);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, gbuffer.m_depthTextureMsaa);
			glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, depthFormat, bufferWidth, bufferHeight, numLayers, GL_TRUE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);

			// Create the color textures.
			glGenTextures(1, &gbuffer.m_colorTextureMsaa);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, gbuffer.m_colorTextureMsaa);
			glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, dataFormat, bufferWidth, bufferHeight, numLayers, GL_TRUE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);

			// Create the normal buffer
			glGenTextures(1, &gbuffer.m_normalTextureMsaa);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, gbuffer.m_normalTextureMsaa);
			glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, dataFormat, bufferWidth, bufferHeight, numLayers, GL_TRUE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);

			// Create the specular buffer
			glGenTextures(1, &gbuffer.m_specularTextureMsaa);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, gbuffer.m_specularTextureMsaa);
			glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, dataFormat, bufferWidth, bufferHeight, numLayers, GL_TRUE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);

			// Create the gbuffer FBO
			glGenFramebuffers(1, &gbuffer.m_gbufferMsaa);
			glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.m_gbufferMsaa);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer.m_colorTextureMsaa, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gbuffer.m_normalTextureMsaa, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gbuffer.m_specularTextureMsaa, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gbuffer.m_depthTextureMsaa, 0);
			glDrawBuffers(sizeof(drawBuffers) / sizeof(GLuint), drawBuffers);

			// Make sure it is complete
			status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
				Debug::log_error() << "MSAA GBuffer not complete, cause: " << GPU::enumToString(status) << Debug::end;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Store the gbuffer
			scene.m_gbuffer[i] = gbuffer;
		}

		Debug::log_debug() << "Successfully created GBuffer." << Debug::end;

		// Success
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool createVoxelGrid(Scene& scene, unsigned bufferWidth, unsigned bufferHeight, unsigned bufferDepth, GLenum gbufferDataFormat, GLenum radianceFormat, bool anisotropic)
	{
		Debug::log_debug() << "Creating Voxel grid [" << bufferWidth << "X" << bufferHeight << "X" << bufferDepth << "]" << Debug::end;

		GPU::VoxelGrid voxelGrid;

		/////////////////////////////////////////
		// Normal textures
		/////////////////////////////////////////

		// Store the buffer dimensions for later.
		voxelGrid.m_width = bufferWidth;
		voxelGrid.m_height = bufferHeight;
		voxelGrid.m_depth = bufferDepth;
		voxelGrid.m_dimensions = glm::ivec3(bufferWidth, bufferHeight, bufferDepth);
		voxelGrid.m_gbufferDataFormat = gbufferDataFormat;
		voxelGrid.m_radianceDataFormat = radianceFormat;
		voxelGrid.m_anisotropic = anisotropic;

		// Albedo
		glGenTextures(1, &voxelGrid.m_albedoTexture);
		glBindTexture(GL_TEXTURE_3D, voxelGrid.m_albedoTexture);
		glTexImage3D(GL_TEXTURE_3D, 0, gbufferDataFormat, bufferWidth, bufferHeight, bufferDepth, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glBindTexture(GL_TEXTURE_3D, 0);

		// Normal
		glGenTextures(1, &voxelGrid.m_normalTexture);
		glBindTexture(GL_TEXTURE_3D, voxelGrid.m_normalTexture);
		glTexImage3D(GL_TEXTURE_3D, 0, gbufferDataFormat, bufferWidth, bufferHeight, bufferDepth, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glBindTexture(GL_TEXTURE_3D, 0);

		// Specular
		glGenTextures(1, &voxelGrid.m_specularTexture);
		glBindTexture(GL_TEXTURE_3D, voxelGrid.m_specularTexture);
		glTexImage3D(GL_TEXTURE_3D, 0, gbufferDataFormat, bufferWidth, bufferHeight, bufferDepth, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glBindTexture(GL_TEXTURE_3D, 0);
		
		// Radiance
		glGenTextures(1, &voxelGrid.m_radianceTexture);
		glBindTexture(GL_TEXTURE_3D, voxelGrid.m_radianceTexture);
		glTexImage3D(GL_TEXTURE_3D, 0, radianceFormat, bufferWidth, bufferHeight, bufferDepth, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
		glGenerateMipmap(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, 0);

		// Radiance mipmaps
		if (anisotropic)
		{
			glGenTextures(6, voxelGrid.m_radianceMipmaps);
			for (int i = 0; i < 6; ++i)
			{
				glBindTexture(GL_TEXTURE_3D, voxelGrid.m_radianceMipmaps[i]);
				glTexImage3D(GL_TEXTURE_3D, 0, radianceFormat, bufferWidth / 2, bufferHeight / 2, bufferDepth / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
				glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
				glGenerateMipmap(GL_TEXTURE_3D);
				glBindTexture(GL_TEXTURE_3D, 0);
			}
		}

		// Store the voxel grid in the scene
		scene.m_voxelGrid = voxelGrid;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	static GLbitfield s_persistentBufferFlags = GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;

	////////////////////////////////////////////////////////////////////////////////
	bool createGPUBuffer(Scene& scene, const std::string& bufferName, GLenum bufferType, bool immutable, bool indexed, 
		GLuint bindingId, GLenum flags, GLsizeiptr storageSize, GLsizeiptr elementSize)
	{
		// Make sure no such one exists already.
		if (scene.m_genericBuffers.find(bufferName) != scene.m_genericBuffers.end())
			return true;

		Debug::log_debug() << "Creating GPU buffer \"" << bufferName << "\" of size " << Units::bytesToString(storageSize) << "(s)." << Debug::end;

		// The created buffer
		GPU::GenericBuffer buffer;

		// Store the properties of the buffer
		buffer.m_indexed = indexed;
		buffer.m_bindingId = bindingId;
		buffer.m_immutable = immutable;
		buffer.m_persistentlyMapped = (flags & s_persistentBufferFlags) == s_persistentBufferFlags;
		buffer.m_bufferType = bufferType;
		buffer.m_flags = flags;
		buffer.m_size = storageSize;
		buffer.m_elementSize = elementSize;
		buffer.m_totalSize = buffer.m_persistentlyMapped ? storageSize * 3 : storageSize;

		// Create the buffer object.
		glGenBuffers(1, &buffer.m_buffer);
		glBindBuffer(buffer.m_bufferType, buffer.m_buffer);
		glObjectLabel(GL_BUFFER, buffer.m_buffer, bufferName.length(), bufferName.c_str());

		// Allocate space, if needed
		if (buffer.m_totalSize != 0)
		{
			// Special treatment for immutable buffers
			if (buffer.m_immutable)
				glBufferStorage(buffer.m_bufferType, buffer.m_totalSize, nullptr, buffer.m_flags);

			// Regular glBufferData allocation
			else
				glBufferData(buffer.m_bufferType, buffer.m_totalSize, nullptr, GL_STREAM_DRAW);
		}

		// Map the buffer for persistently mapped buffers
		if (buffer.m_persistentlyMapped)
		{
			char* dataPtr = (char*) glMapBufferRange(buffer.m_bufferType, 0, buffer.m_totalSize, s_persistentBufferFlags);
			for (size_t i = 0; i < 3; ++i) buffer.m_persistentRegions[0] = dataPtr + i * buffer.m_size;
		}

		// Unbind the buffer, we are done with it
		glBindBuffer(buffer.m_bufferType, 0);

		// Store the buffer
		scene.m_genericBuffers[bufferName] = buffer;

		Debug::log_debug() << "GPU buffer \"" << bufferName << "\" successfully created." << Debug::end;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool resizeGPUBuffer(Scene& scene, const std::string& bufferName, size_t newSize, bool growOnly)
	{
		// Make sure the buffer exists
		if (scene.m_genericBuffers.find(bufferName) == scene.m_genericBuffers.end())
		{
			Debug::log_error() << "Attempting to resize non-existent GPU buffer \"" << bufferName << "\"" << Debug::end;
			return false;
		}

		// Extract the buffer object
		GPU::GenericBuffer& buffer = scene.m_genericBuffers[bufferName];

		// Only resize the buffer if the current size is smaller than the requested size
		if (growOnly && buffer.m_size >= newSize)
		{
			Debug::log_debug() << "Skipping GPU buffer resize for buffer \"" << bufferName << "\";"
				<< " current size (" << Units::bytesToString(buffer.m_size) << ")"
				<< " is larger than requested size (" << Units::bytesToString(newSize) << ")."
				<< Debug::end;

			return true;
		}

		Debug::log_debug() << "Resizing GPU buffer \"" << bufferName << "\" of size " << Units::bytesToString(newSize) << "(s)." << Debug::end;

		// Store the new buffer size
		buffer.m_size = newSize;
		buffer.m_totalSize = buffer.m_persistentlyMapped ? newSize * 3 : newSize;

		// Immutable buffers must be re-created
		if (buffer.m_immutable)
		{
			glDeleteBuffers(1, &buffer.m_buffer);
			glGenBuffers(1, &buffer.m_buffer);
			glBindBuffer(buffer.m_bufferType, buffer.m_buffer);
			glObjectLabel(GL_BUFFER, buffer.m_buffer, bufferName.length(), bufferName.c_str());
		}

		// Bind the buffer to modify it
		glBindBuffer(buffer.m_bufferType, buffer.m_buffer);

		// Immutable buffers must use glBufferStorage
		if (buffer.m_immutable) glBufferStorage(buffer.m_bufferType, buffer.m_totalSize, nullptr, buffer.m_flags);

		// Otherwise simply update via glBufferData
		else glBufferData(buffer.m_bufferType, buffer.m_totalSize, nullptr, GL_STREAM_DRAW);

		// Map the buffer for persistently mapped buffers
		if (buffer.m_persistentlyMapped)
		{
			char* dataPtr = (char*)glMapBufferRange(buffer.m_bufferType, 0, buffer.m_totalSize, s_persistentBufferFlags);
			for (size_t i = 0; i < 3; ++i) buffer.m_persistentRegions[0] = dataPtr + i * buffer.m_size;
		}

		// Unbind the buffer, we are done with it
		glBindBuffer(buffer.m_bufferType, 0);

		Debug::log_debug() << "GPU buffer \"" << bufferName << "\" successfully resized." << Debug::end;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool deleteGPUBuffer(Scene& scene, const std::string& bufferName)
	{
		if (auto it = scene.m_genericBuffers.find(bufferName); it != scene.m_genericBuffers.end())
		{
			glDeleteBuffers(1, &it->second.m_buffer);
			scene.m_genericBuffers.erase(it);
			return true;
		}
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool createPerfCounter(Scene& scene, const std::string& counterName)
	{
		// Make sure no such one exists already.
		if (scene.m_perfCounters.find(counterName) != scene.m_perfCounters.end())
			return true;

		// The created counter
		GPU::PerfCounter counter;

		// Create the buffer object.
		glGenQueries(2, counter.m_counters);

		// Store the perf counter
		scene.m_perfCounters[counterName] = counter;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool createGpuOcclusionQuery(Scene& scene, const std::string& queryName)
	{
		// Make sure no such one exists already.
		if (scene.m_occlusionQueries.find(queryName) != scene.m_occlusionQueries.end())
			return true;

		// The created query
		GPU::OcclusionQuery query;

		// Create the buffer object.
		glGenQueries(1, &query.m_query);

		// Store the occlusion query
		scene.m_occlusionQueries[queryName] = query;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding shaders. */
	void bindShader(Scene& scene, GPU::Shader const& shader)
	{
		glUseProgram(shader.m_program);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding shaders. */
	void bindShader(Scene& scene, std::string const& shaderName)
	{
		if (scene.m_shaders.find(shaderName) != scene.m_shaders.end())
			bindShader(scene, scene.m_shaders[shaderName]);
	}

	////////////////////////////////////////////////////////////////////////////////
	void bindShader(Scene& scene, std::string const& categoryName, std::string const& shaderName)
	{
		bindShader(scene, Asset::getShaderName(categoryName, shaderName));
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding framebuffers. */
	void bindFramebuffer(Scene& scene, GPU::Texture const& texture)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, texture.m_framebuffer);
		glViewport(0, 0, texture.m_width, texture.m_height);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding framebuffers. */
	void bindFramebuffer(Scene& scene, std::string const& framebufferName)
	{
		bindFramebuffer(scene, scene.m_textures[framebufferName]);
	}

	////////////////////////////////////////////////////////////////////////////////
	void bindTexture(GPU::Texture const& texture, GLenum index)
	{
		glActiveTexture(index);
		glBindTexture(texture.m_type, texture.m_texture);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding textures. */
	void bindTexture(GPU::Texture const& texture, GPU::TextureIndices index)
	{
		bindTexture(texture, GL_TEXTURE0 + int(index));
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding textures. */
	void bindTexture(Scene& scene, std::string const& textureName, GPU::TextureIndices index)
	{
		bindTexture(scene.m_textures[textureName], index);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding textures. */
	void bindTexture(Scene& scene, std::string const& textureName)
	{
		bindTexture(scene.m_textures[textureName], scene.m_textures[textureName].m_bindingId);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void bindBuffer(GPU::GenericBuffer const& buffer, GPU::UniformBufferIndices index)
	{
		// TODO: consider triple buffering for persistently mapped buffers

		if (buffer.m_indexed)
		{
			glBindBufferBase(buffer.m_bufferType, index, buffer.m_buffer);
		}
		else
		{
			glBindBuffer(buffer.m_bufferType, buffer.m_buffer);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void bindBuffer(Scene& scene, std::string const& uboName)
	{
		bindBuffer(scene.m_genericBuffers[uboName], GPU::UniformBufferIndices(scene.m_genericBuffers[uboName].m_bindingId));
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void bindBuffer(Scene& scene, std::string const& uboName, GPU::UniformBufferIndices index)
	{
		bindBuffer(scene.m_genericBuffers[uboName], index);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for unbinding buffers. */
	void unbindBuffer(GPU::GenericBuffer const& buffer, GPU::UniformBufferIndices index)
	{
		if (buffer.m_indexed)
		{
			glBindBufferBase(buffer.m_bufferType, index, 0);
		}
		else
		{
			glBindBuffer(buffer.m_bufferType, 0);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void unbindBuffer(Scene& scene, std::string const& uboName)
	{
		unbindBuffer(scene.m_genericBuffers[uboName], GPU::UniformBufferIndices(scene.m_genericBuffers[uboName].m_bindingId));
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Helper function for binding buffers. */
	void unbindBuffer(Scene& scene, std::string const& uboName, GPU::UniformBufferIndices index)
	{
		unbindBuffer(scene.m_genericBuffers[uboName], index);
	}

	////////////////////////////////////////////////////////////////////////////////
	void uploadBufferData(GPU::GenericBuffer& buffer, std::string const& bufferName, size_t size, const void* data)
	{
		// TODO: use memcpy for persistently mapped buffers
		GLint bufferSize;
		glGetBufferParameteriv(buffer.m_bufferType, GL_BUFFER_SIZE, &bufferSize);

		Debug::log_trace() << "Uploading uniform data to " << bufferName << "; "
			<< "data size: " << Units::bytesToString(size) << ", "
			<< "buffer size: " << Units::bytesToString(buffer.m_size) 
			<< " (" << Units::bytesToString(bufferSize) << ")"
			<< Debug::end;

		// Use glBufferSubData for immutable buffers / buffers with enough size
		if (buffer.m_size >= size)
		{
			glBufferSubData(buffer.m_bufferType, 0, size, data);
		}

		// Otherwise use regular glBufferData to update the buffer memory as well
		else
		{
			buffer.m_size = size;
			glBufferData(buffer.m_bufferType, size, data, GL_STREAM_DRAW);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void uploadBufferData(Scene& scene, std::string const& uboName, size_t size, const void* data)
	{
		GPU::GenericBuffer& ubo = scene.m_genericBuffers[uboName];

		bindBuffer(ubo, GPU::UniformBufferIndices(ubo.m_bindingId));
		uploadBufferData(ubo, uboName, size, data);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool waitForGpu(Scene& scene, const size_t maxWaitPeriod)
	{
		// Place down a GPU sync
		GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

		// Wait for the fence to become signaled
		size_t timeWaited = 0;
		GLenum fenceStatus = GL_UNSIGNALED;
		while ((maxWaitPeriod <= 0 || timeWaited <= maxWaitPeriod) && fenceStatus != GL_CONDITION_SATISFIED && fenceStatus != GL_ALREADY_SIGNALED)
		{
			fenceStatus = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 10);
			timeWaited += 10;
		}

		// It was a success if the fence was signaled
		return fenceStatus == GL_CONDITION_SATISFIED || fenceStatus == GL_ALREADY_SIGNALED;
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseTextures(Scene& scene, Object* object)
	{
		// Delete the textures.
		for (auto texture : scene.m_textures)
		{
			glDeleteTextures(1, &texture.second.m_texture);
			if (texture.second.m_framebuffer != 0) glDeleteFramebuffers(1, &texture.second.m_texture);
		}
		scene.m_textures.clear();
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseMeshes(Scene& scene, Object* object)
	{
		// Delete the meshes.
		for (auto mesh : scene.m_meshes)
		{
			glDeleteVertexArrays(1, &mesh.second.m_vao);
			glDeleteBuffers(1, &mesh.second.m_vboPosition);
			glDeleteBuffers(1, &mesh.second.m_vboNormal);
			glDeleteBuffers(1, &mesh.second.m_vboTangent);
			glDeleteBuffers(1, &mesh.second.m_vboBitangent);
			glDeleteBuffers(1, &mesh.second.m_vboUV);
			glDeleteBuffers(1, &mesh.second.m_ibo);
			glDeleteBuffers(1, &mesh.second.m_mbo);
		}
		scene.m_meshes.clear();
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseShaders(Scene& scene, Object* object)
	{
		// Delete the shaders.
		for (auto shader : scene.m_shaders)
		{
			glDeleteProgram(shader.second.m_program);
		}
		scene.m_shaders.clear();
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseGbuffer(Scene& scene, Object* object)
	{
		// Delete the gbuffer resources.
		for (int i = 0; i < 2; ++i)
		{
			glDeleteTextures(1, &scene.m_gbuffer[i].m_depthTexture);
			glDeleteTextures(2, scene.m_gbuffer[i].m_colorTextures);
			glDeleteTextures(1, &scene.m_gbuffer[i].m_normalTexture);
			glDeleteTextures(1, &scene.m_gbuffer[i].m_specularTexture);
			glDeleteFramebuffers(1, &scene.m_gbuffer[i].m_gbuffer);
			glDeleteFramebuffers(scene.m_gbuffer[i].m_numLayers, scene.m_gbuffer[i].m_gbufferPerLayer);
			glDeleteFramebuffers(2, scene.m_gbuffer[i].m_colorBuffersLayered);
			for (size_t i = 0; i < 2; ++i)
			{
				glDeleteFramebuffers(scene.m_gbuffer[i].m_numLayers, scene.m_gbuffer[i].m_colorBuffersLayered);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseVoxelGrid(Scene& scene, Object* object)
	{
		// Delete the voxel grid resources.
		glDeleteTextures(1, &scene.m_voxelGrid.m_albedoTexture);
		glDeleteTextures(1, &scene.m_voxelGrid.m_normalTexture);
		glDeleteTextures(1, &scene.m_voxelGrid.m_specularTexture);
		glDeleteTextures(1, &scene.m_voxelGrid.m_radianceTexture);
		if (scene.m_voxelGrid.m_radianceMipmaps[0] != 0) glDeleteTextures(6, scene.m_voxelGrid.m_radianceMipmaps);
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseGpuBuffers(Scene& scene, Object* object)
	{
		// Delete the uniform buffers.
		for (auto genericBuffer : scene.m_genericBuffers)
		{
			glDeleteBuffers(1, &genericBuffer.second.m_buffer);
		}
		scene.m_genericBuffers.clear();
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseOcclusionQueries(Scene& scene, Object* object)
	{
		// Delete the occlusion queries.
		for (auto query : scene.m_occlusionQueries)
		{
			glDeleteQueries(1, &query.second.m_query);
		}
		scene.m_occlusionQueries.clear();
	}

	////////////////////////////////////////////////////////////////////////////////
	void releasePerfCounters(Scene& scene, Object* object)
	{
		// Delete the perf counters.
		for (auto counter : scene.m_perfCounters)
		{
			glDeleteQueries(2, counter.second.m_counters);
		}
		scene.m_perfCounters.clear();
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseFonts(Scene& scene, Object* object)
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseNeuralNetworks(Scene& scene, Object* object)
	{
		for (auto model : scene.m_tfModels)
		{
			TensorFlow::releaseModel(model.second);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//  SCENE LIFECYCLE
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	ObjectUpdateFunctions::iterator findEntry(ObjectUpdateFunctions& graph, int refObjectId)
	{
		return std::find_if(graph.begin(), graph.end(), [refObjectId](ObjectUpdateFunctions::value_type const& item) { return item.m_objectType == refObjectId; });
	}

	////////////////////////////////////////////////////////////////////////////////
	ObjectRenderFunctions::iterator findEntry(ObjectRenderFunctions& graph, std::string const& refName)
	{
		return std::find_if(graph.begin(), graph.end(), [refName](ObjectRenderFunctions::value_type const& item) { return item.m_name == refName; });
	}

	////////////////////////////////////////////////////////////////////////////////
	bool emptyReference(int refObjectId)
	{
		return refObjectId == 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool emptyReference(std::string const& refName)
	{
		return refName.empty();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string const& nodeName(ObjectUpdateFunctions const& graph, size_t id)
	{
		return objectNames()[graph[id].m_objectType];
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string const& nodeName(ObjectRenderFunctions const& graph, size_t id)
	{
		return graph[id].m_name;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string const& nodeName(ObjectUpdateFunctionRegistrator const& node)
	{
		return objectNames()[std::get<3>(node).m_objectType];
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string const& nodeName(ObjectRenderFunctionRegistrator const& node)
	{
		return std::get<3>(node).m_name;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string const& referenceName(ObjectUpdateFunctionRegistrator const& node)
	{
		return objectNames()[std::get<0>(node)];
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string const& referenceName(ObjectRenderFunctionRegistrator const& node)
	{
		return std::get<0>(node);
	}

	////////////////////////////////////////////////////////////////////////////////
	int nodePriority(ObjectUpdateFunctionRegistrator const& node)
	{
		return std::get<2>(node);
	}

	////////////////////////////////////////////////////////////////////////////////
	int nodePriority(ObjectRenderFunctionRegistrator const& node)
	{
		return std::get<2>(node);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename R, typename G>
	void buildGraph(std::string const& name, R registrators, G& graph)
	{
		// Skip this if the graph isn't empty
		if (graph.size() > 0) return;

		// Pre-sort the list by priority
		std::sort(registrators.begin(), registrators.end(), [](auto const& a, auto const& b) { return nodePriority(a.second) < nodePriority(b.second); });

		Debug::log_debug() << "Building " << name << " graph from " << registrators.size() << " entries" << Debug::end;
		Debug::log_debug() << std::string(80, '-') << Debug::end;
		for (auto const& registratorIt : registrators)
		{
			auto [objectId, registrator] = registratorIt;
			auto [refId, relation, priority, fn] = registrator;

			Debug::log_debug() << " - " << nodeName(registrator) << ": " << priority << Debug::end;
		}
		Debug::log_debug() << std::string(80, '-') << Debug::end;
		while (registrators.size() > 0)
		{
			// New set of registrators
			R newRegistrators;
			bool anyProcessed = false;
			int lowestPriority = 999;
			for (auto const& registratorIt : registrators)
			{
				auto [objectId, registrator] = registratorIt;
				auto [refId, relation, priority, fn] = registrator;
				lowestPriority = std::min(lowestPriority, priority);
			}

			Debug::log_debug() << std::string(80, '-') << Debug::end;
			Debug::log_debug() << "Starting pass with " << registrators.size() << " entries; lowest priority: " << lowestPriority << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;

			for (auto const& registratorIt : registrators)
			{
				// Object id and registrator
				auto [objectId, registrator] = registratorIt;
				auto [refId, relation, priority, fn] = registrator;
				bool processed = false;

				// Ignore nodes with a lower priority
				if (priority > lowestPriority)
				{
					Debug::log_debug() << " - Ignoring " << nodeName(registrator) << " based on priority (" << priority << " > " << lowestPriority << ")" << Debug::end;
					newRegistrators.push_back(registratorIt);
					continue;
				}

				if (emptyReference(refId))
				{
					auto targetIt = graph.end();

					// insert before end = last
					if (relation < 0) targetIt = graph.end();
					// insert after begin = first
					else if (relation > 0) targetIt = graph.begin();

					Debug::log_debug() << " - Inserting " << nodeName(registrator) << " at: " << std::distance(graph.begin(), targetIt) << Debug::end;

					// Insert into the graph
					graph.insert(targetIt, fn);

					//Debug::log_debug() << std::string(40, '-') << Debug::end;
					//Debug::log_debug() << "Current " << name << " graph:" << Debug::end;
					//for (size_t i = 0; i < graph.size(); ++i)
					//{
					//	Debug::log_debug() << (i + 1) << ". " << nodeName(graph, i) << Debug::end;
					//}
					//Debug::log_debug() << std::string(80, '-') << Debug::end;

					// Mark the object type as processed
					processed = anyProcessed = true;
				}
				else
				{
					// Look for the referenced component and consume it if found
					if (auto refIt = findEntry(graph, refId); refIt != graph.end())
					{
						// Location to insert at
						auto targetIt = refIt;

						// insert before 
						if (relation < 0) targetIt = refIt;

						// insert after
						else if (relation > 0) targetIt = refIt + 1;

						Debug::log_debug() << " - Inserting " << nodeName(registrator) << " at: " << std::distance(graph.begin(), targetIt) << Debug::end;

						// Insert into the graph
						graph.insert(targetIt, fn);

						//Debug::log_debug() << std::string(40, '-') << Debug::end;
						//Debug::log_debug() << "Current " << name << " graph:" << Debug::end;
						//for (size_t i = 0; i < graph.size(); ++i)
						//{
						//	Debug::log_debug() << (i + 1) << ". " << nodeName(graph, i) << Debug::end;
						//}
						//Debug::log_debug() << std::string(80, '-') << Debug::end;

						// Mark the object type as processed
						processed = anyProcessed = true;
					}
					else
					{
						Debug::log_debug() << " - Ignoring " << nodeName(registrator) << "; requirements not satisfied (\"" << referenceName(registrator) << "\" not found)" << Debug::end;
					}
				}

				// Append to the not-yet-processed list
				if (processed == false) newRegistrators.push_back(registratorIt);
			}

			// Continue in the next iteration from the remaining registrators
			registrators = newRegistrators;

			// Log how many we have left
			Debug::log_debug() << std::string(80, '-') << Debug::end;
			Debug::log_debug() << "Finished pass with " << registrators.size() << " entries left." << Debug::end;
			Debug::log_debug() << std::string(80, '-') << Debug::end;

			if (anyProcessed == false)
			{
				Debug::log_error() << "Unable to construct a valid " << name << " graph: potentially ambiguous constraints" << Debug::end;
				Debug::log_debug() << "Entries not placed: " << Debug::end;
				for (auto const& registratorIt : registrators)
				{
					auto [objectId, registrator] = registratorIt;
					auto [refId, relation, priority, fn] = registrator;
					Debug::log_debug() << " - " << nodeName(registrator) << " (" << priority << ", " << referenceName(registrator) << ")" << Debug::end;
				}
				break;
			}
		}

		// Print the final graph
		Debug::log_debug() << "Final " << name << " graph:" << Debug::end;
		Debug::log_debug() << std::string(80, '-') << Debug::end;
		for (size_t i = 0; i < graph.size(); ++i)
		{
			Debug::log_debug() << (i + 1) << ". " << nodeName(graph, i) << Debug::end;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	Scene::Scene()
	{
		// Initialize the profiler tree
		for (size_t i = 0; i < m_profilerWritePosition.size(); ++i)
		{
			m_profilerWritePosition[i] = m_profilerTree[m_profilerBufferWriteId][i].set_head(Profiler::ProfilerTreeEntry{});
		}

		Scene& scene = *this;

		// Store the global resource releasers
		appendResourceReleaser(scene, scene.m_name, GBuffer, releaseGbuffer, "GBuffer");
		appendResourceReleaser(scene, scene.m_name, GBuffer, releaseVoxelGrid, "Voxel Grid");
		appendResourceReleaser(scene, scene.m_name, Shader, releaseShaders, "OpenGL Shaders");
		appendResourceReleaser(scene, scene.m_name, Mesh, releaseMeshes, "Meshes");
		appendResourceReleaser(scene, scene.m_name, Texture, releaseTextures, "Textures");
		appendResourceReleaser(scene, scene.m_name, OcclusionQuery, releaseOcclusionQueries, "Occlusion Queries");
		appendResourceReleaser(scene, scene.m_name, PerfCounter, releasePerfCounters, "Performance Counters");
		appendResourceReleaser(scene, scene.m_name, Font, releaseFonts, "Fonts");
		appendResourceReleaser(scene, scene.m_name, NeuralNetwork, releaseNeuralNetworks, "Neural Networks");
	}

	////////////////////////////////////////////////////////////////////////////////
	void setupScene(Scene& scene)
	{
		// Build the update graph
		Debug::log_debug() << std::string(80, '=') << Debug::end;
		buildGraph("update", objectUpdateFunctionRegistrators(), objectUpdateFunctions());
		Debug::log_debug() << std::string(80, '=') << Debug::end;
		buildGraph("render (OpenGL)", objectRenderFunctionRegistratorsOpenGL(), objectRenderFunctionsOpenGL());
		Debug::log_debug() << std::string(80, '=') << Debug::end;

		// Create the default demo objects
		if (scene.m_isDemoScene)
		{
			for (auto const& objectSetup : objectDemoSceneSetupFunctions())
			{
				objectSetup.second(scene);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void teardownScene(Scene& scene)
	{}

	////////////////////////////////////////////////////////////////////////////////
	void rebuildFirstObjectAccelStructure(Scene& scene)
	{
		// Clear the old values
		scene.m_firstObjects.clear();

		// Start by trying to fill the vector with ungrouped objects (for the various settings objects)
		for (auto objectType : objectTypes())
		{
			auto const& objects = filterObjects(scene, objectType, [&](Object* object){ return object->m_groups == 0; }, true, false, false);
			if (!objects.empty()) scene.m_firstObjects[objectType] = objects[0];
		}

		// Fill in the missing objects from the grouped objects
		for (auto objectType : objectTypes())
		{
			if (scene.m_firstObjects.find(objectType) == scene.m_firstObjects.end())
			{
				auto const& objects = filterObjects(scene, objectType, true, false, true);
				scene.m_firstObjects[objectType] = objects.empty() ? nullptr : objects[0];
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateScene(Scene& scene)
	{
		// Rebuild the first object acceleration structure
		rebuildFirstObjectAccelStructure(scene);

		// Simulation settings
		Object* simulationSettings = findFirstObject(scene, OBJECT_TYPE_SIMULATION_SETTINGS);

		// Update the various object types
		for (auto it : objectUpdateFunctions())
		{
			// Extaxt the data
			auto [objectType, objectUpdateFunction] = it;

			Debug::log_trace() << "Updating object type: " << objectNames()[objectType] << Debug::end;

			Profiler::ScopedCpuPerfCounter perfCounter(scene, objectNames()[objectType]);

			// make sure that newly requested resources are loaded immediately
			loadResources(scene);

			// Go through each object of the corresponding object type
			for (auto object : filterObjects(scene, objectType, false, false))
			{
				Debug::DebugRegion region({ object->m_name });

				Debug::log_trace() << "Updating object: " << object->m_name << Debug::end;

				Profiler::ScopedCpuPerfCounter perfCounter(scene, object->m_name);

				// Invoke it's update callback
				objectUpdateFunction(scene, simulationSettings, object);

				Debug::log_trace() << "Updating object finished: " << object->m_name << Debug::end;
			}

			Debug::log_trace() << "Updating object type finished: " << objectNames()[objectType] << Debug::end;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderScene(Scene& scene, ObjectRenderFunctions const& renderFunctions, ObjectRenderParameters renderParameters)
	{
		// List of render functions to disable
		auto& disabledFunctions = renderParameters.m_renderSettings->component<RenderSettings::RenderSettingsComponent>().m_disabledFunctions;

		// Render the various object types
		for (auto const& function : renderFunctions)
		{
			Debug::DebugRegion region({ function.m_name });

			// Store the function name in the render params
			renderParameters.m_functionName = function.m_name;

			// Skip disabled render functions
			if (disabledFunctions.find(renderParameters.m_functionName) != disabledFunctions.end())
			{
				Debug::log_trace() << "Skipping disabled render function: " << renderParameters.m_functionName << Debug::end;
				break;
			}

			// Invoke the object type condition function
			while (function.m_callbackCondition(scene, renderParameters.m_simulationSettings, renderParameters.m_renderSettings, renderParameters.m_camera, renderParameters.m_functionName))
			{
				Profiler::ScopedGpuPerfCounter perfCounter(scene, function.m_name, true);

				Debug::log_trace() << "Invoking render function: " << renderParameters.m_functionName << Debug::end;

				// Push in the debug group
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, renderParameters.m_functionName.length(), renderParameters.m_functionName.c_str());

				// Call the pre-state callback, if any
				if (function.m_preState) function.m_preState(scene, renderParameters.m_simulationSettings, renderParameters.m_renderSettings, renderParameters.m_camera, renderParameters.m_functionName);

				// Make sure we actually have a render callback
				if (function.m_renderFunction)
				{
					// Go through each object of the corresponding object type
					for (auto object : filterObjects(scene, function.m_objectType, false, false))
					{
						Debug::DebugRegion region({ object->m_name });

						// Store the object in the render params
						renderParameters.m_object = object;

						// Invoke the object condition
						while (function.m_objectCondition(scene, renderParameters.m_simulationSettings, renderParameters.m_renderSettings, renderParameters.m_camera, renderParameters.m_functionName, renderParameters.m_object))
						{
							Debug::log_trace() << "Rendering object: " << object->m_name << Debug::end;

							Profiler::ScopedGpuPerfCounter perfCounter(scene, object->m_name, true);
							//Profiler::ScopedCpuPerfCounter perfCounter(scene, object->m_name, true);

							// Push in the debug group
							glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, object->m_name.length(), object->m_name.c_str());

							// Invoke it's GUI generator callback
							function.m_renderFunction(scene, renderParameters.m_simulationSettings, renderParameters.m_renderSettings, renderParameters.m_camera, renderParameters.m_functionName, renderParameters.m_object);

							// Pop the debug group
							glPopDebugGroup();

							Debug::log_trace() << "Rendering object finished: " << object->m_name << Debug::end;
						}
					}
				}

				// Call the post-state callback, if any
				if (function.m_postState) function.m_postState(scene, renderParameters.m_simulationSettings, renderParameters.m_renderSettings, renderParameters.m_camera, renderParameters.m_functionName);

				// Pop the debug group
				glPopDebugGroup();

				Debug::log_trace() << "Invoking render function finished: " << renderParameters.m_functionName << Debug::end;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderScene(Scene& scene)
	{
		// Perf counter
		Profiler::ScopedGpuPerfCounter perfCounter(scene, scene.m_name);

		ObjectRenderParameters renderParameters;

		// Various settings objects and render camera
		renderParameters.m_simulationSettings = findFirstObject(scene, OBJECT_TYPE_SIMULATION_SETTINGS);
		renderParameters.m_renderSettings = findFirstObject(scene, OBJECT_TYPE_RENDER_SETTINGS);
		renderParameters.m_camera = RenderSettings::getMainCamera(scene, renderParameters.m_renderSettings);

		// No camera available
		if (renderParameters.m_camera == nullptr || !SimulationSettings::isObjectEnabled(scene, renderParameters.m_camera))
		{
			Debug::log_error() << "Unable to render scene; no camera available." << Debug::end;
			return;
		}

		// Skip rendering if the window is not in focus
		if (renderParameters.m_simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_focused == false && 
			renderParameters.m_renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_backgroundRendering == false)
			return;

		Debug::log_trace() << "Rendering scene: " << scene.m_name << ", using camera: " << renderParameters.m_camera->m_name << Debug::end;

		// Render with the corresponding render functions
		switch (renderParameters.m_renderSettings->component<RenderSettings::RenderSettingsComponent>().m_rendering.m_renderer)
		{
		case RenderSettings::OpenGL:
			renderScene(scene, objectRenderFunctionsOpenGL(), renderParameters);
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void mainLoop(Scene& scene)
	{
		// Extract the simulation settings object
		Object* simulationSettings = findFirstObject(scene, OBJECT_TYPE_SIMULATION_SETTINGS);

		// Initialize the time management variables
		simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId = 0;
		simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_fixedDeltaTime = 1.0f / simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_maxFps;
		simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastUpdateTime = glfwGetTime() - simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_fixedDeltaTime;
		simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastUpdateSecond = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastUpdateTime;
		simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_numFrames = 0;
		simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastFPS = 0;
		simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_globalTime = 0.0f;
		simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_simulationStarted = true;
		time(&simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_startupDate);

		while (!glfwWindowShouldClose(scene.m_context.m_window))
		{
			// Minimum time between two consecutive updates
			const float minUpdateTime = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_fixedDeltaTime;

			// Calculate the delta time
			double currentTime = glfwGetTime();

			// Don't do anything if not enough time has passed
			if (currentTime - simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastUpdateTime < minUpdateTime)
				continue;

			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_deltaTime = currentTime - simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastUpdateTime;
			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_scaledDeltaTime = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_deltaTime * simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_timeDilation;

			// Update the global time
			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_globalTime += simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_deltaTime;
			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_scaledGlobalTime += simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_scaledDeltaTime;

			// Store the frame start time
			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameStartTimepoint[simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId] = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_globalTime;

			// Handle the FPS
			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_numFrames++;
			if (currentTime - simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastUpdateSecond > 1.0)
			{
				simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastFPS = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_numFrames;
				simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastUpdateSecond = currentTime;
				simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_numFrames = 0;
			}
			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_smoothFPS = 1.0 / simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_deltaTime;
			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_focused = glfwGetWindowAttrib(scene.m_context.m_window, GLFW_FOCUSED);

			// Skip the remainder if we are not simulating in the background
			if (simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_focused == false && simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_simulateInBackground == false)
			{
				// Consume events
				glfwPollEvents();

				// Skip the rest
				continue;
			}

			// Store the FPS
			Profiler::storeData(scene, { "Render", "Frame#" }, (int)simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId);
			Profiler::storeData(scene, { "Render", "FPS (Current)" }, (float)simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_smoothFPS);
			Profiler::storeData(scene, { "Render", "FPS" }, (int)simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastFPS);
			
			// End the current profiler frame
			Profiler::endFrame(scene);

			// Start a new profiler frame
			Profiler::beginFrame(scene);

			// Update the scene and its object
			{
				Debug::DebugRegion region({ "Frame #"s + std::to_string(simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId), "Update" });
				Profiler::ScopedCpuPerfCounter perfCounter(scene, "Update");

				// Poll system the events
				glfwPollEvents();

				// Update the scene
				updateScene(scene);
			}

			// Render the scene
			{
				Debug::DebugRegion region({ "Frame #"s + std::to_string(simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId), "Render" });
				Profiler::ScopedGpuPerfCounter perfCounter(scene, "Render");

				// Render our objects
				renderScene(scene);

				// Display the contents
				glfwSwapBuffers(scene.m_context.m_window);
			}

			// Store the last update time
			simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_lastUpdateTime = currentTime;

			// Increment the frame number
			++simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_frameId;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		// Active scene
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"object_group", "Scene",
			"Which object groups to enable on startup.",
			"NAME", { "Scene_Sponza", "Aberration_TiledSplat" }, {},
			Config::attribRegexString()
		});

		// Active camera
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"camera", "Scene",
			"Which camera to enable on startup.",
			"NAME", { "SponzaCameraFree" }, {},
			Config::attribRegexString()
		});
	};
}