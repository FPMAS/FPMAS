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
	 * Algorithm used to build a sample of generic indexes, without
	 * replacement.
	 *
	 * `Index_t` can be anything that fulfill the following requirements:
	 * - `Index_t` is
	 *   [CopyConstructible](https://en.cppreference.com/w/cpp/named_req/CopyConstructible)
	 * - Given an instance `index`, `index++` and `++index` are valid
	 *   expressions and correspond to the expected behaviors.
	 * - Given instances `i1` and `i2`, `i1!=i2` and `i1==i2` is valid.
	 * - Considering `index=begin`, successively applying `index++` necessarily
	 *   reaches a point were `index!=end` is `false` (and `index==end` is
	 *   `true`). The behavior of applying index++ when `index==end` is however
	 *   not specified.
	 *
	 * Indexes are randomly selected in the range `[begin, end)`. `n` is
	 * assumed to be less than or equal to the number of items in the `[begin,
	 * end)` range.
	 *
	 * This algorithm can be used to generate the same sample on all processes
	 * if `n` and the specified `[begin, end)` range are the same on all
	 * processes, and the specified sequential random number generator is in
	 * the same state on all processes.
	 *
	 * @param begin range start, included in the selection
	 * @param end range end, not included in the selection
	 * @param n sample size (must be the same on all processes)
	 * @param gen random number generator
	 *
	 * @return indexes sample of size `n`
	 *
	 * @tparam Index_t (automatically deduced) an abstract index type that
	 * support increment. Returned indexes are a sample of indexes selected in
	 * `[begin, end)`. Such indexes can for example be used to retrieve items
	 * in a local vector to actually build a sample from the selected indexes.
	 * `Index_t` could also represent some discrete grid coordinates for
	 * example.
	 * @tparam Generator_t (automatically deduced) type of random generator
	 *
	 * @see DistributedIndex
	 */
	template<typename Index_t, typename Generator_t>
		std::vector<Index_t> sample_indexes(
				Index_t begin, Index_t end,
				std::size_t n,
				Generator_t& gen
				) {
			// (process, local_index)
			std::vector<Index_t> result_indexes(n);

			/* Initializes the indexes reservoir */
			Index_t p = begin;
			for(std::size_t i = 0; i < n; i++) {
				result_indexes[i] = p++;
			}
			/**************************************/

			std::size_t i = n;
			while(p != end) {
				UniformIntDistribution<std::size_t> random_k(0, i);
				std::size_t k = random_k(gen);
				if(k < n)
					result_indexes[k] = p;
				++i;
				++p;
			}

			return result_indexes;
		}

	/**
	 * Represents a continuous index distributed across processes.
	 *
	 * A distributed index is represented as a `(process, local_offset)` pair.
	 * Processes are indexed from `0` to `p-1`.
	 * Each process is assigned to a given number of items `n_p`.
	 *
	 * The sequence of values iterated by the index is as all processes items
	 * were iterated in a single sequence, from process `0` to `p-1`:
	 * ```
	 * (0, 0) (0, 1) ... (0, n_0-1) (1, 0) ... (1, n_1-1) ... (p-1, 0) ... (p-1, n_(p-1)-1) (p, 0)
	 * ```
	 *
	 * The interest of this structure is that local offsets associated to a
	 * process `p` can for exemple be used as indexes in a vector hosted by the
	 * process `p`.
	 */
	class DistributedIndex {
		private:
			std::vector<std::size_t> item_counts;
		public:
			/**
			 * Process index.
			 */
			int p;
			/**
			 * Local offset.
			 */
			std::size_t offset;

			DistributedIndex() = default;

			/**
			 * DistributedIndex constructor.
			 *
			 * `item_counts` is such that processes `p` owns `item_counts[p]`
			 * items. The number of processes is set to `item_counts.size()`.
			 *
			 * @param item_counts counts of items on each process
			 * @param p process index (in `[0, item_counts.size())`)
			 * @param offset local offset (in `[0, item_counts[p])`)
			 */
			DistributedIndex(
					const std::vector<std::size_t>& item_counts,
					int p, std::size_t offset)
				: item_counts(item_counts), p(p), offset(offset) {
				}

			/**
			 * Begin of the distributed index associated to `item_counts`, i.e.
			 * `(p0, 0)` where `p0` is the first process that owns at least one
			 * item.
			 */
			static DistributedIndex begin(const std::vector<std::size_t>& item_counts);

			/**
			 * End of the distributed index associated to `item_counts`, i.e.
			 * (p, 0).
			 */
			static DistributedIndex end(const std::vector<std::size_t>& item_counts);

			/**
			 * Increments the current DistributedIndex.
			 */
			DistributedIndex& operator++();

			/**
			 * Increments the current DistributedIndex and returns the index
			 * value before the increment.
			 */
			DistributedIndex operator++(int);

			/**
			 * Returns a copy of the current DistributedIndex incremented by
			 * `n`.
			 */
			DistributedIndex operator+(std::size_t n) const;
			
			/**
			 * Returns the distance between `i1` and `i2`, i.e. the number of
			 * increments to reach `i2` from `i1`. The behavior is undefined if
			 * `i2` cannot be reached from `i1`.
			 *
			 * `distance(begin(item_counts), end(item_counts))` notably returns
			 * the total number of items owned by all processes.
			 */
			static std::size_t distance(
					const DistributedIndex& i1, const DistributedIndex& i2);

			/**
			 * Returns true iff `index` is equal to this, i.e. iff their
			 * processes and local offsets are equal.
			 *
			 * `item_counts` are not taken into account in the comparison
			 * process.
			 */
			bool operator==(const DistributedIndex& index) const;
			/**
			 * Equivalent to `!(*this==index)`.
			 */
			bool operator!=(const DistributedIndex& index) const;
	};

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

			communication::TypedMpi<std::size_t> int_mpi(comm);
			std::vector<std::size_t> item_counts
				= int_mpi.allGather(local_items.size());

			// index = (process, local_offset)
			std::vector<DistributedIndex> result_indexes
				= sample_indexes(
						DistributedIndex::begin(item_counts),
						DistributedIndex::end(item_counts),
						std::min(
							n,
							std::accumulate(item_counts.begin(), item_counts.end(), 0ul)
							), rd_choices);
			
			// Requests required items
			communication::TypedMpi<std::vector<std::size_t>> request_mpi(comm);
			std::unordered_map<int, std::vector<std::size_t>> requests;
			for(auto item : result_indexes)
				requests[item.p].push_back(item.offset);
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

			communication::TypedMpi<std::size_t> int_mpi(comm);
			std::vector<std::size_t> item_counts
				= int_mpi.allGather(local_items.size());

			// index = (process, local_offset)
			std::vector<DistributedIndex> result_indexes
				= sample_indexes(
						DistributedIndex::begin(item_counts),
						DistributedIndex::end(item_counts),
						std::min(
							n,
							std::accumulate(item_counts.begin(), item_counts.end(), 0ul)
							), gen);

			// Because gen is sequential, the same result_indexes list is built
			// on all processes
			
			std::vector<T> result;
			for(auto item : result_indexes)
				if(item.p == comm.getRank())
					result.push_back(local_items[item.offset]);
			return result;
		}
}}
#endif
