#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Constants.h"
#include "Debug.h"
#include "DateTime.h"

#include "LibraryExtensions/StdEx.h"

////////////////////////////////////////////////////////////////////////////////
/// THREADING HELPERS
////////////////////////////////////////////////////////////////////////////////
namespace Threading
{
	////////////////////////////////////////////////////////////////////////////////
	meta_enum(ThreadedWorkDistribution, int, Linear, Interleaved);

	////////////////////////////////////////////////////////////////////////////////
	meta_enum(WorkStealingStrategy, int, NoWorkStealing, SimpleWorkStealing);

	////////////////////////////////////////////////////////////////////////////////
	// Number of worker threads allowed to work at once
	int numThreads();

	////////////////////////////////////////////////////////////////////////////////
	// Id of the current thread
	size_t currentThreadId();

	////////////////////////////////////////////////////////////////////////////////
	template<typename Callback>
	void invoke_for_threads(int threadId, Callback const& callback)
	{
		for (int i = (threadId == numThreads() ? 0 : threadId); i < (threadId == numThreads() ? numThreads() : threadId + 1); ++i)
			callback(i);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Callback>
	void invoke_for_threads(int threadId, int maxThreads, Callback const& callback)
	{
		for (int i = (threadId == numThreads() ? 0 : maxThreads); i < (threadId == numThreads() ? maxThreads : threadId + 1); ++i)
			callback(i);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Structure describing the parameters for a threaded work. */
	struct ThreadedExecuteParams
	{
		ThreadedExecuteParams(size_t numThreads = 1, std::string const& workName = "", std::string const& workItemName = "",
			Debug::DebugOutputLevel progressLogLevel = Debug::Null,
			ThreadedWorkDistribution workDistribution = Interleaved, WorkStealingStrategy workStealing = NoWorkStealing):
				m_numThreads(numThreads),
				m_workName(workName),
				m_workItemName(workItemName),
				m_progressLogLevel(progressLogLevel),
				m_workDistribution(workDistribution),
				m_workStealing(workStealing)
		{}

		// The number of threads that this work is distributed between
		size_t m_numThreads = 1;

		// Name of the work
		std::string m_workName = "";

		// Name of the work items
		std::string m_workItemName = "";

		// Progress logging level
		Debug::DebugOutputLevel m_progressLogLevel = Debug::Null;

		// How to distribute the work among the threads
		ThreadedWorkDistribution m_workDistribution = Interleaved;

		// Work stealing approach
		WorkStealingStrategy m_workStealing = NoWorkStealing;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Structure holding various information regarding the threaded execution. */
	struct ThreadedExecuteEnvironment
	{
		// How many threads we are using
		size_t m_numThreads = 0;

		// How many are running right now
		size_t m_numRunning = 0;

		// Index of the first thread that is still running
		size_t m_lowestActiveId = 0;

		// How many work items we have overall
		size_t m_numTotalWorkItems = 0;
		
		// Total number of work items left
		std::atomic_uint m_numTotalWorkItemsLeft;

		// How many work items we have per batch
		size_t m_numItemsPerBatch = 0;

		// Name of the work
		std::string m_workName{ };

		// Name of a work item
		std::string m_workItemName{ };

		// Progress logging level
		Debug::DebugOutputLevel m_progressLogLevel = Debug::Null;

		// How often to log
		double m_logFrequency = 1.0 / 2.0;

		// Start time of the threaded work
		double m_startTime = 0.0;

		// Progress bar components
		size_t m_progressbarMaxLength = 0;
		double m_lastLogTime = 0.0;

		// Lock for status update
		std::mutex m_statusUpdateLock;

		// Mutexes to access each individual thread
		std::array<std::mutex, Constants::s_maxThreads> m_workQueueMutex;

		// Whether the specified thread is running or not
		std::array<bool, Constants::s_maxThreads> m_isThreadRunning;

		// How many work items the specified thread has
		std::array<size_t, Constants::s_maxThreads> m_numWorkItems;

		// How many work items the specified thread has remaining
		std::array<size_t, Constants::s_maxThreads> m_numWorkItemsLeft;

		// Indices of the work queues to use
		std::array<size_t, Constants::s_maxThreads> m_queueIds;

		// Various accessor functions
		inline bool isLeadingThread(size_t threadId = currentThreadId()) const {
			return m_lowestActiveId == threadId;
		}
		inline bool isAlive(size_t threadId = currentThreadId()) const {
			return m_isThreadRunning[threadId];
		}
		inline size_t numWorkItems(size_t threadId = currentThreadId()) const {
			return m_numWorkItems[threadId];
		}
		inline size_t numWorkItemsLeft(size_t threadId = currentThreadId()) const {
			return m_numWorkItemsLeft[threadId];
		}
		inline size_t numWorkItemsCompleted(size_t threadId = currentThreadId()) const {
			return m_numWorkItems[threadId] - m_numWorkItemsLeft[threadId];
		}
		inline size_t nextWorkItem(size_t threadId = currentThreadId()) const {
			return m_numWorkItems[threadId] - m_numWorkItemsLeft[threadId];
		}
		inline size_t numTotalWorkItemsLeft() const {
			return m_numTotalWorkItemsLeft;
		}
		inline size_t numTotalWorkItemsCompleted() const {
			return m_numTotalWorkItems - m_numTotalWorkItemsLeft;
		}
		inline size_t mostNumItemsLeft() const {
			size_t mostItemsLeft = 0;
			for (size_t threadId = 0; threadId < m_numThreads; ++threadId)
				if (isAlive(threadId))
					mostItemsLeft = std::max(mostItemsLeft, numWorkItemsLeft(threadId));
			return mostItemsLeft;
		}
		inline size_t leastNumItemsLeft() const {
			size_t leastItemsLeft = std::numeric_limits<size_t>::max();
			for (size_t threadId = 0; threadId < m_numThreads; ++threadId)
				if (isAlive(threadId))
					leastItemsLeft = std::min(leastItemsLeft, numWorkItemsLeft(threadId));
			return leastItemsLeft;
		}
		inline size_t bestProgress() const {
			size_t bestProgress = 0;
			for (size_t threadId = 0; threadId < m_numThreads; ++threadId)
				if (isAlive(threadId))
					bestProgress = std::max(bestProgress, numWorkItemsCompleted(threadId));
			return bestProgress;
		}
		inline size_t worstProgress() const {
			size_t worstProgress = std::numeric_limits<size_t>::max();
			for (size_t threadId = 0; threadId < m_numThreads; ++threadId)
				if (isAlive(threadId))
					worstProgress = std::min(worstProgress, numWorkItemsCompleted(threadId));
			return worstProgress;
		}
		inline std::string getWorkItemName(const bool plural) const {
			return (m_workItemName.empty() ? "work item" : m_workItemName) + (plural ? "(s)" : "");
		}
		inline double getAvgTimePerItem() const {
			if (bestProgress() <= 1) return 0.0; // ignore 1 as the item is immediately counted when started
			return (glfwGetTime() - m_startTime) / double(worstProgress());
		}
		inline double getExpectedTimeLeft() const {
			return (mostNumItemsLeft() + 1) * getAvgTimePerItem();
		}
		inline std::string getAvgTimePerItemStr() const {
			const double avgTime = getAvgTimePerItem();
			return avgTime == 0.0 ? "N/A" : DateTime::getDeltaTimeFormatted(avgTime, true, true, DateTime::Seconds);
		}
		inline std::string getExpectedTimeLeftStr() const {
			const double expectedTimeLeft = getExpectedTimeLeft();
			return expectedTimeLeft == 0.0 ? "N/A" : DateTime::getDeltaTimeFormatted(expectedTimeLeft, true, true, DateTime::Seconds);
		}
		inline bool shouldLogProgress() const {
			return m_progressLogLevel != Debug::Null &&
				((isLeadingThread() && glfwGetTime() - m_lastLogTime >= m_logFrequency) || m_numTotalWorkItemsLeft == 0);
		}
		inline std::string createProgressBar(const size_t barWidth = 20) {
			const float progressPerc = float(nextWorkItem()) / float(numWorkItems());
			const size_t numBars = size_t(progressPerc * barWidth);
			const size_t numSpaces = barWidth - numBars;

			std::stringstream ss;
			ss << m_workName << ": ";
			ss << "[" << std::string(numBars, '#') << std::string(numSpaces, ' ') << "]: ";
			ss << numTotalWorkItemsCompleted() << "/" << m_numTotalWorkItems << ", ";
			ss << "expected time left: " << getExpectedTimeLeftStr() << " ";
			ss << "(" << getAvgTimePerItemStr() << " per " << getWorkItemName(false) << ") ";
			ss << "[" << m_numRunning << "/" << m_numThreads << " threads active" << "]";
			const size_t totalBarLength = std::stringstream_length(ss);
			m_progressbarMaxLength = std::max(totalBarLength, m_progressbarMaxLength);
			ss << std::string(m_progressbarMaxLength - totalBarLength, ' ');
			return ss.str();
		}
		inline void logProgress() {
			if (shouldLogProgress())
			{
				m_lastLogTime = glfwGetTime();
				Debug::log_output(m_progressLogLevel) << Debug::cr << createProgressBar();
			}
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	namespace work_indices_impl
	{
		////////////////////////////////////////////////////////////////////////////////
		template<typename... S>
		struct WorkItemTypes 
		{
			using work_item_type = std::tuple<S...>;
			using work_indices_type = std::vector<std::vector<work_item_type>>;
			using thread_indices_type = std::vector<size_t>;
		};

		////////////////////////////////////////////////////////////////////////////////
		template<typename... S>
		struct WorkIndices
		{
			// Necessary typedefs
			using WorkIndexType = typename work_indices_impl::WorkItemTypes<S...>::work_item_type;
			using WorkIndicesType = typename work_indices_impl::WorkItemTypes<S...>::work_indices_type;
			using ThreadIndicesType = typename work_indices_impl::WorkItemTypes<S...>::thread_indices_type;

			// The number of threads that this work is distributed between
			size_t m_numThreads;

			// How to distribute the work among the threads
			ThreadedWorkDistribution m_workDistribution;

			// Work stealing approach
			WorkStealingStrategy m_workStealing;

			// Number of total items (from each individual source
			WorkIndexType m_numTotalItemsPerSource;

			// Total number of invocations
			size_t m_numTotalInvocations;

			// How many work item per batch
			size_t m_indicesPerBatch;

			// Work item indices
			WorkIndicesType m_workIndices;

			// Individual thread indices to loop through
			ThreadIndicesType m_threadIndices;
		};

		////////////////////////////////////////////////////////////////////////////////
		template<typename S>
		size_t numWorkItems(S items)
		{
			return items;
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename S, typename... Rest>
		size_t numWorkItems(S items, Rest... rest)
		{
			return items * numWorkItems(rest...);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename S, size_t T, size_t N>
		size_t numTotalWorkItems(S numItems, typename std::enable_if<N == T, int>::type = 0)
		{
			return std::get<N>(numItems);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename S, size_t T, size_t N>
		size_t numTotalWorkItems(S numItems, typename std::enable_if<N != T, int>::type = 0)
		{
			return std::get<N>(numItems) * numTotalWorkItems<S, T, N + 1>(numItems);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename S, size_t T, size_t N>
		size_t itemIndex(S indices, S numItems, typename std::enable_if<N == T, int>::type = 0)
		{
			return std::get<N>(indices);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename S, size_t T, size_t N>
		size_t itemIndex(S indices, S numItems, typename std::enable_if<N != T, int>::type = 0)
		{
			// 0 * s1 * s2 + 1 * s2 + 2
			return numTotalWorkItems<S, T, N + 1>(numItems) * std::get<N>(indices) + itemIndex<S, T, N + 1>(indices, numItems);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename W>
		size_t threadIndex(W const& result, size_t itemId)
		{
			switch (result.m_workDistribution)
			{
			case ThreadedWorkDistribution::Linear:
				return itemId / result.m_indicesPerBatch;
			case ThreadedWorkDistribution::Interleaved:
				return itemId % result.m_numThreads;
			}
			return 0;
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename W, size_t T, size_t N>
		void generateWorkItems(W& result, typename W::WorkIndexType base, typename std::enable_if<N == T, int>::type = 0)
		{
			// Generate the work items
			for (size_t i = 0; i < std::get<N>(result.m_numTotalItemsPerSource); ++i)
			{
				// Write out the corresponding item index
				std::get<N>(base) = i;

				// Compute the single item-index for this work item
				size_t itemId = itemIndex<typename W::WorkIndexType, T, 0>(base, result.m_numTotalItemsPerSource);

				// Generate its thread id
				size_t threadId = threadIndex(result, itemId);

				// Store it
				result.m_workIndices[threadId].push_back(base);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename W, size_t T, size_t N>
		void generateWorkItems(W& result, typename W::WorkIndexType base, typename std::enable_if<N != T, int>::type = 0)
		{
			// Generate the work items
			for (size_t i = 0; i < std::get<N>(result.m_numTotalItemsPerSource); ++i)
			{
				// Write out the corresponding item index
				std::get<N>(base) = i;

				// Invoke the child loop
				generateWorkItems<W, T, N + 1>(result, base);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename... S>
		void generateWorkItems(WorkIndices<S...>& result)
		{
			using ItemType = typename work_indices_impl::WorkItemTypes<S...>::work_item_type;
			generateWorkItems<WorkIndices<S...>, sizeof...(S) - 1, 0>(result, ItemType{});
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename S>
		S clampWorkItem(S item)
		{
			return std::max(S(1), item);
			//return item <= 0 ? 1 : item;
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	template<typename... S>
	work_indices_impl::WorkIndices<S...> workIndices(ThreadedExecuteParams const& params, S... workItems)
	{
		// Result of the generation
		work_indices_impl::WorkIndices<S...> result;

		// Store the execute parameters
		result.m_numThreads = std::max(size_t(1), params.m_numThreads);
		result.m_workDistribution = params.m_workDistribution;
		result.m_workStealing = params.m_workStealing;

		// Total number of invocations
		result.m_numTotalInvocations = work_indices_impl::numWorkItems(work_indices_impl::clampWorkItem(workItems)...);

		// Number of indices per batch
		result.m_indicesPerBatch = std::ceil(float(result.m_numTotalInvocations) / float(result.m_numThreads));

		// Store the total number of items
		result.m_numTotalItemsPerSource = std::make_tuple(work_indices_impl::clampWorkItem(workItems)...);

		// Generate the thread indices
		result.m_threadIndices = std::iota<size_t>(result.m_numThreads, 0);

		// Allocate space in the work vectors
		result.m_workIndices.resize(result.m_numThreads);

		// Actually generate the work items
		work_indices_impl::generateWorkItems<S...>(result);

		// Return the result
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace threaded_execute_impl
	{
		////////////////////////////////////////////////////////////////////////////////
		void initExecution(ThreadedExecuteEnvironment& environment, ThreadedExecuteParams const& params, const size_t numThreads, const size_t numTotalWorkItems, const size_t numItemsPerBatch);

		////////////////////////////////////////////////////////////////////////////////
		void cleanupExecution(ThreadedExecuteEnvironment& environment);

		////////////////////////////////////////////////////////////////////////////////
		void initWorker(ThreadedExecuteEnvironment& environment, size_t threadId, size_t numWorkItems);

		////////////////////////////////////////////////////////////////////////////////
		void cleanupWorker(ThreadedExecuteEnvironment& environment, size_t threadId);

		////////////////////////////////////////////////////////////////////////////////
		template<typename Fn, typename W>
		void loopCoreNoWorkStealing(ThreadedExecuteEnvironment& environment, Fn const& fn, W const& indices, size_t threadId)
		{
			// Go through each individual index set
			for (auto const& index : indices.m_workIndices[threadId])
			{
				--environment.m_numWorkItemsLeft[threadId];
				--environment.m_numTotalWorkItemsLeft;
				environment.logProgress();
				fn(environment, index);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename Fn, typename W>
		void loopCoreSimpleWorkStealing(ThreadedExecuteEnvironment& environment, Fn const& fn, W const& indices, size_t threadId)
		{
			// Go through each individual index set
			while (true)
			{
				// Determine the queue to take work from
				const size_t queueIndex = environment.m_queueIds[threadId];

				// Whether we have any work left to do or not
				bool hasWorkLeft = false;

				// Determine the index of the current work item
				size_t workItemIndex;
				{
					// Lock the queue to prevent stealing while working
					std::lock_guard queueGuard(environment.m_workQueueMutex[threadId]);

					// Check if our queue is empty or not
					if (environment.m_numWorkItemsLeft[queueIndex] > 0)
					{
						// Get the next work item
						workItemIndex = environment.m_numWorkItems[queueIndex] - environment.m_numWorkItemsLeft[queueIndex];

						// Decrement the work item counter
						--environment.m_numWorkItemsLeft[queueIndex];

						// Signal that we found a work item
						hasWorkLeft = true;
					}
				}

				// Try to steal work from other queues
				if (!hasWorkLeft)
				{
					// TODO: do work stealing
				}

				// Stop if we have nothing left to work on
				if (!hasWorkLeft) break;

				// Call the callback on the work item
				fn(environment, indices.m_workIndices[queueIndex][workItemIndex]);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename Fn, typename W>
		auto wrapLoopCore(ThreadedExecuteEnvironment& environment, Fn const& fn, W const& indices)
		{
			return [&](size_t threadId)
			{
				// Init the worker
				initWorker(environment, threadId, indices.m_workIndices[threadId].size());

				// Invoke the loop core function
				if (indices.m_workStealing == NoWorkStealing || indices.m_numThreads <= 1)
					loopCoreNoWorkStealing(environment, fn, indices, threadId);
				else if (indices.m_workStealing == SimpleWorkStealing)
					loopCoreSimpleWorkStealing(environment, fn, indices, threadId);

				// Release the worker
				cleanupWorker(environment, threadId);
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename EP, typename F, typename... S>
		void executeImpl(work_indices_impl::WorkIndices<S...> const& indices, ThreadedExecuteParams const& params, F const& fn, EP execPolicy)
		{
			ThreadedExecuteEnvironment environment;
			initExecution(environment, params, indices.m_numThreads, indices.m_numTotalInvocations, indices.m_indicesPerBatch);
			std::for_each(execPolicy, indices.m_threadIndices.begin(), indices.m_threadIndices.end(), wrapLoopCore(environment, fn, indices));
			cleanupExecution(environment);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename F, typename... S>
		void execute(work_indices_impl::WorkIndices<S...> const& indices, ThreadedExecuteParams const& params, F const& fn)
		{
			if (indices.m_numThreads > 1) executeImpl(indices, params, fn, std::execution::par_unseq);
			else                          executeImpl(indices, params, fn, std::execution::seq);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename F, typename... S>
		auto loopCoreIndices(F const& fn)
		{
			return [&](ThreadedExecuteEnvironment const& environment, auto const& indices)
			{
				std::apply(fn, std::tuple_cat(std::tuple<ThreadedExecuteEnvironment const&>(environment), indices));
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename F, typename... S>
		void executeIndices(ThreadedExecuteParams const& params, F const& fn, S... workItems)
		{
			// Generate thread indices
			auto indices = Threading::workIndices(params, workItems...);

			// Construct the callback fn.
			auto callback = loopCoreIndices(fn);

			// Execute the loop
			execute(indices, params, callback);
		}
		
		/*
		////////////////////////////////////////////////////////////////////////////////
		template<typename F, typename W, typename P>
		auto loopCoreIndicesParams(F const& fn, W const& indices, P const& parameters)
		{
			// TODO: implement proper parameter support
			using WorkIndicesType = typename work_indices_impl::WorkItemTypes<S...>::work_indices_type;
			return [&](W::WorkIndicesType const& indices)
			{
				std::apply(fn, std::tuple_cat(parameters, indices));
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename F, typename P, typename... S>
		void executeIndicesParams(size_t numThreads, F const& fn, P const& parameters, S... workItems)
		{
			// Generate thread indices
			auto indices = Threading::workIndices(numThreads, workItems...);

			// Construct the callback fn.
			auto callback = loopCoreIndicesParams(fn, parameters)

			// Execute the loop
			execute(numThreads, callback, indices);
		}

		/*
		////////////////////////////////////////////////////////////////////////////////
		template<typename F, typename... S>
		auto loopCoreItems(F const& fn, const S&... items)
		{
			// TODO: implement item lookup
			using WorkIndicesType = typename work_indices_impl::WorkItemTypes<S...>::work_indices_type;
			return [&](W::WorkIndicesType const& indices)
			{
				std::apply(fn, indices);
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename F, typename... S>
		void executeItems(size_t numThreads, F const& fn, S... workItems)
		{
			// Generate thread indices
			auto indices = Threading::workIndices(numThreads, std::size(workItems)...);

			// Construct the callback fn.
			auto callback = loopCoreItems(fn, workItems);

			// Execute the loop
			execute(numThreads, callback, indices);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename F, typename P, typename... S>
		auto loopCoreItemsParams(F const& fn, W const& indices, P const& parameters, const S&... items)
		{
			// TODO: implement item lookup
			// TODO: implement proper parameter support
			using WorkIndicesType = typename work_indices_impl::WorkItemTypes<S...>::work_indices_type;
			return [&](W::WorkIndicesType const& indices)
			{
				std::apply(fn, std::tuple_cat(parameters, indices));
			};
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename F, typename P, typename... S>
		void executeItemsParams(size_t numThreads, F const& fn, P const& parameters, S... workItems)
		{
			// Generate thread indices
			auto indices = Threading::workIndices(numThreads, std::size(workItems)...);

			// Construct the callback fn.
			auto callback = loopCoreItemsParams(fn, parameters, workItems);

			// Execute the loop
			execute(numThreads, callback, indices);
		}
		*/
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename F, typename... S>
	void threadedExecuteIndices(ThreadedExecuteParams const& params, F const& fn, S... workItems)
	{
		threaded_execute_impl::executeIndices(params, fn, workItems...);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename F, typename... S>
	void threadedExecuteIndices(size_t numThreads, F const& fn, S... workItems)
	{
		ThreadedExecuteParams params;
		params.m_numThreads = numThreads;
		threadedExecuteIndices(params, fn, workItems...);
	}

	/*
	////////////////////////////////////////////////////////////////////////////////
	template<typename F, typename P, typename... S>
	void threadedExecuteIndicesParams(size_t numThreads, F const& fn, P const& parameters, S... workItems)
	{
		// Perform the loop
		threaded_execute_impl::executeIndicesParams(numThreads, fn, parameters, workItems...);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename F, typename... S>
	void threadedExecuteItems(size_t numThreads, F const& fn, S... workItems)
	{
		// Perform the loop
		threaded_execute_impl::executeItems(numThreads, fn, workItems...);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename F, typename P, typename... S>
	void threadedExecuteItemsParams(size_t numThreads, F const& fn, P const& parameters, S... workItems)
	{
		// Perform the loop
		threaded_execute_impl::executeItemsParams(numThreads, fn, parameters, workItems...);
	}
	*/
}