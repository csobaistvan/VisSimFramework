#include "PCH.h"
#include "DelayedJobs.h"
#include "SimulationSettings.h"

namespace DelayedJobs
{
	////////////////////////////////////////////////////////////////////////////////
	// Define the component
	DEFINE_COMPONENT(DELAYED_JOBS);
	DEFINE_OBJECT(DELAYED_JOBS);
	REGISTER_OBJECT_UPDATE_CALLBACK(DELAYED_JOBS, BEFORE, END);

	////////////////////////////////////////////////////////////////////////////////
	void initObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void releaseObject(Scene::Scene& scene, Scene::Object& object)
	{

	}

	////////////////////////////////////////////////////////////////////////////////
	void updateObject(Scene::Scene& scene, Scene::Object* simulationSettings, Scene::Object* object)
	{
		doJobs(scene, object);
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGui(Scene::Scene& scene, Scene::Object* guiSettings, Scene::Object* object)
	{
		ImGui::Checkbox("Process Disabled Objects", &object->component<DelayedJobs::DelayedJobsComponent>().m_processDisabledObjects);
		ImGui::SameLine();
		ImGui::Checkbox("Consume Disabled Jobs", &object->component<DelayedJobs::DelayedJobsComponent>().m_consumeDisabledObjects);
		if (ImGui::Button("Clear Queue"))
		{
			object->component<DelayedJobs::DelayedJobsComponent>().m_jobs.clear();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, DelayedJob const& job)
	{
		auto jobQueue = Scene::findFirstObject(scene, Scene::OBJECT_TYPE_DELAYED_JOBS);

		jobQueue->component<DelayedJobs::DelayedJobsComponent>().m_jobs.push_back(job);
	}

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, Scene::Object* object, std::string const& name, bool alwaysComplete, int framesToWait, DelayedJobs::DelayedJobCallback const& callback)
	{
		DelayedJob job;
		job.m_jobName = name;
		job.m_owner = object->m_name;
		job.m_alwaysComplete = alwaysComplete;
		job.m_async = false;
		job.m_delay = framesToWait;
		job.m_callback = callback;
		job.m_complete = false;
		postJob(scene, job);
	}

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, Scene::ObjectType object, std::string const& name, bool alwaysComplete, int framesToWait, DelayedJobs::DelayedJobCallback const& callback)
	{
		postJob(scene, Scene::findFirstObject(scene, object), name, alwaysComplete, framesToWait, callback);
	}

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, std::string object, std::string const& name, bool alwaysComplete, int framesToWait, DelayedJobs::DelayedJobCallback const& callback)
	{
		postJob(scene, &scene.m_objects[object], name, alwaysComplete, framesToWait, callback);
	}

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, Scene::Object* object, std::string const& name, DelayedJobs::DelayedJobCallback const& callback)
	{
		postJob(scene, object, name, false, 1, callback);
	}

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, Scene::ObjectType object, std::string const& name, DelayedJobs::DelayedJobCallback const& callback)
	{
		postJob(scene, Scene::findFirstObject(scene, object), name, false, 1, callback);
	}

	////////////////////////////////////////////////////////////////////////////////
	void postJob(Scene::Scene& scene, std::string object, std::string const& name, DelayedJobs::DelayedJobCallback const& callback)
	{
		postJob(scene, &scene.m_objects[object], name, false, 1, callback);
	}

	////////////////////////////////////////////////////////////////////////////////
	void runJob(Scene::Scene& scene, DelayedJob& job)
	{
		// Invoke the callback
		job.m_callback(scene, scene.m_objects[job.m_owner]);

		// Mark the job done
		job.m_complete = true;
	}

	////////////////////////////////////////////////////////////////////////////////
	void doJobs(Scene::Scene& scene, Scene::Object* object, bool processDisabled, bool consumeDisabled)
	{
		// Evaluate the jobs
		for (auto& job : object->component<DelayedJobs::DelayedJobsComponent>().m_jobs)
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, Profiler::Category{ job.m_owner, job.m_jobName });
			Debug::DebugRegion region({ job.m_owner, job.m_jobName });

			// Extract the target object
			Scene::Object& owner = scene.m_objects[job.m_owner];

			if (SimulationSettings::isObjectEnabled(scene, &owner) || processDisabled || job.m_alwaysComplete)
			{
				// Decrement the delay counter
				--job.m_delay;

				// Carry out the job, if complete
				if (job.m_delay <= 0)
				{
					// Run it in an async fashion
					if (job.m_async)
					{
						std::async(std::launch::async, runJob, std::ref(scene), std::ref(job));
					}
					else
					{
						// Run the job
						runJob(scene, job);
					}
				}
			}
		}

		// Remove the jobs that are finished
		object->component<DelayedJobs::DelayedJobsComponent>().m_jobs.erase(
			std::remove_if(
				object->component<DelayedJobs::DelayedJobsComponent>().m_jobs.begin(), 
				object->component<DelayedJobs::DelayedJobsComponent>().m_jobs.end(),
				[&](DelayedJobs::DelayedJob const& job) { return job.m_complete; }
			),
			object->component<DelayedJobs::DelayedJobsComponent>().m_jobs.end()
		);
	}

	////////////////////////////////////////////////////////////////////////////////
	void doJobs(Scene::Scene& scene, Scene::Object* object)
	{
		doJobs(scene, object,
			object->component<DelayedJobs::DelayedJobsComponent>().m_processDisabledObjects,
			object->component<DelayedJobs::DelayedJobsComponent>().m_consumeDisabledObjects);
	}
}