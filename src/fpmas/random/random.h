#ifndef FPMAS_RANDOM_H
#define FPMAS_RANDOM_H

/** \file src/fpmas/random/random.h
 * Random number generation related features.
 */

#include "generator.h"
#include "distribution.h"
#include <numeric>

namespace fpmas { namespace random {
	/**
	 * The default seed passed to fpmas::seed() when fpmas::init() is called.
	 */
	extern const std::mt19937::result_type default_seed;

	/**
	 * The seed currently in use by FPMAS.
	 *
	 * Must be initialized by fpmas::seed(), otherwise #default_seed is used.
	 */
	extern fpmas::random::mt19937::result_type seed;

	/**
	 * A predefined distributed random number generator used by choice and
	 * sampling algorithms.
	 */
	extern DistributedGenerator<> rd_choices;

	/**
	 * Randomly selects `n` elements from the `local_items`, with replacement.
	 * In consequence, `n` can be greater than the number of elements of
	 * `local_items`.
	 *
	 * A case of interest occurs when the `local_items` are the same on all
	 * processes. If the provided random number generator is sequential, the
	 * same choices are performed on all processes (provided that generators
	 * are in the same state on each process). If the generator is distributed,
	 * different choices are performed on each process.
	 *
	 * @param local_items local items from which random choices should be made
	 * @param n number of items to chose
	 * @param gen random number generator
	 * @return random choices
	 */
	template<typename T, typename Generator_t>
		std::vector<T> local_choices(
				const std::vector<T>& local_items, std::size_t n,
				Generator_t& gen) {
			random::UniformIntDistribution<std::size_t> local_random_item(
					0, local_items.size()-1);
			std::vector<T> random_items(n);
			for(std::size_t i = 0; i < n; i++)
				random_items[i] = local_items[local_random_item(gen)];
			return random_items;
		}

	/**
	 * Same as local_choices(const std::vector<T>&, std::size_t, Generator_t),
	 * but uses the predefined #rd_choices distributed generator instead.
	 *
	 * Different choices are performed on all processes, even if the
	 * local_items are the same.
	 *
	 * The #rd_choices instance can be seeded with the fpmas::seed() method.
	 */
	template<typename T>
		std::vector<T> local_choices(
				const std::vector<T>& local_items, std::size_t n) {
			return local_choices(local_items, n, rd_choices);
		}

	/**
	 * Randomly selects `n` items from **all** `local_items` specified on all
	 * processes, with replacement.
	 *
	 * A set of `n` choices is built **independently** on each process.
	 *
	 * Distinct `local_items` lists of any size can be specified on each
	 * process. `n` items are returned independently **on each process**, so
	 * different values of `n` can be specified on each process.
	 *
	 * The elements are selected as if choices where performed locally on a
	 * list representing the concatenation of all `local_items` specified on
	 * all processes.
	 *
	 * This method performs collective communications within the specified MPI
	 * communicator. In consequence, the method must be called from all
	 * processes.
	 *
	 * The complete item list is never centralized, in order to limit
	 * communications and memory usage.
	 *
	 * Results are returned in an  std::vector `v` of size `comm.getSize()`,
	 * such that `v[i]` contains items selected from process `i`.
	 *
	 * @par Tip
	 * A one dimensional vector can be generated from the result with
	 * `std::vector<T> choices = std::accumulate(v.begin(), v.end(), std::vector<T>(), fpmas::utils::Concat())`.
	 *
	 * @see split_choices
	 *
	 * @param comm MPI communicator
	 * @param local_items local items provided by the current process
	 * @param n local number of items to select from all items (might be
	 * different on all processes)
	 *
	 * @return a choices list of size `n`, selected from all the processes.
	 */
	template<typename T>
		std::vector<std::vector<T>> distributed_choices(
				api::communication::MpiCommunicator& comm,
				const std::vector<T>& local_items, std::size_t n) {
			communication::TypedMpi<std::size_t> int_mpi(comm);
			std::vector<std::size_t> item_counts
				= int_mpi.allGather(local_items.size());

			std::unordered_map<int, std::size_t> requests;

			random::DiscreteDistribution<std::size_t> rd_process(item_counts);
			for(std::size_t i = 0; i < n; i++)
				requests[rd_process(rd_choices)]++;
			requests = int_mpi.allToAll(requests);

			std::unordered_map<int, std::vector<T>> random_items;
			for(auto item : requests)
				random_items[item.first]
					= local_choices(local_items, item.second, rd_choices);

			communication::TypedMpi<std::vector<T>> item_mpi(comm);

			std::vector<std::vector<T>> results(comm.getSize());
			for(auto item : item_mpi.allToAll(random_items))
				results[item.first] = item.second;

			return results;
		}

