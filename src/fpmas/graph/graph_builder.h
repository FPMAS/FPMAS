#ifndef FPMAS_GRAPH_BUILDER_H
#define FPMAS_GRAPH_BUILDER_H

#include "fpmas/api/graph/graph_builder.h"
#include "fpmas/api/random/distribution.h"

namespace fpmas { namespace graph {

	template<typename T>
		class FixedDegreeDistributionRandomGraph : public api::graph::GraphBuilder<T> {
			private:
				api::random::Generator& generator;
				api::random::Distribution<std::size_t>& distrib;

			public:
				FixedDegreeDistributionRandomGraph(
						api::random::Generator& generator,
						api::random::Distribution<std::size_t>& distrib)
					: generator(generator), distrib(distrib) {}

				void build(std::vector<T> nodes_data, api::graph::LayerId layer, api::graph::DistributedGraph<T>& graph) override;
		};

	template<typename T>
		void FixedDegreeDistributionRandomGraph<T>
			::build(std::vector<T> nodes_data, api::graph::LayerId layer, api::graph::DistributedGraph<T>& graph) {
				std::vector<api::graph::DistributedNode<T>*> built_nodes;
				for(T& data : nodes_data)
					built_nodes.push_back(graph.buildNode(std::move(data)));

				for(auto node : built_nodes) {
					std::size_t edge_count = std::min(distrib(generator), built_nodes.size()-1);

					std::vector<api::graph::DistributedNode<T>*> available_nodes = built_nodes;
					available_nodes.erase(std::remove(available_nodes.begin(), available_nodes.end(), node));

					std::shuffle(available_nodes.begin(), available_nodes.end(), generator);
					for(std::size_t i = 0; i < edge_count; i++) {
						graph.link(node, available_nodes.at(i), layer);
					}
				}
		}
}}
#endif
