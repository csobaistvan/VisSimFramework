#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "Profiler.h"
#include "Asset.h"

////////////////////////////////////////////////////////////////////////////////
/// SCENE STRUCTURES
////////////////////////////////////////////////////////////////////////////////
namespace Scene
{
	////////////////////////////////////////////////////////////////////////////////
	using ComponentId = int;
	using ObjectType = unsigned long long;

	////////////////////////////////////////////////////////////////////////////////
	/** Component class to component id mapping. */
	template<typename T> struct ComponentClassToComponentId {
		static constexpr ComponentId s_componentId = 0;
	};
	template<ComponentId id> struct ComponentIdToComponentClass {
		using type = void;
	};

	////////////////////////////////////////////////////////////////////////////////
	struct TypeErasedComponentAny
	{
		////////////////////////////////////////////////////////////////////////////////
		using Storage = std::any;

		////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		inline static Storage make() { return T(); }

		////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		inline static T& extractRef(Storage& component) { return std::any_cast<T&>(component); }

		////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		inline static T const& extractConstRef(Storage const& component) { return std::any_cast<T const&>(component); }
	};

	////////////////////////////////////////////////////////////////////////////////
	struct TypeErasedComponentUniquePtr
	{
		////////////////////////////////////////////////////////////////////////////////
		using Storage = std::unique_ptr<void, void(*)(void const*)>;

		////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		inline static Storage make()
		{
			return Storage(new T(), [](void const* data)
			{
				delete (static_cast<T const*>(data));
			});
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		inline static T& extractRef(Storage& component) { return *(static_cast<T*>(component.get())); }

		////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		inline static T const& extractConstRef(Storage const& component) { return *(static_cast<T const*>(component.get())); }
	};

	////////////////////////////////////////////////////////////////////////////////
	using TypeErasedComponent = TypeErasedComponentUniquePtr;

	////////////////////////////////////////////////////////////////////////////////
	template<ComponentId id, typename ComponentClass = typename ComponentIdToComponentClass<id>::type>
	void defaultComponentConstructor(Object& object)
	{
		object.m_components.emplace(std::make_pair(id, std::move(TypeErasedComponent::make<ComponentClass>())));
	}

	////////////////////////////////////////////////////////////////////////////////
	/** List of components. */
	using ComponentTypes = std::vector<ComponentId>;
	ComponentTypes& componentTypes();

	////////////////////////////////////////////////////////////////////////////////
	/** List of object types. */
	using ObjectTypes = std::vector<ObjectType>;
	ObjectTypes& objectTypes();

	////////////////////////////////////////////////////////////////////////////////
	/** List of component constructors. */
	using ComponentConstructor = std::function<void(Object& object)>;
	using ComponentConstructors = std::unordered_map<ComponentId, ComponentConstructor>;
	ComponentConstructors& componentConstructors();

	////////////////////////////////////////////////////////////////////////////////
	/** List of object names. */
	using ComponentNames = std::unordered_map<ComponentId, std::string>;
	ComponentNames& componentNames();
	
	////////////////////////////////////////////////////////////////////////////////
	/** List of object names. */
	using ObjectNames = std::unordered_map<ObjectType, std::string>;
	ObjectNames& objectNames();

	////////////////////////////////////////////////////////////////////////////////
	/** List of the default object initializers. */
	using ObjectInitializer = std::function<void(Scene & scene, Object & object)>;
	using ObjectInitializers = std::unordered_map<ObjectType, ObjectInitializer>;
	ObjectInitializers& objectInitializers();

	////////////////////////////////////////////////////////////////////////////////
	/** List of the default object releasers. */
	using ObjectReleaser = std::function<void(Scene & scene, Object & object)>;
	using ObjectReleasers = std::unordered_map<ObjectType, ObjectReleaser>;
	ObjectReleasers& objectReleasers();

	////////////////////////////////////////////////////////////////////////////////
	// Object type input handling functions
	using ObjectInputFunction = std::function<void(Scene& scene, Object* simulationSettings, Object* inputSettings, Object* object)>;
	using ObjectInputFunctions = std::unordered_map<ObjectType, ObjectInputFunction>;
	ObjectInputFunctions& objectInputFunctions();

