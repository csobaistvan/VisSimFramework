#include "PCH.h"
#include "Threading.h"
#include "Config.h"
#include "Debug.h"
#include "StaticInitializer.h"

namespace Threading
{
	////////////////////////////////////////////////////////////////////////////////
	// Id of the current thread (thread local value)
	thread_local size_t s_currentThreadId = 0;

	////////////////////////////////////////////////////////////////////////////////
	// Number of worker threads allowed to work at once
	int numThreads()
	{
		static int s_numThreads = glm::clamp(Config::AttribValue("threads").get<int>(), 0, glm::min((int)std::thread::hardware_concurrency(), (int)Constants::s_maxThreads));
		return s_numThreads;
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t currentThreadId()
	{
		return s_currentThreadId;
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace threaded_execute_impl
	{
		////////////////////////////////////////////////////////////////////////////////
		void initExecutionCommon(ThreadedExecuteEnvironment& environment, ThreadedExecuteParams const& params, const size_t numTotalWorkItems)
		{
			// Configure the rest of the settings
			environment.m_lowestActiveId = 0;
			environment.m_workName = params.m_workName;
			environment.m_workItemName = params.m_workItemName;
			environment.m_progressLogLevel = params.m_progressLogLevel;
			environment.m_numTotalWorkItemsLeft = numTotalWorkItems;
			environment.m_startTime = glfwGetTime();
		}

		////////////////////////////////////////////////////////////////////////////////
		void initExecutionSeq(ThreadedExecuteEnvironment& environment, ThreadedExecuteParams const& params, const size_t numTotalWorkItems, const size_t numItemsPerBatch)
		{
			Debug::log_trace() << "Initializing sequential work with a total of " << numTotalWorkItems << " work items..." << Debug::end;

			// Initialize the global structures
			environment.m_numTotalWorkItems = numTotalWorkItems;
			environment.m_numItemsPerBatch = numItemsPerBatch;
			environment.m_numThreads = 1;
			environment.m_numRunning = 1;

			// Common initialization steps
			initExecutionCommon(environment, params, numTotalWorkItems);
		}

		////////////////////////////////////////////////////////////////////////////////
		void initExecutionDist(ThreadedExecuteEnvironment& environment, ThreadedExecuteParams const& params, const size_t numThreads, const size_t numTotalWorkItems, const size_t numItemsPerBatch)
		{
			Debug::log_trace() << "Initializing threaded work with " << numThreads << " workers with a total of " << numTotalWorkItems << " work items..." << Debug::end;

			// Initialize the global structures
			environment.m_numTotalWorkItems = numTotalWorkItems;
			environment.m_numItemsPerBatch = numItemsPerBatch;
			environment.m_numThreads = numThreads;
			environment.m_numRunning = numThreads;

			// Common initialization steps
			initExecutionCommon(environment, params, numTotalWorkItems);
		}

		////////////////////////////////////////////////////////////////////////////////
		void initExecution(ThreadedExecuteEnvironment& environment, ThreadedExecuteParams const& params, const size_t numThreads, const size_t numTotalWorkItems, const size_t numItemsPerBatch)
		{
			// Invoke the appropriate delegate
			if (numThreads <= 1) initExecutionSeq(environment, params, numTotalWorkItems, numItemsPerBatch);
			else                 initExecutionDist(environment, params, numThreads, numTotalWorkItems, numItemsPerBatch);
		}

		////////////////////////////////////////////////////////////////////////////////
		void cleanupExecutionCommon(ThreadedExecuteEnvironment& environment)
		{
			// Log our progress
			if (environment.m_progressLogLevel != Debug::Null)
				Debug::log_output(environment.m_progressLogLevel) << Debug::end;

			Debug::log_trace() << "Threaded work finished." << Debug::end;
		}

		////////////////////////////////////////////////////////////////////////////////
		void cleanupExecutionSeq(ThreadedExecuteEnvironment& environment)
		{
			// Common cleanup steps
			cleanupExecutionCommon(environment);

			Debug::log_trace() << "Threaded work finished." << Debug::end;
		}

		////////////////////////////////////////////////////////////////////////////////
		void cleanupExecutionDist(ThreadedExecuteEnvironment& environment)
		{
			// Common cleanup steps
			cleanupExecutionCommon(environment);
		}

		////////////////////////////////////////////////////////////////////////////////
		void cleanupExecution(ThreadedExecuteEnvironment& environment)
		{
			// Invoke the appropriate delegate
			if (environment.m_numThreads <= 1) cleanupExecutionSeq(environment);
			else                               cleanupExecutionDist(environment);
		}

		////////////////////////////////////////////////////////////////////////////////
		void initWorkerDist(ThreadedExecuteEnvironment& environment, size_t threadId, size_t numWorkItems)
		{
			Debug::log_trace() << "Initializing worker thread " << threadId << " with " << numWorkItems << " work items..." << Debug::end;

			// Set the current thread id
			s_currentThreadId = threadId;

			// Initialize the thread's attributes
			environment.m_isThreadRunning[threadId] = true;
			environment.m_numWorkItems[threadId] = numWorkItems;
			environment.m_numWorkItemsLeft[threadId] = numWorkItems;
			environment.m_queueIds[threadId] = threadId;
		}

		////////////////////////////////////////////////////////////////////////////////
		void initWorkerSeq(ThreadedExecuteEnvironment& environment, size_t threadId, size_t numWorkItems)
		{
			Debug::log_trace() << "Initializing worker thread " << threadId << " with " << numWorkItems << " work items..." << Debug::end;

			// Set the current thread id
			s_currentThreadId = 0;

			// Initialize the thread's attributes
			environment.m_isThreadRunning[threadId] = true;
			environment.m_numWorkItems[threadId] = numWorkItems;
			environment.m_numWorkItemsLeft[threadId] = numWorkItems;
			environment.m_queueIds[threadId] = threadId;
		}

		////////////////////////////////////////////////////////////////////////////////
		void initWorker(ThreadedExecuteEnvironment& environment, size_t threadId, size_t numWorkItems)
		{
			if (environment.m_numThreads <= 1) initWorkerSeq(environment, threadId, numWorkItems);
			else                               initWorkerDist(environment, threadId, numWorkItems);
		}

		////////////////////////////////////////////////////////////////////////////////
		void cleanupWorkerDist(ThreadedExecuteEnvironment& environment, size_t threadId)
		{
			Debug::log_trace() << "Cleaning up worker thread " << threadId << Debug::end;

			// Signal that the thread is off
			environment.m_isThreadRunning[threadId] = false;

			{
				std::lock_guard lockGuard(environment.m_statusUpdateLock);

				// Decrement the running index
				--environment.m_numRunning;

				// Update the first active thread index
				environment.m_lowestActiveId = std::numeric_limits<size_t>::max();
				for (size_t i = 0; i < environment.m_numThreads; ++i)
				{
					if (environment.m_isThreadRunning[i])
					{
						environment.m_lowestActiveId = i;
						break;
					}
				}

				Debug::log_trace() << environment.m_numRunning << " threads still running, lowest worker thread id: " << environment.m_lowestActiveId << Debug::end;
			}

			// Reset the current thread id; this is a workaround for the case when
			// the main thread is also utilized as a worker
			s_currentThreadId = 0;
		}

		////////////////////////////////////////////////////////////////////////////////
		void cleanupWorkerSeq(ThreadedExecuteEnvironment& environment)
		{
			// Reset the current thread id; this is a workaround for the case when
			// the main thread is also utilized as a worker
			s_currentThreadId = 0;
		}

		////////////////////////////////////////////////////////////////////////////////
		void cleanupWorker(ThreadedExecuteEnvironment& environment, size_t threadId)
		{
			if (environment.m_numThreads <= 1) cleanupWorkerSeq(environment);
			else                               cleanupWorkerDist(environment, threadId);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	STATIC_INITIALIZER()
	{
		// @CONSOLE_VAR(Application, Max Threads, -threads, 16, 12, 10, 8, 6, 4, 2, 1)
		Config::registerConfigAttribute(Config::AttributeDescriptor{
			"threads", "Application",
			"Maximum allowed number of threads.",
			"N", { "16" }, {},
			Config::attribRegexInt()
		});
	};
}