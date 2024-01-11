#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"
#include "EditorSettings.h"

////////////////////////////////////////////////////////////////////////////////
/// JOB QUEUE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace DelayedJobs
{
	////////////////////////////////////////////////////////////////////////////////
	/** Component and display name. */
	static constexpr const char* COMPONENT_NAME = "DelayedJobs";
	static constexpr const char* DISPLAY_NAME = "Delayed Jobs";
	static constexpr const char* CATEGORY = "Settings";

	////////////////////////////////////////////////////////////////////////////////
	/** Async job callback. */
	using DelayedJobCallback = std::function<void(Scene::Scene&, Scene::Object&)>;

	////////////////////////////////////////////////////////////////////////////////
	/** Delayed job queue entry. */
	struct DelayedJob
	{
		// Owning object.
		std::string m_owner;

		// Name of the job
		std::string m_jobName;

		// Callback function.
		 DelayedJobCallback m_callback;

		 // Whether this job is to be always completed or not
		 bool m_alwaysComplete = true;

		 // How many frames to wait before carrying out
		 int m_delay = 1;

		 // Whether it should run async or not
		 bool m_async = false;

		 // Whether the job is finished or not
		 bool m_complete = false;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Delayed job queue component. */
	struct DelayedJobsComponent
	{
		// Whether we should process disabled objects or not
		bool m_processDisabledObjects;

		// Whether we should consume jobs corresponding to disabled objects or not
		bool m_consumeDisabledObjects;

		// ---- Private members

		// The jobs stored for delayed execution.
		std::vector<DelayedJob> m_jobs;
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
	void postJob(Scene::Scene& scene, DelayedJob const& job);

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, Scene::Object* object, std::string const& name, bool alwaysComplete, int framesToWait, DelayedJobCallback const& callback);

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, Scene::ObjectType object, std::string const& name, bool alwaysComplete, int framesToWait, DelayedJobCallback const& callback);

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, std::string object, std::string const& name, bool alwaysComplete, int framesToWait, DelayedJobCallback const& callback);

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, Scene::Object* object, std::string const& name, DelayedJobCallback const& callback);

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, Scene::ObjectType object, std::string const& name, DelayedJobCallback const& callback);

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, std::string object, std::string const& name, DelayedJobCallback const& callback);

	////////////////////////////////////////////////////////////////////////////////
	void doJobs(Scene::Scene& scene, Scene::Object* object, bool processDisabled, bool consumeDisabled);

	////////////////////////////////////////////////////////////////////////////////
	void doJobs(Scene::Scene& scene, Scene::Object* object);
}

////////////////////////////////////////////////////////////////////////////////
// Component declaration
DECLARE_COMPONENT(DELAYED_JOBS, DelayedJobsComponent, DelayedJobs::DelayedJobsComponent)
DECLARE_OBJECT(DELAYED_JOBS, COMPONENT_ID_DELAYED_JOBS, COMPONENT_ID_EDITOR_SETTINGS)