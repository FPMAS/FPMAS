#ifndef DISTRIBUTED_GRAPH_IMPL_H
#define DISTRIBUTED_GRAPH_IMPL_H

#include "api/graph/parallel/distributed_graph.h"

#include "graph/base/basic_graph.h"

namespace FPMAS::graph::parallel {
	
	using FPMAS::api::graph::parallel::LocationState;
	template<
		typename DistNodeImpl,
		typename DistArcImpl,
		template<typename> class LoadBalancingImpl
	>
	class BasicDistributedGraph : 
		public base::AbstractGraphBase<
			BasicDistributedGraph<DistNodeImpl, DistArcImpl, LoadBalancingImpl>,
			DistNodeImpl, DistArcImpl>,
		public api::graph::parallel::DistributedGraph<
			BasicDistributedGraph<DistNodeImpl, DistArcImpl, LoadBalancingImpl>,
			DistNodeImpl, DistArcImpl
		> {
			typedef api::graph::parallel::DistributedGraph<
				BasicDistributedGraph<DistNodeImpl, DistArcImpl, LoadBalancingImpl>,
				DistNodeImpl, DistArcImpl
			> dist_graph_base;

			using typename dist_graph_base::node_type;
			using typename dist_graph_base::node_base;
			using typename dist_graph_base::arc_type;
			using typename dist_graph_base::layer_id_type;
			using typename dist_graph_base::node_map;
			using typename dist_graph_base::partition;

			private:
			LoadBalancingImpl<node_type> loadBalancing;

			public:
			void removeNode(node_type*) {};
			void unlink(arc_type*) {};

			node_type* importNode(node_type* node){return nullptr;};
			void exportNode(node_type* node, int rank){};

			arc_type* importArc(arc_type* arc){return nullptr;};

			const node_map& getLocalNodes() const{ return this->getNodes();};
			const node_map& getDistantNodes() const{ return this->getNodes();};

			void balance() override {
				this->distribute(loadBalancing.balance(this->getNodes(), {}));
			};

			void distribute(
					partition partition) override{};

			void synchronize(){};

			template<typename... Args> node_type* _buildNode(
					const DistributedId& id, Args... args) {
				return new node_type(id, std::forward<Args>(args)...);
			}

			template<typename... Args> arc_type* _link(
					const DistributedId& id,
					node_base* src, node_base* tgt, layer_id_type layer,
					Args... args) {
				return new arc_type(
						id, src, tgt, layer,
						std::forward<Args>(args)...,
						LocationState::LOCAL
						);
			}
		};
}
#endif
