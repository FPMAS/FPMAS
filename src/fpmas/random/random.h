#ifndef FPMAS_RANDOM_H
#define FPMAS_RANDOM_H

#include "generator.h"
#include "distribution.h"

namespace fpmas { namespace random {
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
	 * same choices are performed on all processes. If the generator is
	 * distributed, different choices are performed on each process.
	 *
	 * @param local_items local items from which random choices should be made
	 * @param n number of items to chose
	 * @param gen random number generator
	 * @return random choices
	 */
	template<typename T, typename Generator_t>
		std::vector<T> local_choices(
				const std::vector<T>& local_items, std::size_t n,
				Generator_t gen) {
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
	 * Randomly selects `n` items from **all** `local_items` specified on each
	 * processes.
	 *
	 * This method performs collective communications within the specified MPI
	 * communicator. In consequence, the method must be called from all
	 * processes.
	 *
	 * Distinct `local_items` lists of any size can be specified on each
	 * process. `n` items are returned **on each process**. Different values of
	 * `n` can be specified on each process.
	 *
	 * The elements are selected as if choices where performed locally on a
	 * list representing the concatenation of all `local_items` specified on
	 * all processes.
	 *
	 * The interest of this distributed implementation is that the complete
	 * item list is never centralized, in order to limit communications and
	 * memory usage.
	 *
	 * Results are returned in an  std::vector `v` of size `comm.getSize()`,
	 * such that `v[i]` contains items selected from process `i`.
	 *
	 * @par Tip
	 * A one dimensional vector can be generated from the result with
	 * `std::accumulate(v.begin(), v.end(), fpmas::utils::Concat())`.
	 *
	 * @param comm MPI communicator
	 * @param local_items local items provided by the current process
	 * @param n local number of items to select from all items
	 */
	template<typename T>
		std::vector<std::vector<T>> distributed_choices(
				api::communication::MpiCommunicator& comm,
				const std::vector<T>& local_items, std::size_t n) {
			static random::DistributedGenerator<> gen;

			communication::TypedMpi<std::size_t> int_mpi(comm);
			std::vector<std::size_t> item_counts = int_mpi.allGather(local_items.size());

			std::unordered_map<int, std::size_t> requests;

			random::DiscreteDistribution<std::size_t> rd_process(item_counts);
			for(std::size_t i = 0; i < n; i++)
				requests[rd_process(gen)]++;
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

}}
#endif