	/**
	 * Randomly selects `n` items from **all** `local_items` specified on all
	 * processes, with replacement.
	 *
	 * Contrary to distributed_choices(), a **single** choices list of size `n`
	 * is built across processes. Return values correspond to the subset of
	 * values selected on the current process, so that all returned items are
	 * contained in `local_items` and `n` items are chosen in total.
	 *
	 * In consequence, the specified `n` must be the **same** on **all
	 * processes**.
	 *
	 * This method performs collective communications within the specified MPI
	 * communicator. In consequence, the method must be called from all
	 * processes.
	 *
	 * The complete item list is never centralized, in order to limit
	 * communications and memory usage.
	 *
	 * @param comm MPI communicator
	 * @param local_items local items provided by the current process
	 * @param n local number of items to select from all items (must be the
	 * same on all processes)
	 *
	 * @return selected local items, so that the concatenation of results from
	 * all processes is a valid choices list of size `n` selected from all
	 * `local_items`.
	 */
	template<typename T>
		std::vector<T> split_choices(
				api::communication::MpiCommunicator& comm,
				const std::vector<T>& local_items, std::size_t n) {
			// A deterministic generator that produces the same sequence on all
			// processes
			static mt19937_64 gen(seed);

			communication::TypedMpi<std::size_t> int_mpi(comm);
			std::vector<std::size_t> item_counts
				= int_mpi.allGather(local_items.size());

			random::DiscreteDistribution<std::size_t> rd_process(item_counts);

			std::unordered_map<int, std::size_t> requests;

			// Determines how many items will be selected from this process, so
			// that the sum of local_items_count on all processes is equal to
			// n.
			std::size_t local_items_count = 0;
			for(std::size_t i = 0; i < n; i++) {
				// Same result on all processes
				int process = rd_process(gen);
				if(process == comm.getRank())
					local_items_count++;
			}

			return local_choices(local_items, local_items_count);
		}
	/**
	 * Randomly selects `n` elements from the `local_items`, without
	 * replacement. If `n` is greater that the number of elements of
	 * `local_items`, all items are returned (and the size of the sample is
	 * less than `n`).
	 *
	 * A case of interest occurs when the `local_items` are the same on all
	 * processes. If the provided random number generator is sequential, the
	 * same sample is built on all processes (provided that generators are in
	 * the same state on each process). If the generator is distributed,
	 * different samples are built on each process.
	 *
	 * @param local_items local items from which random choices should be made
	 * @param n number of items to chose
	 * @param gen random number generator
	 * @return random choices
	 */
	template<typename T, typename Generator_t>
		std::vector<T> local_sample(
				const std::vector<T>& local_items, std::size_t n,
				Generator_t& gen) {
			std::vector<T> sample(std::min(local_items.size(), n));

			for(std::size_t i = 0; i < sample.size(); i++)
				sample[i] = local_items[i];


			for(std::size_t i = sample.size(); i < local_items.size(); i++) {
				UniformIntDistribution<std::size_t> rd_index(0, i);
				std::size_t k = rd_index(gen);
				if(k < sample.size())
					sample[k] = local_items[i];
			}
			return sample;
		}

