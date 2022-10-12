#ifndef FPMAS_RANDOM_GRAPH_BUILDER_H
#define FPMAS_RANDOM_GRAPH_BUILDER_H

#include "fpmas/random/random.h"

/** \file src/fpmas/graph/random_graph_builder.h
 * Algorithms to build random graphs.
 */

namespace fpmas { namespace graph {
	/**
	 * A base class that can be used for graph builders based on a random
	 * number of outgoing edges for each node, according to the specified
	 * random distribution.
	 *
	 * Static random generators are also available, so that the graph
	 * generation process can be controlled with fpmas::seed().
	 */
	class RandomGraphBuilder {
		protected:
			/**
			 * Function object returning a random edge count.
			 */
			std::function<std::size_t()> num_edge;
			/**
			 * RandomGraphBuilder constructor.
			 *
			 * @tparam Generator_t random generator type (must satisfy
			 * _UniformRandomBitGenerator_)
			 * @tparam EdgeDist edge count distribution (must satisfy
			 * _RandomNumberDistribution_)
			 *
			 * @param generator random generator
			 * @param edge_distribution outgoing edge count distribution
			 */
			template<typename Generator_t, typename EdgeDist>
				RandomGraphBuilder(
						Generator_t& generator,
						EdgeDist& edge_distribution) :
					// A lambda function is used to "catch" generator end
					// edge_distribution, without requiring class level
					// template parameters. Moreover, template arguments can be
					// automatically deduced in the context of a constructor,
					// but not at the class level.
					num_edge([&generator, &edge_distribution] () {
							return edge_distribution(generator);
							}) {}
		public:
			/**
			 * Static and distributed random generator.
			 */
			static random::DistributedGenerator<> distributed_rd;
			/**
			 * Static random generator, that should produce the same random
			 * number generation sequences on all processes, provided that the
			 * seed() method is properly called with the same argument on all
			 * processes (or not called at all, to use the default seed).
			 */
			static random::mt19937_64 rd;

			/**
			 * Seeds the `distributed_rd` and `rd` random generators.
			 *
			 * @param seed random seed
			 */
			static void seed(random::mt19937_64::result_type seed);
	};
}}
#endif
