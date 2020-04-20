#ifndef DISTRIBUTED_NODE_H
#define DISTRIBUTED_NODE_H

#include "api/graph/parallel/distributed_node.h"
#include "graph/base/basic_node.h"
#include "graph/parallel/distributed_arc.h"

namespace FPMAS::graph::parallel {

	using FPMAS::api::graph::parallel::LocationState;
	template<typename T>
	class DistributedNode : 
		public graph::base::BasicNode<T, DistributedId, DistributedArc<T>>,
		public api::graph::parallel::DistributedNode<T, DistributedArc<T>> {
			typedef graph::base::BasicNode<T, DistributedId, DistributedArc<T>> node_base;

		private:
			LocationState _state = LocationState::LOCAL;
			int location;

		public:
			DistributedNode(const DistributedId& id)
				: node_base(id) {
				}

			DistributedNode(const DistributedId& id, T&& data)
				: node_base(id, std::move(data)) {
			}

			DistributedNode(const DistributedId& id, T&& data, float weight)
				: node_base(id, std::move(data), weight) {
			}

			int getLocation() const override {return location;}
			void setLocation(int location) override {this->location = location;}

			LocationState state() const override {return _state;}
			void setState(LocationState state) override {this->_state=state;}

	};
}

namespace nlohmann {

	using FPMAS::graph::parallel::DistributedNode;
	template<typename T>
		struct adl_serializer<DistributedNode<T>> {
			static DistributedNode<T> from_json(const json& j) {
				DistributedNode<T> node {
					j.at("id").get<DistributedId>(),
					std::move(j.at("data").get<T>()),
					j.at("weight").get<float>(),
				};
				return node;
			}

			static void to_json(json& j, const DistributedNode<T>& node) {
				j["id"] = node.getId();
				j["data"] = node.data();
				j["weight"] = node.getWeight();
			}
		};

}
#endif
