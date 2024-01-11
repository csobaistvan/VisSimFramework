#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "Transform.h"

////////////////////////////////////////////////////////////////////////////////
/// CAMERA FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace Camera
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "Camera";
	static constexpr const char* DISPLAY_NAME = "Camera";
	static constexpr const char* CATEGORY = "Actor";

	////////////////////////////////////////////////////////////////////////////////
	/** A camera component. */
	struct CameraComponent
	{
		// Various aperture determination methods
		meta_enum(ApertureMethod, int, Fixed, Physical);

		// Field of view related vars (limits in degrees, current in radians)
		float m_fovyMin = 50.0f;
		float m_fovyMax = 90.0f;
		float m_fovy = glm::radians(60.0f);

		// Near clipping plane distance.
		float m_near = 0.0f;

		// Far clipping plane distance.
		float m_far = 1.0f;

		// Movement speed (in units per second).
		float m_movementSpeed = 800.0f;

		// Multiplier for strolling
		float m_strollMultiplier = 0.125f;

		// Multiplier for sprinting
		float m_sprintMultiplier = 3.0f;

		// Mouse turn speed.
		float m_mouseTurnSpeed = glm::radians(5.0f);

		// Keyboard turn speed.
		float m_keyTurnSpeed = glm::radians(60.0f);

		// Aperture determination method
		ApertureMethod m_apertureMethod;

		// Aperture size-related vars(in millimeters).
		float m_apertureMin = 3.0f;
		float m_apertureMax = 9.0f;
		float m_fixedAperture;

		// Physical aperture noise parameters
		int m_apertureNoiseOctaves = 3;
		float m_apertureNoiseAmplitude = 0.0f;
		float m_apertureNoisePersistance = 0.5f;
		float m_apertureAdaptationRate = 100.0f;

		// Focus distance-realted vars (in meters)
		float m_focusDistanceMin = 0.1;
		float m_focusDistanceMax = 10.0f;
		float m_focusDistance = 10.0f;

		// Whether the camera is fixed or not
		bool m_locked = false;

		// ---- Private members

		// Previous aperture and focus settings
		float m_prevAperture = 0.0f;
		float m_prevFocusDistance = 0.0f;

		// Whether the camera changed since the previous frame or not.
		bool m_settingsChanged = false;

		// Variable to manage holding the camera settings
		int m_settingsLocked = 0;

		// The view frustum defined by this camera.
		BVH::Frustum m_viewFrustum;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Uniform buffer for the camera data. */
	struct UniformData
	{
		// Camera view matrix.
		glm::mat4 m_view;

		// Camera projection matrix.
		glm::mat4 m_projection;

		// Camera view-projection matrix.
		glm::mat4 m_viewProjection;

		// Inverse view matrix
		glm::mat4 m_inverseView;

		// Inverse projection matrix.
		glm::mat4 m_inverseProjection;

		// Inverse view-projection matrix.
		glm::mat4 m_inverseViewProjection;

		// Previous camera view matrix.
		glm::mat4 m_prevView;

		// Previous camera projection matrix.
		glm::mat4 m_prevProjection;

		// Previous camera projection matrix.
		glm::mat4 m_prevViewProjection;

		// Previous inverse view matrix
		glm::mat4 m_prevInverseView;

		// Previous inverse projection matrix.
		glm::mat4 m_prevInverseProjection;

		// Previous inverse projection matrix.
		glm::mat4 m_prevInverseViewProjection;

		// Camera position.
		alignas(sizeof(glm::vec4)) glm::vec3 m_eye;

		// Camera direction.
		alignas(sizeof(glm::vec4)) glm::vec3 m_eyeDir;

		// Field of view values.
		alignas(sizeof(glm::vec2)) glm::vec2 m_fov;
		alignas(sizeof(glm::vec2)) glm::vec2 m_fovDegs;

		// Near and far clip planes
		GLfloat m_near;
		GLfloat m_far;

		// Aspect ratio.
		GLfloat m_aspectRatio;

		// Aperture parameters
		GLuint m_apertureMethod;
		GLfloat m_apertureMin;
		GLfloat m_apertureMax;
		GLfloat m_apertureSize;
		GLfloat m_aperturePrevSize;
		GLint m_apertureNoiseOctaves;
		GLfloat m_apertureNoiseAmplitude;
		GLfloat m_apertureNoisePersistance;
		GLfloat m_apertureAdaptationRate;

		// Focus distance
		GLfloat m_focusDistanceMin;
		GLfloat m_focusDistanceMax;
		GLfloat m_focusDistance;
		GLfloat m_focusDistancePrev;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void handleInput(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* input, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);
	
	////////////////////////////////////////////////////////////////////////////////
	bool renderConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToClipSpace(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToSphericalCoordinates(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToSphericalCoordinatesDegM(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToCameraSpace(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToCameraSpaceMeters(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToWorldSpace(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToWorldSpaceMeters(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth);

	////////////////////////////////////////////////////////////////////////////////
	float getPlaneSizePinHole(float fovy, float depth);

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth should be in meters. */
	float getPlaneSizePinHole(Scene::Object* camera, float depth);

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth should be in meters. */
	float getPlaneSizeThinLens(float apertureDiameter, float fovy, float depth);

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth should be in meters. */
	float getPlaneSizeThinLens(Scene::Object* camera, float depth);

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth should be in meters. */
	float getPixelSizePinHole(float fovy, float height, float depth);

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth should be in meters. */
	float getPixelSizePinHole(Scene::Object* renderSettings, Scene::Object* camera, float depth);

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth and aperture size should be in meters. */
	float getPixelSizeThinLens(float apertureDiameter, float fovy, float height, float depth);

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth should be in meters. */
	float getPixelSizeThinLens(Scene::Object* renderSettings, Scene::Object* camera, float depth);

	////////////////////////////////////////////////////////////////////////////////
	float getDepthMeters(Scene::Object* renderSettings, Scene::Object* camera, glm::mat4 const& projection, float depth);

	////////////////////////////////////////////////////////////////////////////////
	float getDepthMeters(Scene::Object* renderSettings, Scene::Object* camera, float depth);

	////////////////////////////////////////////////////////////////////////////////
	float getDepthLinear(Scene::Object* renderSettings, Scene::Object* camera, glm::mat4 const& projection, float meters);

	////////////////////////////////////////////////////////////////////////////////
	float getDepthLinear(Scene::Object* renderSettings, Scene::Object* camera, float meters);

	////////////////////////////////////////////////////////////////////////////////
	float getAspectRatio(glm::ivec2 resolution);

	////////////////////////////////////////////////////////////////////////////////
	float getAspectRatio(Scene::Object* renderSettings, Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec2 getFieldOfView(glm::ivec2 resolution, float fovy);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec2 getFieldOfView(Scene::Object* renderSettings, Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	std::array<float, 2> getFieldOfViewLimits(Scene::Object* renderSettings, Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getForwardVector(Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getRightVector(Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getUpVector(Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getPrevForwardVector(Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getPrevRightVector(Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getPrevUpVector(Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getViewMatrix(Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getProjectionMatrix(Scene::Object* renderSettings, Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getViewProjectionMatrix(Scene::Object* renderSettings, Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getPrevViewMatrix(Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getPrevProjectionMatrix(Scene::Object* renderSettings, Scene::Object* camera);

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getPrevViewProjectionMatrix(Scene::Object* renderSettings, Scene::Object* camera);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(CAMERA, CameraComponent, Camera::CameraComponent)
DECLARE_OBJECT(CAMERA, COMPONENT_ID_TRANSFORM, COMPONENT_ID_CAMERA, COMPONENT_ID_EDITOR_SETTINGS)