	////////////////////////////////////////////////////////////////////////////////
	// Object demo setup update functions
	using ObjectDemoSceneSetupFunction = std::function<void(Scene& scene)>;
	using ObjectDemoSceneSetupFunctions = std::unordered_map<ObjectType, ObjectDemoSceneSetupFunction>;
	ObjectDemoSceneSetupFunctions& objectDemoSceneSetupFunctions();

	////////////////////////////////////////////////////////////////////////////////
	// Object type update functions
	struct ObjectUpdateFunction
	{
		using UpdateFunction = std::function<void(Scene& scene, Object* simulationSettings, Object* object)>;

		ObjectType m_objectType;
		UpdateFunction m_updateFunction;
	};
	using ObjectUpdateFunctions = std::vector<ObjectUpdateFunction>;
	ObjectUpdateFunctions& objectUpdateFunctions();

	////////////////////////////////////////////////////////////////////////////////
	// Object type update function registrators
	using ObjectUpdateFunctionRegistrator = std::tuple<int, int, int, ObjectUpdateFunction>; // [reference object id, relative position, priority, update function]
	using ObjectUpdateFunctionRegistrators = std::vector<std::pair<ObjectType, ObjectUpdateFunctionRegistrator>>;
	ObjectUpdateFunctionRegistrators& objectUpdateFunctionRegistrators();

	////////////////////////////////////////////////////////////////////////////////
	/** Object render function params. */
	struct ObjectRenderParameters
	{
		std::string m_functionName;
		Object* m_simulationSettings;
		Object* m_renderSettings;
		Object* m_camera; 
		Object* m_object;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Object render callbacks. */
	struct ObjectRenderFunction
	{
		using RenderFunction = std::function<void(Scene& scene, Object* simulationSettings, Object* renderSettings, Object* camera, std::string const& functionName, Object* object)>;
		using StateFunction = std::function<void(Scene& scene, Object* simulationSettings, Object* renderSettings, Object* camera, std::string const& functionName)>;
		using CallbackCondition = std::function<bool(Scene& scene, Object* simulationSettings, Object* renderSettings, Object* camera, std::string const& functionName)>;
		using ObjectCondition = std::function<bool(Scene& scene, Object* simulationSettings, Object* renderSettings, Object* camera, std::string const& functionName, Object* object)>;

		std::string m_name;
		ObjectType m_objectType;
		RenderFunction m_renderFunction;
		CallbackCondition m_callbackCondition;
		ObjectCondition m_objectCondition;
		StateFunction m_preState;
		StateFunction m_postState;
	};
	using ObjectRenderFunctions = std::vector<ObjectRenderFunction>;
	ObjectRenderFunctions& objectRenderFunctionsOpenGL();

	////////////////////////////////////////////////////////////////////////////////
	// Render function registrators
	using ObjectRenderFunctionRegistrator = std::tuple<std::string, int, int, ObjectRenderFunction>; // [reference pass name, relative position, priority, render function]
	using ObjectRenderFunctionRegistrators = std::vector<std::pair<ObjectType, ObjectRenderFunctionRegistrator>>;
	ObjectRenderFunctionRegistrators& objectRenderFunctionRegistratorsOpenGL();

	////////////////////////////////////////////////////////////////////////////////
	/** List of object GUI categories. */
	using ObjectGuiCategories = std::unordered_map<ObjectType, std::string>;
	ObjectGuiCategories& objectGuiCategories();

	////////////////////////////////////////////////////////////////////////////////
	// Object type gui generators
	using ObjectGuiGenerator = std::function<void(Scene & scene, Object * guiSettings, Object * object)>;
	using ObjectGuiGenerators = std::unordered_map<ObjectType, ObjectGuiGenerator>;
	ObjectGuiGenerators& objectGuiGenerators();