	/**
	 * Same as local_sample(const std::vector<T>&, std::size_t, Generator_t),
	 * but uses the predefined #rd_choices distributed generator instead.
	 *
	 * Different samples are built on all processes, even if the
	 * local_items are the same.
	 *
	 * The #rd_choices instance can be seeded with the fpmas::seed() method.
	 */
	template<typename T>
		std::vector<T> local_sample(
				const std::vector<T>& local_items, std::size_t n
				) {
			return local_sample(local_items, n, rd_choices);
		}

	/**
	 * Distributed algorithms used to build a sample of indexes, without
	 * replacement.
	 *
	 * Returned indexes are pairs of `(process_rank, local_offset)`, where
	 * `local_offset` is included in `[0, local_items_count)` for each process,
	 * so that the local offset can be used as an index in a local vector for
	 * example.
	 *
	 * Indexes are randomly selected from all the possible `(process_rank,
	 * local_offset)` pairs, depending on the MPI communicator size and the
	 * provided local item counts. If `n` is greater than the number of
	 * possible indexes (i.e. the sum of all local item counts), all indexes
	 * are returned.
	 *
	 * This method performs collective communications within the specified MPI
	 * communicator. In consequence, the method must be called from all
	 * processes.
	 *
	 * The same sample is generated on all processes if `n` is the same on all
	 * processes and the specified random number generator is in the same state
	 * on all processes.
	 *
	 * @param comm MPI communicator
	 * @param local_items_count number of items available in the current
	 * process
	 * @param n sample size (must be the same on all processes)
	 * @param gen random number generator
	 *
	 * @return indexes sample of size `n`
	 */
	template<typename Generator_t>
	std::vector<std::pair<int, std::size_t>> sample_indexes(
			api::communication::MpiCommunicator& comm,
			std::size_t local_items_count, std::size_t n,
			Generator_t& gen
			) {
		communication::TypedMpi<std::size_t> int_mpi(comm);
		// Item count on each process
		std::vector<std::size_t> item_counts
			= int_mpi.allGather(local_items_count);
		std::size_t total_item_count = std::accumulate(
				item_counts.begin(), item_counts.end(), 0);
		n = std::min(n, total_item_count);

		// (process, local_index)
		std::vector<std::pair<int, std::size_t>> result_indexes(n);

		/* Initializes the indexes reservoir */
		int p = 0;
		std::size_t local_index = 0;
		for(std::size_t i = 0; i < n; i++) {
			if(local_index >= item_counts[p]) {
				p++;
				local_index = 0;
			}
			result_indexes[i] = {p, local_index++};
		}
		/**************************************/

		std::size_t i = n;
		while(p != (int) item_counts.size()) {
			while(local_index != item_counts[p]) {
				std::pair<int, std::size_t> index(p, local_index++);
				UniformIntDistribution<std::size_t> random_k(0, i);
				std::size_t k = random_k(gen);
				if(k < n)
					result_indexes[k] = index;
				++i;
			}
			p++;
			local_index=0;
		}

		return result_indexes;
	}

