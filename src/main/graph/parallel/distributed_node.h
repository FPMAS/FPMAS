#ifndef DISTRIBUTED_NODE_IMPL_H
#define DISTRIBUTED_NODE_IMPL_H

#include "api/graph/parallel/distributed_node.h"
#include "api/graph/parallel/synchro/mutex.h"
#include "graph/base/basic_node.h"
#include "graph/parallel/distributed_arc.h"

namespace FPMAS::graph::parallel {

	using FPMAS::api::graph::parallel::LocationState;

	template<typename T, template<typename> class Mutex>
	class DistributedNode : 
		public graph::base::BasicNode<T, DistributedId, api::graph::parallel::DistributedArc<T>>,
		public api::graph::parallel::DistributedNode<T> {
			typedef graph::base::BasicNode<T, DistributedId, api::graph::parallel::DistributedArc<T>> NodeBase;

		private:
			//typedef FPMAS::api::graph::parallel::synchro::Mutex<T> mutex_base;
			LocationState _state = LocationState::LOCAL;
			int location;
			Mutex<T> _mutex;

		public:
			typedef T DataType;
			DistributedNode(const DistributedId& id)
				: NodeBase(id), _mutex(this->data()) {
				}

			DistributedNode(const DistributedId& id, T&& data)
				: NodeBase(id, std::move(data)), _mutex(this->data()){
			}

			DistributedNode(const DistributedId& id, T&& data, float weight)
				: NodeBase(id, std::move(data), weight), _mutex(this->data()){
			}

			int getLocation() const override {return location;}
			void setLocation(int location) override {this->location = location;}

			LocationState state() const override {return _state;}
			void setState(LocationState state) override {this->_state=state;}

			Mutex<T>& mutex() override {return _mutex;}
	};
}

namespace nlohmann {

	using FPMAS::graph::parallel::DistributedNode;
	template<typename T, template<typename> class Mutex>
		struct adl_serializer<DistributedNode<T, Mutex>> {
			static DistributedNode<T, Mutex> from_json(const json& j) {
				DistributedNode<T, Mutex> node {
					j.at("id").get<DistributedId>(),
					std::move(j.at("data").get<T>()),
					j.at("weight").get<float>(),
				};
				return node;
			}

			static void to_json(json& j, const DistributedNode<T, Mutex>& node) {
				j["id"] = node.getId();
				j["data"] = node.data();
				j["weight"] = node.getWeight();
			}
		};

}
#endif
