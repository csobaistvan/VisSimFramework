#include "PCH.h"
#include "Camera.h"
#include "Transform.h"

namespace Camera
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(CAMERA);
	DEFINE_OBJECT(CAMERA);
	REGISTER_OBJECT_UPDATE_CALLBACK(CAMERA, AFTER, INPUT);
	REGISTER_OBJECT_RENDER_CALLBACK(CAMERA, "Camera Uniforms", OpenGL, AFTER, "Uniforms [Begin]", 1, &Camera::renderObjectOpenGL, &RenderSettings::firstCallTypeCondition, &Camera::renderConditionOpenGL, nullptr, nullptr);

	////////////////////////////////////////////////////////////////////////////////
	void initShaders(Scene::Scene& scene, Scene::Object* = nullptr)
	{
		// Shader loading parameters
		Asset::ShaderParameters shaderParameters;
		shaderParameters.m_enums = Asset::generateMetaEnumDefines
		(
			Camera::CameraComponent::ApertureMethod_meta
		);

		// Load the aperture size shader
		Asset::loadShader(scene, "Camera", "aperture_size", "", shaderParameters);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		Scene::appendResourceInitializer(scene, object.m_name, Scene::Shader, initShaders, "Shaders");

		object.component<Camera::CameraComponent>().m_settingsChanged = true;
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void handleInput(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* input, Scene::Object* object)
	{
		// Extract the render settings object
		Scene::Object* renderSettings = findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		// Whether the camera is locked or not
		bool locked =
			object->component<CameraComponent>().m_locked || 
			object->component<CameraComponent>().m_settingsLocked > 0 ||
			input->component<InputSettings::InputComponent>().m_input == 0 || 
			RenderSettings::getMainCamera(scene, renderSettings) != object;

		// Simply zero out the delta if the camera is locked
		float delta = (locked == false) ? simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_deltaTime : 0.0f;

		// Delta values
		float moveDelta = RenderSettings::metersToUnits(scene, renderSettings, object->component<Camera::CameraComponent>().m_movementSpeed * delta);
		float mouseTurnDelta = object->component<Camera::CameraComponent>().m_mouseTurnSpeed * delta;
		float keyTurnDelta = object->component<Camera::CameraComponent>().m_keyTurnSpeed * delta;

		// Strolling
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_LEFT_CONTROL] > 0)
		{
			moveDelta *= object->component<Camera::CameraComponent>().m_strollMultiplier;
			mouseTurnDelta *= object->component<Camera::CameraComponent>().m_strollMultiplier;
			keyTurnDelta *= object->component<Camera::CameraComponent>().m_strollMultiplier;
		}

		// Sprinting
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_LEFT_SHIFT] > 0)
		{
			moveDelta *= object->component<Camera::CameraComponent>().m_sprintMultiplier;
			mouseTurnDelta *= object->component<Camera::CameraComponent>().m_sprintMultiplier;
			keyTurnDelta *= object->component<Camera::CameraComponent>().m_sprintMultiplier;
		}

		// Move forward
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_W] > 0)
		{
			object->component<Transform::TransformComponent>().m_position += Camera::getForwardVector(object) * moveDelta;
		}

		// Move backward
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_S] > 0)
		{
			object->component<Transform::TransformComponent>().m_position -= Camera::getForwardVector(object) * moveDelta;
		}

		// Move left
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_A] > 0)
		{
			object->component<Transform::TransformComponent>().m_position -= Camera::getRightVector(object) * moveDelta;
		}

		// Move right
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_D] > 0)
		{
			object->component<Transform::TransformComponent>().m_position += Camera::getRightVector(object) * moveDelta;
		}

		// Move down
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_Q] > 0)
		{
			object->component<Transform::TransformComponent>().m_position -= Camera::getUpVector(object) * moveDelta;
		}

		// Move up
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_E] > 0)
		{
			object->component<Transform::TransformComponent>().m_position += Camera::getUpVector(object) * moveDelta;
		}

		// Turn the camera
		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_LEFT] > 0)
		{
			object->component<Transform::TransformComponent>().m_orientation.y += keyTurnDelta;
		}

		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_RIGHT] > 0)
		{
			object->component<Transform::TransformComponent>().m_orientation.y -= keyTurnDelta;
		}

		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_UP] > 0)
		{
			object->component<Transform::TransformComponent>().m_orientation.x += keyTurnDelta;
		}

		if (input->component<InputSettings::InputComponent>().m_keys[GLFW_KEY_DOWN] > 0)
		{
			object->component<Transform::TransformComponent>().m_orientation.x -= keyTurnDelta;
		}

		// Calculate the delta
		glm::vec2 deltaCoordinates = (input->component<InputSettings::InputComponent>().m_currentMouseCoordinates - input->component<InputSettings::InputComponent>().m_prevMouseCoordinates);

		// Modify the camera orientation
		object->component<Transform::TransformComponent>().m_orientation.x -= deltaCoordinates.y * mouseTurnDelta;
		object->component<Transform::TransformComponent>().m_orientation.y -= deltaCoordinates.x * mouseTurnDelta;
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		// Access the render settings object
		Scene::Object* renderSettings = findFirstObject(scene, Scene::OBJECT_TYPE_RENDER_SETTINGS);

		// Reset the camera dirty flag
		object->component<Camera::CameraComponent>().m_settingsChanged = false;

		// Recompute the view frustum
		object->component<Camera::CameraComponent>().m_viewFrustum = BVH::Frustum(Camera::getViewProjectionMatrix(renderSettings, object));

		// Store the previous values
		object->component<Camera::CameraComponent>().m_prevAperture = object->component<Camera::CameraComponent>().m_fixedAperture;
		object->component<Camera::CameraComponent>().m_prevFocusDistance = object->component<Camera::CameraComponent>().m_focusDistance;
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		Transform::generateGui(scene, guiSettings, object);

		ImGui::Separator();

		bool cameraChanged = false;
		cameraChanged |= ImGui::DragFloat("Near Plane", &object->component<Camera::CameraComponent>().m_near, 0.001f, 0.00001f, 0.0f, "%.3f m");
		cameraChanged |= ImGui::DragFloat("Far Plane", &object->component<Camera::CameraComponent>().m_far, 0.001f, 0.000001f, 0.0f, "%.3f m");
		cameraChanged |= ImGui::Combo("Aperture Method", &object->component<Camera::CameraComponent>().m_apertureMethod, Camera::CameraComponent::ApertureMethod_meta);
		ImGui::SliderInt("Aperture Noise Octaves", &object->component<Camera::CameraComponent>().m_apertureNoiseOctaves, 1, 32);
		ImGui::SliderFloat("Aperture Noise Amplitude", &object->component<Camera::CameraComponent>().m_apertureNoiseAmplitude, 0.0f, 8.0f);
		ImGui::SliderFloat("Aperture Noise Persistance", &object->component<Camera::CameraComponent>().m_apertureNoisePersistance, 0.0f, 1.0f);
		ImGui::SliderFloat("Aperture Adaptation Rate", &object->component<Camera::CameraComponent>().m_apertureAdaptationRate, 0.0f, 100.0f);
		cameraChanged |= ImGui::DragFloatRange2("Fovy Limits", &object->component<Camera::CameraComponent>().m_fovyMin, &object->component<Camera::CameraComponent>().m_fovyMax, 0.01f, 40.0f, 120.0f, "%.3f deg", "%.3f deg");
		cameraChanged |= ImGui::DragFloatRange2("Aperture Limits", &object->component<Camera::CameraComponent>().m_apertureMin, &object->component<Camera::CameraComponent>().m_apertureMax, 0.01f, 0.0f, 10.0f, "%.3f mm", "%.3f mm");
		cameraChanged |= ImGui::DragFloatRange2("Focus Distance Limits", &object->component<Camera::CameraComponent>().m_focusDistanceMin, &object->component<Camera::CameraComponent>().m_focusDistanceMax, 0.01f, 0.0f, 10.0f, "%.3f m", "%.3f m");
		cameraChanged |= ImGui::SliderAngle("Field of View", &object->component<Camera::CameraComponent>().m_fovy, object->component<Camera::CameraComponent>().m_fovyMin, object->component<Camera::CameraComponent>().m_fovyMax);
		cameraChanged |= ImGui::SliderFloat("Fixed Aperture", &object->component<Camera::CameraComponent>().m_fixedAperture, object->component<Camera::CameraComponent>().m_apertureMin, object->component<Camera::CameraComponent>().m_apertureMax);
		cameraChanged |= ImGui::SliderFloat("Focus Distance", &object->component<Camera::CameraComponent>().m_focusDistance, object->component<Camera::CameraComponent>().m_focusDistanceMin, object->component<Camera::CameraComponent>().m_focusDistanceMax);
		
		ImGui::Separator();

		ImGui::SliderAngle("Mouse Turn Speed", &object->component<Camera::CameraComponent>().m_mouseTurnSpeed, 1.0f, 10.0f, "%.3f deg/s");
		ImGui::SliderAngle("Key Turn Speed", &object->component<Camera::CameraComponent>().m_keyTurnSpeed, 1.0f, 90.0f, "%.3f deg/s");
		ImGui::SliderFloat("Move Speed", &object->component<Camera::CameraComponent>().m_movementSpeed, 0.01f, 10.0f, "%.3f m/s");
		ImGui::SliderFloat("Stroll Multiplier", &object->component<Camera::CameraComponent>().m_strollMultiplier, 0.0f, 1.0f);
		ImGui::SliderFloat("Sprint Multiplier", &object->component<Camera::CameraComponent>().m_sprintMultiplier, 1.0f, 8.0f);
		ImGui::Checkbox("Locked", &object->component<Camera::CameraComponent>().m_locked);

		Debug::log_debug() << "Camera changed: " << cameraChanged << Debug::end;

		object->component<Camera::CameraComponent>().m_settingsChanged = cameraChanged;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool renderConditionOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		return RenderSettings::firstCallObjectCondition(scene, simulationSettings, renderSettings, camera, functionName, object) && object == camera;
	}

	////////////////////////////////////////////////////////////////////////////////
	void renderObjectOpenGL(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* renderSettings, Scene::Object* camera, std::string const& functionName, Scene::Object* object)
	{
		// Upload the camera uniforms
		Camera::UniformData cameraData;
		cameraData.m_view = Camera::getViewMatrix(object);
		cameraData.m_projection = Camera::getProjectionMatrix(renderSettings, object);
		cameraData.m_viewProjection = cameraData.m_projection * cameraData.m_view;
		cameraData.m_inverseView = glm::inverse(cameraData.m_view);
		cameraData.m_inverseProjection = glm::inverse(cameraData.m_projection);
		cameraData.m_inverseViewProjection = glm::inverse(cameraData.m_viewProjection);
		cameraData.m_prevView = Camera::getPrevViewMatrix(object);
		cameraData.m_prevProjection = Camera::getPrevProjectionMatrix(renderSettings, object);
		cameraData.m_prevViewProjection = cameraData.m_prevProjection * cameraData.m_prevView;
		cameraData.m_prevInverseView = glm::inverse(cameraData.m_prevView);
		cameraData.m_prevInverseProjection = glm::inverse(cameraData.m_prevProjection);
		cameraData.m_prevInverseViewProjection = glm::inverse(cameraData.m_prevViewProjection);
		cameraData.m_near = RenderSettings::metersToUnits(renderSettings, object->component<Camera::CameraComponent>().m_near);
		cameraData.m_far = RenderSettings::metersToUnits(renderSettings, object->component<Camera::CameraComponent>().m_far);
		cameraData.m_apertureMethod = object->component<Camera::CameraComponent>().m_apertureMethod;
		cameraData.m_apertureMin = object->component<Camera::CameraComponent>().m_apertureMin;
		cameraData.m_apertureMax = object->component<Camera::CameraComponent>().m_apertureMax;
		cameraData.m_apertureSize = object->component<Camera::CameraComponent>().m_fixedAperture;
		cameraData.m_aperturePrevSize = object->component<Camera::CameraComponent>().m_prevAperture;
		cameraData.m_apertureNoiseOctaves = object->component<Camera::CameraComponent>().m_apertureNoiseOctaves;
		cameraData.m_apertureAdaptationRate = object->component<Camera::CameraComponent>().m_apertureAdaptationRate;
		cameraData.m_apertureNoiseAmplitude = object->component<Camera::CameraComponent>().m_apertureNoiseAmplitude;
		cameraData.m_apertureNoisePersistance = object->component<Camera::CameraComponent>().m_apertureNoisePersistance;
		cameraData.m_focusDistanceMin = object->component<Camera::CameraComponent>().m_focusDistanceMin;
		cameraData.m_focusDistanceMax = object->component<Camera::CameraComponent>().m_focusDistanceMax;
		cameraData.m_focusDistance = object->component<Camera::CameraComponent>().m_focusDistance;
		cameraData.m_focusDistancePrev = object->component<Camera::CameraComponent>().m_prevFocusDistance;
		cameraData.m_aspectRatio = Camera::getAspectRatio(renderSettings, object);
		cameraData.m_eye = object->component<Transform::TransformComponent>().m_position;
		cameraData.m_eyeDir = Camera::getForwardVector(object);
		cameraData.m_fov = Camera::getFieldOfView(renderSettings, object);
		cameraData.m_fovDegs = glm::degrees(cameraData.m_fov);
		uploadBufferData(scene, "Camera", cameraData);

		// Compute the current aperture size
		{
			Profiler::ScopedGpuPerfCounter perfCounter(scene, "Aperture Size");

			// Bind the previous luminance texture
			glActiveTexture(GPU::TextureEnums::TEXTURE_POST_PROCESS_1_ENUM);
			glBindTexture(GL_TEXTURE_2D, scene.m_textures["ToneMap_Luminance_" + std::to_string(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_gbufferRead)].m_texture);

			// Bind the corresponding shader
			Scene::bindShader(scene, "Camera", "aperture_size");

			// Dispatch the compute shader
			glDispatchCompute(1, 1, 1);

			// Place a memory barrier for the SSBOs
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


			if (renderSettings->component<RenderSettings::RenderSettingsComponent>().m_features.m_showApertureSize)
			{
				glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene.m_genericBuffers["Camera"].m_buffer);
				Camera::UniformData* data = (Camera::UniformData*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
				Profiler::storeData(scene, "Aperture Size", (float)data->m_apertureSize);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToClipSpace(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth)
	{
		const glm::ivec2 resolution = renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution;
		const glm::vec3 screenSpaceCoords = glm::vec3(glm::vec2(screenCoords) / glm::vec2(resolution - 1), depth);
		return screenSpaceCoords * 2.0f - 1.0f;
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToSphericalCoordinates(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth)
	{
		const glm::vec3 camPos = screenToCameraSpace(renderSettings, camera, screenCoords, depth);
		const glm::vec2 anglesRad = glm::vec2(glm::atan(camPos.x, -camPos.z), glm::atan(camPos.y, glm::length(glm::vec2(camPos.z, camPos.x))));
		const float radius = glm::length(camPos);
		return glm::vec3(anglesRad, radius);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToSphericalCoordinatesDegM(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth)
	{
		const glm::vec3 sphericalCoords = screenToSphericalCoordinates(renderSettings, camera, screenCoords, depth);
		return glm::vec3(glm::degrees(glm::vec2(sphericalCoords.x, sphericalCoords.y)), RenderSettings::unitsToMeters(renderSettings, sphericalCoords.z));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToCameraSpace(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth)
	{
		const glm::vec4 clipSpaceCoords = glm::vec4(screenToClipSpace(renderSettings, camera, screenCoords, depth), 1.0f);
		const glm::mat4 inverseProjection = glm::inverse(getProjectionMatrix(renderSettings, camera));
		const glm::vec4 camSpaceCoords = inverseProjection * clipSpaceCoords;
		return glm::vec3(camSpaceCoords) / camSpaceCoords.w;
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToCameraSpaceMeters(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth)
	{
		return RenderSettings::unitsToMeters(renderSettings, screenToCameraSpace(renderSettings, camera, screenCoords, depth));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToWorldSpace(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth)
	{
		const glm::vec4 clipSpaceCoords = glm::vec4(screenToClipSpace(renderSettings, camera, screenCoords, depth), 1.0f);
		const glm::mat4 inverseViewProjection = glm::inverse(getViewProjectionMatrix(renderSettings, camera));
		const glm::vec4 worldSpaceCoords = inverseViewProjection * clipSpaceCoords;
		return glm::vec3(worldSpaceCoords) / worldSpaceCoords.w;
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 screenToWorldSpaceMeters(Scene::Object* renderSettings, Scene::Object* camera, glm::ivec2 screenCoords, float depth)
	{
		return RenderSettings::unitsToMeters(renderSettings, screenToWorldSpace(renderSettings, camera, screenCoords, depth));
	}

	////////////////////////////////////////////////////////////////////////////////
	float getPlaneSizePinHole(float fovy, float depth)
	{
		return 2.0f * (glm::tan(fovy * 0.5f) * depth);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth should be in meters. */
	float getPlaneSizePinHole(Scene::Object* camera, float depth)
	{
		return getPlaneSizePinHole(
			camera->component<Camera::CameraComponent>().m_fovy, 
			depth);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth should be in meters. */
	float getPlaneSizeThinLens(float apertureDiameter, float fovy, float depth)
	{
		return 2.0f * (glm::tan(fovy * 0.5f) * depth + apertureDiameter * 0.5f);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Returns the size of a pixel (in meters) at the parameter object depth. Depth should be in meters. */
	float getPlaneSizeThinLens(Scene::Object* camera, float depth)
	{
		return getPlaneSizeThinLens(
			camera->component<Camera::CameraComponent>().m_fixedAperture * 1e-3f,
			camera->component<Camera::CameraComponent>().m_fovy,
			depth);
	}

	////////////////////////////////////////////////////////////////////////////////
	float getPixelSizePinHole(float fovy, float height, float depth)
	{
		return getPlaneSizePinHole(fovy, depth) / height;
	}

	////////////////////////////////////////////////////////////////////////////////
	float getPixelSizePinHole(Scene::Object* renderSettings, Scene::Object* camera, float depth)
	{
		return getPixelSizePinHole(
			camera->component<Camera::CameraComponent>().m_fovy, 
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution.y, 
			depth);
	}

	////////////////////////////////////////////////////////////////////////////////
	float getPixelSizeThinLens(float apertureDiameter, float fovy, float height, float depth)
	{
		return getPlaneSizeThinLens(apertureDiameter, fovy, depth) / height;
	}

	////////////////////////////////////////////////////////////////////////////////
	float getPixelSizeThinLens(Scene::Object* renderSettings, Scene::Object* camera, float depth)
	{
		return getPixelSizeThinLens(
			camera->component<Camera::CameraComponent>().m_fixedAperture * 1e-3f,
			camera->component<Camera::CameraComponent>().m_fovy,
			renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution.y,
			depth);
	}

	////////////////////////////////////////////////////////////////////////////////
	float getDepthMeters(Scene::Object* renderSettings, Scene::Object* camera, glm::mat4 const& projection, float depth)
	{
		return RenderSettings::unitsToMeters(renderSettings, -(projection[3][2] / (depth * -2.0f + 1.0f - projection[2][2])));
	}

	////////////////////////////////////////////////////////////////////////////////
	float getDepthMeters(Scene::Object* renderSettings, Scene::Object* camera, float depth)
	{
		return getDepthMeters(renderSettings, camera, getProjectionMatrix(renderSettings, camera), depth);
	}

	////////////////////////////////////////////////////////////////////////////////
	float getDepthLinear(Scene::Object* renderSettings, Scene::Object* camera, glm::mat4 const& projection, float meters)
	{
		return (meters - camera->component<CameraComponent>().m_near) /
			(camera->component<CameraComponent>().m_far - camera->component<CameraComponent>().m_near);
	}

	////////////////////////////////////////////////////////////////////////////////
	float getDepthLinear(Scene::Object* renderSettings, Scene::Object* camera, float meters)
	{
		return getDepthLinear(renderSettings, camera, getProjectionMatrix(renderSettings, camera), meters);
	}

	////////////////////////////////////////////////////////////////////////////////
	float getAspectRatio(glm::ivec2 resolution)
	{
		return ((float)resolution.x) / ((float)resolution.y);
	}

	////////////////////////////////////////////////////////////////////////////////
	float getAspectRatio(Scene::Object* renderSettings, Scene::Object* camera)
	{
		return getAspectRatio(renderSettings->component<RenderSettings::RenderSettingsComponent>().m_resolution);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec2 getFieldOfView(float aspectRatio, float fovy)
	{
		//return glm::vec2(2.0f * glm::atan(aspectRatio * glm::tan(fovy / 2.0f)), fovy);
		return glm::vec2(aspectRatio * fovy, fovy);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec2 getFieldOfView(glm::ivec2 resolution, float fovy)
	{
		return getFieldOfView(getAspectRatio(resolution), fovy);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec2 getFieldOfView(Scene::Object* renderSettings, Scene::Object* camera)
	{
		return getFieldOfView(getAspectRatio(renderSettings, camera), camera->component<Camera::CameraComponent>().m_fovy);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::array<float, 2> getFieldOfViewLimits(Scene::Object* renderSettings, Scene::Object* camera)
	{
		return std::array<float, 2>
		{
			glm::radians(camera->component<Camera::CameraComponent>().m_fovyMin),
			glm::radians(camera->component<Camera::CameraComponent>().m_fovyMax)
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getForwardVector(Scene::Object* camera)
	{
		return Transform::getForwardVector(camera);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getRightVector(Scene::Object* camera)
	{
		return Transform::getRightVector(camera);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getUpVector(Scene::Object* camera)
	{
		return Transform::getUpVector(camera);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getPrevForwardVector(Scene::Object* camera)
	{
		return Transform::getPrevForwardVector(camera);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getPrevRightVector(Scene::Object* camera)
	{
		return Transform::getPrevRightVector(camera);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::vec3 getPrevUpVector(Scene::Object* camera)
	{
		return Transform::getPrevUpVector(camera);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getViewMatrix(Scene::Object* camera)
	{
		return glm::inverse(Transform::getModelMatrix(camera));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getProjectionMatrix(Scene::Object* renderSettings, Scene::Object* camera)
	{
		return glm::perspective(camera->component<Camera::CameraComponent>().m_fovy, 
			getAspectRatio(renderSettings, camera), 
			RenderSettings::metersToUnits(renderSettings, camera->component<Camera::CameraComponent>().m_near), 
			RenderSettings::metersToUnits(renderSettings, camera->component<Camera::CameraComponent>().m_far));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getViewProjectionMatrix(Scene::Object* renderSettings, Scene::Object* camera)
	{
		return getProjectionMatrix(renderSettings, camera) * getViewMatrix(camera);
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getPrevViewMatrix(Scene::Object* camera)
	{
		return glm::inverse(Transform::getPrevModelMatrix(camera));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getPrevProjectionMatrix(Scene::Object* renderSettings, Scene::Object* camera)
	{
		return glm::perspective(camera->component<Camera::CameraComponent>().m_fovy, 
			getAspectRatio(renderSettings, camera), 
			RenderSettings::metersToUnits(renderSettings, camera->component<Camera::CameraComponent>().m_near),
			RenderSettings::metersToUnits(renderSettings, camera->component<Camera::CameraComponent>().m_far));
	}

	////////////////////////////////////////////////////////////////////////////////
	glm::mat4 getPrevViewProjectionMatrix(Scene::Object* renderSettings, Scene::Object* camera)
	{
		return getPrevProjectionMatrix(renderSettings, camera) * getPrevViewMatrix(camera);
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		// @CONSOLE_VAR(Camera, Field of View, -fov, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"fov", "Camera",
			"Field-of-view of the main camera.",
			"NAME", { "60.0" }, {},
			Config::attribRegexInt()
		});

		// @CONSOLE_VAR(Camera, Aperture Diameter, -aperture, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"aperture", "Camera",
			"Aperture diameter of the main camera.",
			"NAME", { "5.0" }, {},
			Config::attribRegexFloat()
		});

		// @CONSOLE_VAR(Camera, Focus Distance, -focus, 8, 4, 2, 1, 0.5, 0.25, 0.1)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"focus", "Camera",
			"Focus distance of the main camera.",
			"NAME", { "8.0" }, {},
			Config::attribRegexFloat()
		});
	};
}