	/**
	 * Randomly selects `n` items from **all** `local_items` specified on all
	 * processes, without replacement.
	 *
	 * A sample of size `n` is built **independently** on each process.
	 *
	 * Distinct `local_items` lists of any size can be specified on each
	 * process. `n` items are returned independently **on each process**, so
	 * different values of `n` can be specified on each process.
	 *
	 * The elements are selected as if the sample was built locally from a list
	 * representing the concatenation of all `local_items` specified on all
	 * processes.
	 *
	 * This method performs collective communications within the specified MPI
	 * communicator. In consequence, the method must be called from all
	 * processes.
	 *
	 * The complete item list is never centralized, in order to limit
	 * communications and memory usage.
	 *
	 * Results are returned in an  std::vector `v` of size `comm.getSize()`,
	 * such that `v[i]` contains items selected from process `i`.
	 *
	 * @par Tip
	 * A one dimensional vector can be generated from the result with
	 * `std::vector<T> sample = std::accumulate(v.begin(), v.end(), std::vector<T>(), fpmas::utils::Concat())`.
	 *
	 * @see split_sample
	 *
	 * @param comm MPI communicator
	 * @param local_items local items provided by the current process
	 * @param n local number of items to select from all items (might be
	 * different on all processes)
	 *
	 * @return a sample of size `n`, selected from all the processes.
	 */
	template<typename T>
		std::vector<std::vector<T>> distributed_sample(
				api::communication::MpiCommunicator &comm,
				const std::vector<T> &local_items, std::size_t n
				) {
			// index = (process, local_offset)
			std::vector<std::pair<int, std::size_t>> result_indexes
				= sample_indexes(comm, local_items.size(), n, rd_choices);
			
			// Requests required items
			communication::TypedMpi<std::vector<std::size_t>> request_mpi(comm);
			std::unordered_map<int, std::vector<std::size_t>> requests;
			for(auto item : result_indexes)
				requests[item.first].push_back(item.second);
			requests = request_mpi.allToAll(requests);

			// Exchange items
			communication::TypedMpi<std::vector<T>> item_mpi(comm);
			std::unordered_map<int, std::vector<T>> items;
			for(auto item : requests)
				for(auto index : item.second)
					items[item.first].push_back(local_items[index]);
			items = item_mpi.allToAll(items);

			// Result transformation
			std::vector<std::vector<T>> results(comm.getSize());
			for(auto item : items)
				results[item.first] = item.second;
			return results;
		}

	/**
	 * Randomly selects `n` items from **all** `local_items` specified on all
	 * processes, without replacement.
	 *
	 * Contrary to distributed_sample(), a **single** sample of size `n` is
	 * built across processes. Returned values correspond to the subset of
	 * values selected on the current process, so that all returned items are
	 * contained in `local_items` and `n` items are chosen in total.
	 *
	 * In consequence, the specified `n` must be the **same** on **all
	 * processes**. If `n` is greater than the total count of items on all
	 * processes, all items are selected, so all items from the `local_items`
	 * list are returned on each process in this context.
	 *
	 * This can be useful in the \Agent context to build a global agent sample:
	 * ```cpp
	 * AgentGroup group = model.buildGroup(...);
	 *
	 * ...
	 *
	 * // Selects 10 agents from ALL the processes
	 * std::vector<fpmas::api::model::Agent*> selected_agents
	 * 	= fpmas::random::split_sample(group.localAgents(), 10);
	 *
	 * // We don't now how many agents are selected on this process, but it is
	 * // guaranteed that selected_agents are LOCAL, so this can be used from
	 * // all processes to safely, transparently and globally initialize the
	 * // randomly selected agents.
	 * for(auto agent : selected_agents)
	 * 	((VirusAgent*) agent)->setState(INFECTED);
	 * ```
	 *
	 * This method performs collective communications within the specified MPI
	 * communicator. In consequence, the method must be called from all
	 * processes.
	 *
	 * The complete item list is never centralized, in order to limit
	 * communications and memory usage.
	 *
	 * @param comm MPI communicator
	 * @param local_items local items provided by the current process
	 * @param n number of items to select from all items (must be the same on
	 * all processes)
	 *
	 * @return selected local items, so that the concatenation of results from
	 * all processes is a valid sample of size `n` selected from all
	 * `local_items`.
	 */
	template<typename T>
		std::vector<T> split_sample(
				api::communication::MpiCommunicator &comm,
				const std::vector<T> &local_items, std::size_t n
				) {
			// A deterministic generator that produces the same sequence on all
			// processes
			static mt19937_64 gen(seed);

			// index = (process, local_offset)
			std::vector<std::pair<int, std::size_t>> result_indexes
				= sample_indexes(comm, local_items.size(), n, gen);
			// Because gen is sequential, the same result_indexes list is built
			// on all processes
			
			std::vector<T> result;
			for(auto item : result_indexes)
				if(item.first == comm.getRank())
					result.push_back(local_items[item.second]);
			return result;
		}
}}
#endif