	////////////////////////////////////////////////////////////////////////////////
	/** Declares a new component. */
	#define DECLARE_COMPONENT(NAME, CLASS, FULL_CLASS) \
	namespace Scene \
	{ \
		static const ComponentId CONCAT(COMPONENT_ID_, NAME) = CONCAT(__COMPONENT_ID_, NAME); \
		template<> struct ComponentClassToComponentId<FULL_CLASS> { static constexpr ComponentId s_componentId = CONCAT(COMPONENT_ID_, NAME); }; \
		template<> struct ComponentIdToComponentClass<CONCAT(COMPONENT_ID_, NAME)> { using type = FULL_CLASS; }; \
		inline FULL_CLASS& CLASS(Object& object) { return object.component<FULL_CLASS>(); } \
		inline FULL_CLASS& CLASS(Object* object) { return object->component<FULL_CLASS>(); } \
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Declares a new object type. */
	#define DECLARE_OBJECT(NAME, ...) \
	namespace Scene \
	{ \
		static const ObjectType CONCAT(OBJECT_TYPE_, NAME) = std::bit_mask({ __VA_ARGS__ }); \
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Defines a component. */
	#define DEFINE_COMPONENT(NAME) \
		STATIC_INITIALIZER() \
		{ \
			auto componentId = CONCAT(Scene::COMPONENT_ID_, NAME); \
			Debug::log_debug() << "Registering ComponentType " << COMPONENT_NAME << " with ID: " << componentId << ", mask: " << std::bit_mask(componentId) << Debug::end; \
			Scene::componentTypes().push_back(componentId); \
			Scene::componentNames()[componentId] = COMPONENT_NAME; \
			Scene::componentConstructors()[componentId] = Scene::defaultComponentConstructor<CONCAT(Scene::COMPONENT_ID_, NAME)>; \
		} \

	////////////////////////////////////////////////////////////////////////////////
	/** Defines an object. */
	#define DEFINE_OBJECT(NAME) \
		STATIC_INITIALIZER() \
		{ \
			__if_exists(CONCAT(Scene::COMPONENT_ID_, NAME)) { Scene::componentConstructors()[CONCAT(Scene::COMPONENT_ID_, NAME)] = Scene::defaultComponentConstructor<CONCAT(Scene::COMPONENT_ID_, NAME)>; } \
			auto objectId = CONCAT(Scene::OBJECT_TYPE_, NAME); \
			Debug::log_debug() << "Registering ObjectType " << DISPLAY_NAME << " with mask: " << objectId << Debug::end; \
			Scene::objectTypes().push_back(objectId); \
			Scene::objectNames()[objectId] = DISPLAY_NAME; \
			Scene::objectGuiCategories()[objectId] = CATEGORY; \
			__if_exists(initObject) { Scene::objectInitializers()[objectId] = &initObject; } \
			__if_exists(releaseObject) { Scene::objectReleasers()[objectId] = &releaseObject; } \
			__if_exists(generateGui) { Scene::objectGuiGenerators()[objectId] = &generateGui; } \
			__if_exists(handleInput) { Scene::objectInputFunctions()[objectId] = &handleInput; } \
			__if_exists(demoSetup) { Scene::objectDemoSceneSetupFunctions()[objectId] = &demoSetup; } \
		}

	////////////////////////////////////////////////////////////////////////////////
	/** Registers the demo setup function */
	#define REGISTER_OBJECT_DEMO_SETUP_CALLBACK(NAME, RELATION, REFERENCE) \
		STATIC_INITIALIZER() \
		{ \
			int BEFORE = -1, AFTER = +1; \
			auto objectId = CONCAT(Scene::OBJECT_TYPE_, NAME); \
			int REF_OBJ_ID = 0; \
			__if_exists(CONCAT(Scene::OBJECT_TYPE_, REFERENCE)) { REF_OBJ_ID = CONCAT(Scene::OBJECT_TYPE_, REFERENCE); } \
			Scene::objectDemoSceneSetupFunctionRegistrators()[objectId] = Scene::objectDemoSceneSetupFunctionRegistrator{ REF_OBJ_ID, RELATION, Scene::ObjectDemoSceneSetupFunction{ objectId, &demoSetup } }; \
		}

	////////////////////////////////////////////////////////////////////////////////
	/** Registers the update function for a specific object type. */
	#define REGISTER_OBJECT_UPDATE_CALLBACK(NAME, RELATION, REFERENCE) \
		STATIC_INITIALIZER() \
		{ \
			int BEFORE = -1, AFTER = +1; \
			auto objectId = CONCAT(Scene::OBJECT_TYPE_, NAME); \
			int REF_OBJ_ID = 0; \
			__if_exists(CONCAT(Scene::OBJECT_TYPE_, REFERENCE)) { REF_OBJ_ID = CONCAT(Scene::OBJECT_TYPE_, REFERENCE); } \
			Scene::objectUpdateFunctionRegistrators().push_back({ objectId, Scene::ObjectUpdateFunctionRegistrator{ REF_OBJ_ID, RELATION, 0, Scene::ObjectUpdateFunction{ objectId, &updateObject } } }); \
		}
	
