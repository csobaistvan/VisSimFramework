#include "PCH.h"
#include "KeyframedAnim.h"

////////////////////////////////////////////////////////////////////////////////
/// KEY-FRAMED ANIMATION FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace KeyframedAnim
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(KEYFRAMED_ANIM);
	DEFINE_OBJECT(KEYFRAMED_ANIM);
	REGISTER_OBJECT_UPDATE_CALLBACK(KEYFRAMED_ANIM, AFTER, ACTOR);

	////////////////////////////////////////////////////////////////////////////////
	std::string getAnimFilePath(Scene::Scene& scene, Scene::Object* object)
	{
		return "AnimData/" + object->component<KeyFramedAnimComponent>().m_animFileName + ".ini";
	}

	////////////////////////////////////////////////////////////////////////////////
	void loadAnimData(Scene::Scene& scene, Scene::Object* object)
	{
		if (object->component<KeyFramedAnimComponent>().m_animFileName.empty()) return;
		if (Asset::existsFile(EnginePaths::assetsFolder() / getAnimFilePath(scene, object)) == false) return;
		std::ordered_pairs_in_blocks attributes = Asset::loadPairsInBlocks(scene, getAnimFilePath(scene, object)).value();

		// Extract the anim data
		auto& tracks = object->component<KeyFramedAnimComponent>().m_tracks;
		for (auto const& category : attributes)
		{
			if (category.first == "Attributes")
			{
				for (auto const& stateVar : category.second)
				{
					if (stateVar.first == "Length") object->component<KeyFramedAnimComponent>().m_length = std::from_string<float>(stateVar.second);
				}
			}

			if (tracks.find(category.first) != tracks.end())
			{
				tracks[category.first].m_frames.clear();

				for (auto const& stateVar : category.second)
				{
					tracks[category.first].m_frames.push_back({ std::from_string<float>(stateVar.first), std::from_string<float>(stateVar.second) });
				}
			}
		}

		// Update the interpolation objects
		updateTrackInterpObjects(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void saveAnimData(Scene::Scene& scene, Scene::Object* object)
	{
		if (object->component<KeyFramedAnimComponent>().m_animFileName.empty()) return;

		std::ordered_pairs_in_blocks animData;

		// common properties
		animData["Attributes"].push_back({ "Length", std::to_string(object->component<KeyFramedAnimComponent>().m_length) });

		// serialize the frames
		for (auto& track : object->component<KeyFramedAnimComponent>().m_tracks)
		{
			for (auto const& frame : track.second.m_frames)
			{
				animData[track.first].push_back({ std::to_string(frame.m_time), std::to_string(frame.m_value) });
			}
		}

		// save the file
		Asset::savePairsInBlocks(scene, getAnimFilePath(scene, object), animData);
	}

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{
		DelayedJobs::postJob(scene, &object, "Read Anim Data", true, 1, [](Scene::Scene& scene, Scene::Object& object)
		{
			loadAnimData(scene, &object);
		});

		DelayedJobs::postJob(scene, &object, "Update Interpolation Objects", true, 1, [](Scene::Scene& scene, Scene::Object& object)
		{
			updateTrackInterpObjects(scene, &object);
		});
	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{
		// Release any old interpolation objects
		for (auto& track : object.component<KeyFramedAnimComponent>().m_tracks)
		{
			if (track.second.m_interp)
			{
				gsl_interp_free(track.second.m_interp);
				gsl_interp_accel_free(track.second.m_interpAccel);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	float sampleTrackSimple(KeyframedAnim::KeyFramedAnimTrack const& track, float time)
	{
		// Skip over empty frames
		if (track.m_frames.empty()) return 0.0f;

		// Iterators to the two frames to interpolate
		std::vector<KeyframedAnim::KeyFramedAnimNode>::const_iterator frameIts[2];

		if (time <= track.m_frames.front().m_time)
		{
			frameIts[0] = frameIts[1] = track.m_frames.begin();
		}
		else if (time >= track.m_frames.back().m_time)
		{
			frameIts[0] = frameIts[1] = track.m_frames.end() - 1;
		}
		else
		{
			frameIts[1] = std::lower_bound(track.m_frames.begin(), track.m_frames.end(), time, [](auto const& node, auto const& time) { return node.m_time < time; });
			frameIts[0] = frameIts[1] - 1;
		}

		// Interpolation factor
		float alpha = (time - frameIts[0]->m_time) / glm::max(frameIts[1]->m_time - frameIts[0]->m_time, 1e-3f);

		// Lerp between the two values
		return glm::mix(frameIts[0]->m_value, frameIts[1]->m_value, alpha);
	}

	////////////////////////////////////////////////////////////////////////////////
	float sampleTrackGsl(KeyframedAnim::KeyFramedAnimTrack const& track, float time)
	{
		if (time <= track.m_frames.front().m_time) return track.m_frames.front().m_value;
		if (time >= track.m_frames.back().m_time) return track.m_frames.back().m_value;
		return (float)gsl_interp_eval(track.m_interp, track.m_times.data(), track.m_values.data(), time, track.m_interpAccel);
	}

	////////////////////////////////////////////////////////////////////////////////
	float sampleTrack(KeyframedAnim::KeyFramedAnimTrack const& track, float time)
	{
		return sampleTrackGsl(track, time);
		//return sampleTrackSimple(track, time);
	}

	////////////////////////////////////////////////////////////////////////////////
	float sampleTrack(Scene::Scene& scene, Scene::Object* object, KeyframedAnim::KeyFramedAnimTrack const& track, float time)
	{
		return sampleTrack(track, glm::clamp(time, 0.0f, object->component<KeyFramedAnimComponent>().m_length));
	}
	////////////////////////////////////////////////////////////////////////////////
	const gsl_interp_type* determineInterpType(Scene::Scene& scene, Scene::Object* object, KeyFramedAnimTrack& track)
	{
		// Determine the interpolation type to use
		const gsl_interp_type* interpType;
		switch (track.m_interpolationMethod)
		{
		case KeyFramedAnimTrack::Linear:
			interpType = gsl_interp_linear;
			break;
		case KeyFramedAnimTrack::Cubic:
			interpType = gsl_interp_cspline;
			break;
		}
		return interpType;
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateTrackInterp(Scene::Scene& scene, Scene::Object* object, KeyFramedAnimTrack& track)
	{
		// Reset the min and max values
		track.m_min = FLT_MAX;
		track.m_max = -FLT_MAX;

		// Go through the curve and update the min/max values by sampling the curve
		for (float t = 0.0f; t < object->component<KeyFramedAnimComponent>().m_length; t += 0.01f)
		{
			float value = sampleTrack(track, t);
			track.m_min = glm::min(track.m_min, value);
			track.m_max = glm::max(track.m_max, value);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateTrackFast(Scene::Scene& scene, Scene::Object* object, KeyFramedAnimTrack& track)
	{
		// Reset the min and max values
		track.m_min = FLT_MAX;
		track.m_max = -FLT_MAX;

		// Go through each frame and compute the min/max
		for (auto frame : track.m_frames)
		{
			track.m_min = glm::min(track.m_min, frame.m_value);
			track.m_max = glm::max(track.m_max, frame.m_value);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateTrackInterpObjects(Scene::Scene& scene, Scene::Object* object, KeyFramedAnimTrack& track)
	{
		// Sort the frames
		std::sort(track.m_frames.begin(), track.m_frames.end(), [](auto const& a, auto const& b) { return a.m_time < b.m_time; });

		// Clear the continous buffers
		track.m_times.clear();
		track.m_times.reserve(track.m_frames.size());
		track.m_values.clear();
		track.m_values.reserve(track.m_frames.size());

		// Insertiont iterators
		auto timeInsertIt = track.m_times.begin();
		auto valueInsertIt = track.m_values.begin();

		// Populate these buffers
		for (size_t i = 0; i < track.m_frames.size(); ++i)
		{
			if (track.m_times.empty() || glm::abs(track.m_times.back() - track.m_frames[i].m_time) > 1e-5f)
			{
				track.m_times.push_back(track.m_frames[i].m_time);
				track.m_values.push_back(track.m_frames[i].m_value);
			}
		}

		// Determine the interpolation type to use
		const gsl_interp_type* interpType = determineInterpType(scene, object, track);

		// Release any old interpolation objects
		if (track.m_interp)
		{
			gsl_interp_free(track.m_interp);
			gsl_interp_accel_reset(track.m_interpAccel);
		}

		// Create the interpolation objects
		else
		{
			track.m_interpAccel = gsl_interp_accel_alloc();
		}

		// Init the interp object
		track.m_interp = gsl_interp_alloc(interpType, track.m_times.size());
		gsl_interp_init(track.m_interp, track.m_times.data(), track.m_values.data(), track.m_times.size());

		// Compute the min/max values
		switch (track.m_interpolationMethod)
		{
		case KeyFramedAnimTrack::Linear:
			updateTrackFast(scene, object, track);
			break;
		case KeyFramedAnimTrack::Cubic:
			updateTrackInterp(scene, object, track);
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateTrackInterpObjects(Scene::Scene& scene, Scene::Object* object)
	{
		for (auto& track : object->component<KeyFramedAnimComponent>().m_tracks)
			KeyframedAnim::updateTrackInterpObjects(scene, object, track.second);
	}

	////////////////////////////////////////////////////////////////////////////////
	void play(Scene::Scene& scene, Scene::Object* object)
	{
		play(scene, object, object->component<KeyFramedAnimComponent>().m_playDirection);
	}

	////////////////////////////////////////////////////////////////////////////////
	void play(Scene::Scene& scene, Scene::Object* object, KeyFramedAnimComponent::PlayDirection direction, float time, float speed)
	{
		object->component<KeyFramedAnimComponent>().m_playState = KeyFramedAnimComponent::Playing;
		object->component<KeyFramedAnimComponent>().m_playDirection = direction;
		object->component<KeyFramedAnimComponent>().m_currentPlayDirection = (direction == KeyFramedAnimComponent::PingPong) ? 
			KeyFramedAnimComponent::Forward : direction;
		if (time >= -1e-3f) object->component<KeyFramedAnimComponent>().m_currentTime = time;
		if (speed >= -1e-3f) object->component<KeyFramedAnimComponent>().m_playbackSpeed = speed;
		if (object->component<KeyFramedAnimComponent>().m_currentPlayRecorded = object->component<KeyFramedAnimComponent>().m_recordPlayback)
		{
			DelayedJobs::postJob(scene, object, "Start Recording", true, 5, [](Scene::Scene& scene, Scene::Object& object)
			{
				RecordSettings::startRecording(scene);
			});
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void playFromStart(Scene::Scene& scene, Scene::Object* object)
	{
		auto playDirection = object->component<KeyFramedAnimComponent>().m_playDirection == KeyFramedAnimComponent::Reverse ? 
			KeyFramedAnimComponent::Forward : object->component<KeyFramedAnimComponent>().m_playDirection;
		play(scene, object, playDirection, object->component<KeyFramedAnimComponent>().m_playbackStart);
	}

	////////////////////////////////////////////////////////////////////////////////
	void playFromEnd(Scene::Scene& scene, Scene::Object* object)
	{
		play(scene, object, KeyFramedAnimComponent::Reverse, object->component<KeyFramedAnimComponent>().m_length);
	}

	////////////////////////////////////////////////////////////////////////////////
	void playForward(Scene::Scene& scene, Scene::Object* object)
	{
		play(scene, object, KeyFramedAnimComponent::Forward);
	}

	////////////////////////////////////////////////////////////////////////////////
	void playReverse(Scene::Scene& scene, Scene::Object* object)
	{
		play(scene, object, KeyFramedAnimComponent::Reverse);
	}

	////////////////////////////////////////////////////////////////////////////////
	void playPingPong(Scene::Scene& scene, Scene::Object* object)
	{
		play(scene, object, KeyFramedAnimComponent::PingPong);
	}

	////////////////////////////////////////////////////////////////////////////////
	void pause(Scene::Scene& scene, Scene::Object* object)
	{
		object->component<KeyFramedAnimComponent>().m_playState = KeyFramedAnimComponent::Paused;
	}

	////////////////////////////////////////////////////////////////////////////////
	void stop(Scene::Scene& scene, Scene::Object* object)
	{
		object->component<KeyFramedAnimComponent>().m_playState = KeyFramedAnimComponent::Stopped;
		object->component<KeyFramedAnimComponent>().m_playDirection = KeyFramedAnimComponent::Forward;
		object->component<KeyFramedAnimComponent>().m_currentPlayDirection = KeyFramedAnimComponent::Forward;
		object->component<KeyFramedAnimComponent>().m_currentTime = 0.0f;
		if (object->component<KeyFramedAnimComponent>().m_currentPlayRecorded)
		{
			RecordSettings::stopRecording(scene);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace AnimDataRecording
	{

		////////////////////////////////////////////////////////////////////////////////
		void clearAnimData(Scene::Scene& scene, Scene::Object* object)
		{
			auto& tracks = object->component<KeyFramedAnimComponent>().m_tracks;
			size_t numFrames = tracks.begin()->second.m_frames.size();

			// Clean up the empty regions at the start
			size_t firstFrameId = 0;
			for (size_t i = 1; i < numFrames; ++i)
			{
				bool differenceFound = false;
				for (auto& track : tracks)
				{
					if (track.second.m_frames[i].m_value != track.second.m_frames[i - 1].m_value)
					{
						differenceFound = true; break;
					}
				}
				if (differenceFound) break;
				++firstFrameId;
			}

			// Clean up the emptry regions at the end
			size_t lastFrameId = numFrames - 1;
			for (size_t i = 1; i < numFrames; ++i)
			{
				bool differenceFound = false;
				for (auto& track : tracks)
				{
					if (track.second.m_frames[numFrames - 1 - i].m_value != track.second.m_frames[numFrames - i].m_value)
					{
						differenceFound = true; break;
					}
				}
				if (differenceFound) break;
				--lastFrameId;
			}

			// Update frame times to reflect changes
			float firstFrameTime = object->component<KeyFramedAnimComponent>().m_tracks.begin()->second.m_frames[firstFrameId].m_time;
			for (auto& track : object->component<KeyFramedAnimComponent>().m_tracks)
			{
				// Erase the unnecessary values
				track.second.m_frames = std::vector<KeyFramedAnimNode>(
					track.second.m_frames.begin() + firstFrameId,
					track.second.m_frames.begin() + lastFrameId);

				// update the frame times
				for (auto& frame : track.second.m_frames)
					frame.m_time -= firstFrameTime;
			}

			// Update the anim duration
			object->component<KeyFramedAnimComponent>().m_length =
				object->component<KeyFramedAnimComponent>().m_tracks.begin()->second.m_frames.back().m_time;

			/*
			// Collapse duplicate nodes
			for (auto& track : tracks)
			{
				// Frame list
				auto& frames = track.second.m_frames;

				// Magnitude of the average value (for scaling the difference vector)
				float avg = std::accumulate(frames.begin(), frames.end(), 0.0f, [](float sum, KeyFramedAnimNode const& node) { return sum + std::abs(node.m_value); }) / frames.size();
				float mag = std::pow(10.0f, std::ceilf(std::log10(avg)));

				// Linearity threshold
				//float linearThreshold = object->component<KeyFramedAnimComponent>().m_record.m_linearThreshold * mag;
				float linearThreshold = (object->component<KeyFramedAnimComponent>().m_record.m_linearThreshold * mag) /
					object->component<KeyFramedAnimComponent>().m_record.m_timePerFrame;

				Debug::log_debug() << "Cleaning up track: " << track.first << Debug::end;
				Debug::log_debug() << "-- Avg. value: " << avg << Debug::end;
				Debug::log_debug() << "-- Avg. magnitude: " << mag << Debug::end;
				Debug::log_debug() << "-- Linear threshold: " << linearThreshold << Debug::end;

				// Start from the first node
				std::vector<KeyFramedAnimNode> resultFrames = { frames.front() };

				// Add unique nodes
				float prevDir = (frames[1].m_value - frames[0].m_value) / (frames[1].m_time - frames[0].m_time);
				for (size_t i = 2; i < frames.size(); ++i)
				{
					float currentDir = (frames[i].m_value - frames[i - 1].m_value) / (frames[i].m_time - frames[i - 1].m_time);
					if (glm::abs(currentDir - prevDir) > linearThreshold)
					{
						resultFrames.push_back(frames[i - 1]);
					}
				}

				// Append the last frame
				resultFrames.push_back(frames.back());

				// Store the resulting frames
				frames = resultFrames;
			}
			*/
		}

		////////////////////////////////////////////////////////////////////////////////
		void startRecording(Scene::Scene& scene, Scene::Object* object)
		{
			stop(scene, object);

			Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
			float currentTime = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_globalTime;

			object->component<KeyframedAnim::KeyFramedAnimComponent>().m_isRecording = true;
			object->component<KeyframedAnim::KeyFramedAnimComponent>().m_timeSinceLastFrame = 0.0f;
			object->component<KeyframedAnim::KeyFramedAnimComponent>().m_recordStartTime = currentTime;

			Debug::log_debug() << "Recording started at " << currentTime << Debug::end;

			object->component<KeyFramedAnimComponent>().m_isRecording = true;

			for (auto& track : object->component<KeyFramedAnimComponent>().m_tracks)
			{
				track.second.m_frames.clear();
				track.second.m_frames.push_back({ 0.0f, *(track.second.m_variables.begin()->second) });
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		void stopRecording(Scene::Scene& scene, Scene::Object* object)
		{
			Scene::Object* simulationSettings = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_SIMULATION_SETTINGS);
			float currentTime = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_globalTime;

			Debug::log_debug() << "Recording stopped at " << currentTime << Debug::end;

			object->component<KeyFramedAnimComponent>().m_isRecording = false;
			object->component<KeyFramedAnimComponent>().m_length = currentTime - object->component<KeyFramedAnimComponent>().m_recordStartTime;

			// Append the last frame
			for (auto& track : object->component<KeyFramedAnimComponent>().m_tracks)
			{
				if ((object->component<KeyFramedAnimComponent>().m_length - track.second.m_frames.end()->m_value) > object->component<KeyFramedAnimComponent>().m_record.m_timePerFrame * 0.2f)
					track.second.m_frames.push_back({ object->component<KeyFramedAnimComponent>().m_length, *(track.second.m_variables.begin()->second) });
			}

			// Clear the animation data
			clearAnimData(scene, object);

			object->component<KeyFramedAnimComponent>().m_length += object->component<KeyFramedAnimComponent>().m_record.m_startPadding;
			object->component<KeyFramedAnimComponent>().m_length += object->component<KeyFramedAnimComponent>().m_record.m_endPadding;

			// Handle the start and end padding
			for (auto& track : object->component<KeyFramedAnimComponent>().m_tracks)
			for (auto& frame: track.second.m_frames)
			{
				frame.m_time += object->component<KeyFramedAnimComponent>().m_record.m_startPadding;
			}

			// Update the tracks
			KeyframedAnim::updateTrackInterpObjects(scene, object);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, object->m_name);

		// Compute the animation delta time
		const float multiplier = (object->component<KeyFramedAnimComponent>().m_currentPlayDirection == KeyFramedAnimComponent::Forward ? 1.0f : -1.0f);
		const float playbackSpeed = object->component<KeyFramedAnimComponent>().m_playbackSpeed;
		const float delta = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_deltaTime;
		const float scaledDelta = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_scaledDeltaTime;
		const float fixedDelta = simulationSettings->component<SimulationSettings::SimulationSettingsComponent>().m_fixedDeltaTime;
		float animDelta = playbackSpeed * multiplier;

		// Compute the animation delta time
		switch (object->component<KeyFramedAnimComponent>().m_playbackType)
		{
			case KeyFramedAnimComponent::RealTime: animDelta *= scaledDelta; break;
			case KeyFramedAnimComponent::Synced:   animDelta *= fixedDelta; break;
		}

		// Don't animate if we are not playing currently
		if (object->component<KeyFramedAnimComponent>().m_playState != KeyFramedAnimComponent::Playing)
			animDelta *= 0.0f;

		// Update the current time
		object->component<KeyFramedAnimComponent>().m_currentTime = glm::clamp(
			object->component<KeyFramedAnimComponent>().m_currentTime + animDelta,
			0.0f, object->component<KeyFramedAnimComponent>().m_length);

		// Update the current time if the animation is playing
		if (object->component<KeyFramedAnimComponent>().m_playState == KeyFramedAnimComponent::Playing)
		{
			// Update the current play direction for pingpong anims
			if (object->component<KeyFramedAnimComponent>().m_playDirection == KeyFramedAnimComponent::PingPong)
			{
				if (object->component<KeyFramedAnimComponent>().m_currentTime == object->component<KeyFramedAnimComponent>().m_length)
					object->component<KeyFramedAnimComponent>().m_currentPlayDirection = KeyFramedAnimComponent::Reverse;

				if (object->component<KeyFramedAnimComponent>().m_currentTime == 0.0f && object->component<KeyFramedAnimComponent>().m_looping)
					object->component<KeyFramedAnimComponent>().m_currentPlayDirection = KeyFramedAnimComponent::Forward;
			}
			else if (object->component<KeyFramedAnimComponent>().m_looping)
			{
				if (object->component<KeyFramedAnimComponent>().m_currentTime == object->component<KeyFramedAnimComponent>().m_length)
					object->component<KeyFramedAnimComponent>().m_currentTime = 0.0f;

				if (object->component<KeyFramedAnimComponent>().m_currentTime == 0.0f)
					object->component<KeyFramedAnimComponent>().m_currentTime = object->component<KeyFramedAnimComponent>().m_length;
			}
		}

		// Update each track
		if (object->component<KeyFramedAnimComponent>().m_playState != KeyFramedAnimComponent::Stopped ||
			(object->component<KeyFramedAnimComponent>().m_generateWhileStopped && object->component<KeyFramedAnimComponent>().m_isRecording == false))
		{
			for (const auto& trackIt : object->component<KeyFramedAnimComponent>().m_tracks)
			{
				Profiler::ScopedCpuPerfCounter perfCounter(scene, trackIt.first);

				// Sample the track
				float sample = KeyframedAnim::sampleTrack(trackIt.second, object->component<KeyFramedAnimComponent>().m_currentTime);

				// Write the new variable into each track
				for (auto variable : trackIt.second.m_variables) (*(variable.second)) = sample;
			}
		}

		// Stop on finish
		if (object->component<KeyFramedAnimComponent>().m_playState == KeyFramedAnimComponent::Playing)
		{
			// Evaluate the stop condition
			const bool shouldStop =
				(object->component<KeyFramedAnimComponent>().m_playDirection == KeyFramedAnimComponent::Forward &&
					object->component<KeyFramedAnimComponent>().m_currentTime == object->component<KeyFramedAnimComponent>().m_length) ||
				(object->component<KeyFramedAnimComponent>().m_playDirection == KeyFramedAnimComponent::Reverse &&
					object->component<KeyFramedAnimComponent>().m_currentTime == 0.0f) ||
				(object->component<KeyFramedAnimComponent>().m_playDirection == KeyFramedAnimComponent::PingPong &&
					object->component<KeyFramedAnimComponent>().m_currentPlayDirection== KeyFramedAnimComponent::Reverse &&
					object->component<KeyFramedAnimComponent>().m_currentTime == 0.0f);

			// Stop if, if we need to
			if (shouldStop)
			{
				object->component<KeyFramedAnimComponent>().m_playState = KeyFramedAnimComponent::Stopped;
				if (object->component<KeyFramedAnimComponent>().m_currentPlayRecorded)
				{
					DelayedJobs::postJob(scene, object, "Finish Recording", true, 5, [](Scene::Scene& scene, Scene::Object& object)
					{
						RecordSettings::stopRecording(scene);
					});
				}
			}
		}

		// Recording
		if (object->component<KeyFramedAnimComponent>().m_isRecording)
		{
			float& timeSinceLastFrame = object->component<KeyFramedAnimComponent>().m_timeSinceLastFrame;
			object->component<KeyFramedAnimComponent>().m_timeSinceLastFrame += delta;
			if (timeSinceLastFrame >= object->component<KeyFramedAnimComponent>().m_record.m_timePerFrame)
			{
				for (auto& track : object->component<KeyFramedAnimComponent>().m_tracks)
				{
					float frameTime = track.second.m_frames.back().m_time + timeSinceLastFrame;
					track.second.m_frames.push_back({ frameTime, *(track.second.m_variables.begin()->second) });
				}

				timeSinceLastFrame = 0.0f;
				object->component<KeyFramedAnimComponent>().m_length = 
					object->component<KeyFramedAnimComponent>().m_tracks.begin()->second.m_frames.back().m_time;
				updateTrackInterpObjects(scene, object);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace TrackCurveChart
	{
		// Total number of samples in the chart
		static const size_t NUM_SAMPLES = 100;

		////////////////////////////////////////////////////////////////////////////////
		struct ChartGeneratorPayload
		{
			// Necessary scene information
			Scene::Object* m_object;

			// Pointer to the iterator track
			KeyFramedAnimTrack* m_track;

			// Width of one sample
			float m_sampleWidth;
		};

		////////////////////////////////////////////////////////////////////////////////
		float chartDataGenerator(const void* pPayload, int index, float x)
		{
			// Extract the payload
			ChartGeneratorPayload const& payload = *(const ChartGeneratorPayload*)pPayload;

			// Position of the sample along the track
			float samplePos = index * payload.m_sampleWidth;

			// Return the sampled track
			return KeyframedAnim::sampleTrack(*payload.m_track, samplePos);
		}

		////////////////////////////////////////////////////////////////////////////////
		void chartTooltipGenerator(const void* pPayload, int v_idx, float x, float y)
		{
			// Extract the payload
			ChartGeneratorPayload const& payload = *(const ChartGeneratorPayload*)pPayload;

			// Position of the sample along the track
			float samplePos = v_idx * payload.m_sampleWidth;

			// Sample the curve
			float sampleVal = KeyframedAnim::sampleTrack(*payload.m_track, samplePos);

			// Generate the actual tooltip
			ImGui::BeginTooltip();
			ImGui::Text("%f: %f", samplePos, sampleVal);
			ImGui::EndTooltip();
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateXTickLabel(const void* pPayload, char* buffer, int buf_size, float val)
		{
			// Extract the payload
			ChartGeneratorPayload const& payload = *(const ChartGeneratorPayload*)pPayload;

			// Position of the sample along the track
			float samplePos = val * payload.m_sampleWidth;

			// Construct the label
			std::string label = Units::secondsToString(samplePos);
			std::strcpy(buffer, label.c_str());
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateYTickLabel(const void* pPayload, char* buffer, int buf_size, float val)
		{
			sprintf(buffer, "%.2f", val);
		}

		////////////////////////////////////////////////////////////////////////////////
		void generateChart(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object, KeyFramedAnimTrack& track)
		{
			// Chart generator payload
			ChartGeneratorPayload payload;
			payload.m_object = object;
			payload.m_track = &track;
			payload.m_sampleWidth = object->component<KeyFramedAnimComponent>().m_length / (NUM_SAMPLES - 1);

			// Index of the current time
			float currentTime = 100.0f * (object->component<KeyFramedAnimComponent>().m_currentTime / object->component<KeyFramedAnimComponent>().m_length);

			// Overlay text
			std::stringstream overlayStream;
			overlayStream.precision(2);

			overlayStream <<
				std::fixed << object->component<KeyFramedAnimComponent>().m_currentTime << " / " <<
				std::fixed << object->component<KeyFramedAnimComponent>().m_length << " | " <<
				KeyFramedAnimComponent::PlayState_value_to_string(object->component<KeyFramedAnimComponent>().m_playState);
			std::string overlayText = overlayStream.str();

			// Plot settings
			ImGui::PlotConfig plotConfig;
			plotConfig.values.ys_count = 1;
			plotConfig.values.ysg = chartDataGenerator;
			plotConfig.values.color = 0;
			plotConfig.values.user_data = &payload;
			plotConfig.values.count = NUM_SAMPLES;
			plotConfig.values.offset = 0;
			plotConfig.grid_x.show = true;
			plotConfig.grid_x.tick_major.color = ImGui::Color32FromGlmVector(glm::vec4(0.0f, 0.0f, 0.0f, 0.125f));
			plotConfig.grid_x.tick_major.thickness = 2.0f;
			plotConfig.grid_x.tick_major.label_fn = &generateXTickLabel;
			plotConfig.grid_x.tick_major.label_size = 0.75f;
			plotConfig.grid_x.tick_major.label_color = ImGui::Color32FromGlmVector(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			plotConfig.grid_x.ticks = 7;
			plotConfig.grid_x.subticks = 1;
			plotConfig.grid_y.show = true;
			plotConfig.grid_y.tick_major.color = ImGui::Color32FromGlmVector(glm::vec4(0.0f, 0.0f, 0.0f, 0.125f));
			plotConfig.grid_y.tick_major.thickness = 2.0f;
			plotConfig.grid_y.tick_major.label_fn = &generateYTickLabel;
			plotConfig.grid_y.tick_major.label_size = 0.75f;
			plotConfig.grid_y.tick_major.label_color = ImGui::Color32FromGlmVector(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			plotConfig.grid_y.ticks = 5;
			plotConfig.grid_y.subticks = 1;
			plotConfig.tooltip.show = true;
			plotConfig.tooltip.generator = &chartTooltipGenerator;
			plotConfig.highlight.show = true;
			plotConfig.highlight.only_first = true;
			plotConfig.scale.min = track.m_min;
			plotConfig.scale.max = track.m_max;
			plotConfig.v_lines.show = true;
			plotConfig.v_lines.xs = &currentTime;
			plotConfig.v_lines.count = 1;
			plotConfig.v_lines.color = ImGui::Color32FromGlmVector(glm::vec4{ 0.81f, 0.21f, 0.21f, 1.0f });
			plotConfig.v_lines.label = true;
			plotConfig.v_lines.label_color = plotConfig.v_lines.color;
			plotConfig.v_lines.label_size = 0.75f;
			plotConfig.v_lines.label_fn = &generateXTickLabel;
			plotConfig.overlay.show = true;
			plotConfig.overlay.text = overlayText.c_str();
			plotConfig.overlay.scale = 2.0f;
			plotConfig.overlay.position = ImVec2(0.5f, -1.0f);
			plotConfig.frame_size = ImVec2(0.0f, 350.0f);
			plotConfig.line_thickness = 2.0f;
			plotConfig.skip_small_lines = true;

			ImGui::PushItemWidth(-1);
			ImGui::PlotEx("##label", plotConfig);
			ImGui::PopItemWidth();
		}
	}
	
	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		bool animPropertiesChanged = false, interpChanged = false, framesChanged = false;

		ImGui::Combo("Play Mode", &object->component<KeyFramedAnimComponent>().m_playbackType, KeyFramedAnimComponent::PlayType_meta);
		ImGui::Combo("Play Direction", &object->component<KeyFramedAnimComponent>().m_playDirection, KeyFramedAnimComponent::PlayDirection_meta);
		ImGui::Checkbox("Looping", &object->component<KeyFramedAnimComponent>().m_looping);
		animPropertiesChanged |= ImGui::DragFloat("Length", &object->component<KeyFramedAnimComponent>().m_length, 0.01f);
		ImGui::SliderFloat("Playback Start", &object->component<KeyFramedAnimComponent>().m_playbackStart, 0.0f, object->component<KeyFramedAnimComponent>().m_length);
		ImGui::DragFloat("Playback Speed", &object->component<KeyFramedAnimComponent>().m_playbackSpeed, 0.01f);

		// Playing
		if (ImGui::ButtonEx("Play", "######"))
		{
			KeyframedAnim::play(scene, object);
		}
		ImGui::SameLine();
		if (ImGui::ButtonEx("Pause", "######"))
		{
			KeyframedAnim::pause(scene, object);
		}
		ImGui::SameLine();
		if (ImGui::ButtonEx("Start", "######"))
		{
			KeyframedAnim::playFromStart(scene, object);
		}
		ImGui::SameLine();
		if (ImGui::ButtonEx("End", "######"))
		{
			KeyframedAnim::playFromEnd(scene, object);
		}
		ImGui::SameLine();
		if (ImGui::ButtonEx("Stop", "######"))
		{
			KeyframedAnim::stop(scene, object);
		}
		ImGui::Checkbox("Record Playback", &object->component<KeyFramedAnimComponent>().m_recordPlayback);
		ImGui::SameLine();
		ImGui::Checkbox("Output While Stopped", &object->component<KeyFramedAnimComponent>().m_generateWhileStopped);

		ImGui::Separator();
		
		ImGui::InputText("Animation File", object->component<KeyFramedAnimComponent>().m_animFileName);
		if (ImGui::Button("Load"))
		{
			loadAnimData(scene, object);
		};
		ImGui::SameLine();
		if (ImGui::Button("Save"))
		{
			saveAnimData(scene, object);
		};

		ImGui::Separator();

		ImGui::SliderFloat("Time Per Frames", &object->component<KeyFramedAnimComponent>().m_record.m_timePerFrame, 0.01f, 5.0f);
		ImGui::SliderFloat("Cleanup Threshold", &object->component<KeyFramedAnimComponent>().m_record.m_linearThreshold, 0.0001f, 0.1f, "%.6f");
		ImGui::SliderFloat("Start Padding", &object->component<KeyFramedAnimComponent>().m_record.m_startPadding, 0.0f, 5.0f);
		ImGui::SliderFloat("End Padding", &object->component<KeyFramedAnimComponent>().m_record.m_endPadding, 0.0f, 5.0f);

		if (ImGui::Button("Start Recording"))
		{
			AnimDataRecording::startRecording(scene, object);
		}

		ImGui::SameLine();

		if (ImGui::Button("Stop Recording"))
		{
			AnimDataRecording::stopRecording(scene, object);
		}

		ImGui::Separator();

		// The various tracks of the animation.
		if (ImGui::BeginTabBar("Tracks") == false)
			return;

		// Go through each track
		for (auto& track : object->component<KeyFramedAnimComponent>().m_tracks)
		{
			ImGui::PushID(&track);

			// Generate the frames
			if (ImGui::BeginTabItem(track.first.c_str()))
			{
				interpChanged = ImGui::Combo("Interpolation Method", &track.second.m_interpolationMethod, KeyFramedAnimTrack::InterpolationMethod_meta) || interpChanged;

				// Generate a chart for the track
				TrackCurveChart::generateChart(scene, guiSettings, object, track.second);

				// Generat the frames
				if (ImGui::TreeNodeEx("Frames"))
				{
					for (size_t i = 0; i < track.second.m_frames.size(); ++i)
					{
						float min = (i == 0) ? 0.0f : track.second.m_frames[i - 1].m_time + 1e-3f,
							max = (i == track.second.m_frames.size() - 1) ? object->component<KeyFramedAnimComponent>().m_length : track.second.m_frames[i + 1].m_time - 1e-3f;
						ImGui::Text("%*d.", std::number_of_digits(track.second.m_frames.size() + 1), i + 1);
						ImGui::SameLine();
						ImGui::PushID(i);
						ImGui::PushItemWidth(-1);
						ImGui::DragFloat2("#label", &track.second.m_frames[i].m_time, 0.01f, min, max);
						framesChanged |= ImGui::IsItemDeactivatedAfterEdit();
						ImGui::PopItemWidth();
						ImGui::PopID();
					}

					ImGui::TreePop();
				}

				ImGui::EndTabItem();
			}
			ImGui::PopID();
		}

		if (ImGui::Button("Add Frame"))
		{
			for (auto& track : object->component<KeyFramedAnimComponent>().m_tracks)
			{
				auto newFrame = track.second.m_frames.back();
				newFrame.m_time += 1.0f;
				track.second.m_frames.push_back(newFrame);
				framesChanged = true;
			}
		}

		if (animPropertiesChanged || interpChanged || framesChanged)
		{
			DelayedJobs::postJob(scene, object, "Update Interp Objects", [](Scene::Scene& scene, Scene::Object& object)
			{
				KeyframedAnim::updateTrackInterpObjects(scene, &object);
			});
		}

		ImGui::EndTabBar();
	}
}