#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
/// KEY-FRAMED ANIMATION FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace KeyframedAnim
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "KeyframedAnimation";
	static constexpr const char* DISPLAY_NAME = "Keyframed Animation";
	static constexpr const char* CATEGORY = "Animation";

	////////////////////////////////////////////////////////////////////////////////
	/** A a key-framed animation track node. */
	struct KeyFramedAnimNode
	{
		// Time of the key
		float m_time = 0.0f;

		// Value of the key
		float m_value = 0.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** A a key-framed animation track. */
	struct KeyFramedAnimTrack
	{
		// Interpolation method
		meta_enum(InterpolationMethod, int, Linear, Cubic);

		// Interpolation method
		InterpolationMethod m_interpolationMethod = Linear;

		// The values to modify
		std::unordered_map<std::string, float*> m_variables;

		// The timeline nodes
		std::vector<KeyFramedAnimNode> m_frames;

		// ---- Private members

		// Set of all the times and values
		std::vector<double> m_times;
		std::vector<double> m_values;

		// Minimum and maximum values along the track
		float m_min = FLT_MAX;
		float m_max = FLT_MAX;

		// Interpolation objects
		gsl_interp* m_interp = nullptr;
		gsl_interp_accel* m_interpAccel = nullptr;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** A key-framed animation component. */
	struct KeyFramedAnimComponent
	{
		// Play direction enums.
		meta_enum(PlayDirection, int, Forward, Reverse, PingPong);

		// Play state enums.
		meta_enum(PlayState, int, Playing, Paused, Stopped);

		// Playback type
		meta_enum(PlayType, int, RealTime, Synced);

		// Play direction.
		PlayDirection m_playDirection = Forward;

		// Playback speed
		float m_playbackSpeed = 1.0f;

		// Playback start
		float m_playbackStart = 0.0f;

		// Playback type
		PlayType m_playbackType = RealTime;

		// Whether the animation is playing or not.
		PlayState m_playState = Stopped;

		// Whether we should record the playback (to video) or not
		bool m_recordPlayback = false;

		// Whether the anim should be looping or not.
		bool m_looping = false;

		// Length of the animation
		float m_length = 0.0f;

		// Where to store the anim data
		std::string m_animFileName = "";

		// The various tracks of the animation.
		std::unordered_map<std::string, KeyFramedAnimTrack> m_tracks;

		struct RecordSettings
		{
			// How often do we record frames
			float m_timePerFrame = 1.0f;

			// Threshold between collapsing linear fields
			float m_linearThreshold = 0.1f;

			// Extra time before the start
			float m_startPadding = 1.0f;

			// Extra time at the end
			float m_endPadding = 1.0f;
		} m_record;

		// Whether we should modify the variables while stopped or not
		bool m_generateWhileStopped = true;

		// ---- Private members

		// Playback vars
		float m_currentTime = 0.0f;
		PlayDirection m_currentPlayDirection = Forward;
		bool m_currentPlayRecorded = false;

		// Recording variables
		bool m_isRecording = false;
		float m_recordStartTime = 0.0f;
		float m_timeSinceLastFrame = 0.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object);

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void play(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void play(Scene::Scene& scene, Scene::Object* object, KeyFramedAnimComponent::PlayDirection direction, float time = -1.0f, float speed = -1.0f);

	////////////////////////////////////////////////////////////////////////////////
	void playFromStart(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void playFromEnd(Scene::Scene & scene, Scene::Object * object);

	////////////////////////////////////////////////////////////////////////////////
	void playForward(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void playReverse(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void playPingPong(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void pause(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void stop(Scene::Scene& scene, Scene::Object* object);

	////////////////////////////////////////////////////////////////////////////////
	void updateTrackInterpObjects(Scene::Scene& scene, Scene::Object* object, KeyFramedAnimTrack& track);

	////////////////////////////////////////////////////////////////////////////////
	void updateTrackInterpObjects(Scene::Scene& scene, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(KEYFRAMED_ANIM, KeyFramedAnimComponent, KeyframedAnim::KeyFramedAnimComponent)
DECLARE_OBJECT(KEYFRAMED_ANIM, COMPONENT_ID_KEYFRAMED_ANIM, COMPONENT_ID_EDITOR_SETTINGS)