	////////////////////////////////////////////////////////////////////////////////
	/** Registers the render callback */
	#define REGISTER_OBJECT_RENDER_CALLBACK(NAME, PASS_NAME, PIPELINE, RELATION, REFERENCE, PRIORITY, ...) \
		STATIC_INITIALIZER() \
		{ \
			int BEFORE = -1, AFTER = +1; \
			auto objectId = CONCAT(Scene::OBJECT_TYPE_, NAME); \
			Scene::objectRenderFunctionRegistrators##PIPELINE().push_back({ objectId, Scene::ObjectRenderFunctionRegistrator{ std::string(REFERENCE), RELATION, PRIORITY, Scene::ObjectRenderFunction{ PASS_NAME, objectId, __VA_ARGS__ } } }); \
		}

	////////////////////////////////////////////////////////////////////////////////
	/** Represents an object in the scene. */
	struct Object
	{
		// Default ctor
		Object() = default;

		// Move ctor
		Object(Object&& other) = default;
		Object& operator=(Object&& other) = default;

		// Disable the copy ctor
		Object(Object& other) = delete;
		Object& operator=(Object& other) = delete;

		// Owning scene
		Scene* m_owner;

		// Object name.
		std::string m_name;

		// Root name.
		std::string m_alias;

		// Whether the object is enabled or not.
		bool m_enabled = true;

		// Whether the object is alive or not
		bool m_alive = false;

		// The groups in which this object resigns
		unsigned long long m_groups = 0;

		// The components that are attached to this object.
		unsigned long long m_componentList = 0;

		// Type agnostic component storage
		std::unordered_map<ComponentId, TypeErasedComponent::Storage> m_components;

		////////////////////////////////////////////////////////////////////////////////
		// Templated component getters
		////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////
		template<ComponentId id> typename ComponentIdToComponentClass<id>::type& component()
		{
			return TypeErasedComponent::extractRef<typename ComponentIdToComponentClass<id>::type>(m_components.find(id)->second);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<ComponentId id> typename ComponentIdToComponentClass<id>::type const& component() const
		{
			return TypeErasedComponent::extractConstRef<typename ComponentIdToComponentClass<id>::type>(m_components.find(id)->second);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename T> T& component()
		{
			return component<ComponentClassToComponentId<typename std::decay<T>::type>::s_componentId>();
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename T> T const& component() const
		{
			return component<ComponentClassToComponentId<typename std::decay<T>::type>::s_componentId>();
		}

		////////////////////////////////////////////////////////////////////////////////
		template<ComponentId id> bool hasComponent() const
		{
			return m_components.find(id) != m_components.end();
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename T> bool hasComponent() const
		{
			return hasComponent<ComponentClassToComponentId<typename std::decay<T>::type>::s_componentId>();
		}

		////////////////////////////////////////////////////////////////////////////////
		template<ComponentId id> typename ComponentIdToComponentClass<id>::type& addComponent()
		{
			if (auto it = m_components.find(id); it != m_components.end())
				return TypeErasedComponent::extractRef<T>(it->second);

			componentConstructors()[id](*this);
			m_componentList |= std::bit_mask(id);
			return component<id>();
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename T> T const& addComponent()
		{
			return addComponent<ComponentClassToComponentId<typename std::decay<T>::type>::s_componentId>();
		}

		////////////////////////////////////////////////////////////////////////////////
		template<ComponentId id> bool removeComponent()
		{
			if (auto it = m_components.find(id); it != m_components.end())
			{
				m_components.erase(it);
				m_componentList &= (~std::bit_mask(id));
				return true;
			}

			return false;
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename T> bool removeComponent()
		{
			return removeComponent<ComponentClassToComponentId<typename std::decay<T>::type>::s_componentId>();
		}
	};
}