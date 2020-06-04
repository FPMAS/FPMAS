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
		public graph::base::BasicNode<DistributedId, api::graph::parallel::DistributedArc<T>>,
		public api::graph::parallel::DistributedNode<T> {
			typedef graph::base::BasicNode<DistributedId, api::graph::parallel::DistributedArc<T>> NodeBase;

		private:
			//typedef FPMAS::api::graph::parallel::synchro::Mutex<T> mutex_base;
			LocationState _state = LocationState::LOCAL;
			int location;
			T data;
			Mutex<T> _mutex;

		public:
			typedef T DataType;
			DistributedNode(const DistributedId& id)
				: NodeBase(id), _mutex(data) {
				}

			DistributedNode(const DistributedId& id, T&& data)
				: NodeBase(id), data(std::move(data)), _mutex(data){
			}

			DistributedNode(const DistributedId& id, T&& data, float weight)
				: NodeBase(id, weight), data(std::move(data)), _mutex(data){
			}

			int getLocation() const override {return location;}
			void setLocation(int location) override {this->location = location;}

			LocationState state() const override {return _state;}
			void setState(LocationState state) override {this->_state=state;}

			Mutex<T>& mutex() override {return _mutex;}
			const Mutex<T>& mutex() const override {return _mutex;}
	};
}

namespace nlohmann {

	using FPMAS::graph::parallel::DistributedNode;
	template<typename T, template<typename> class Mutex>
		struct adl_serializer<DistributedNode<T, Mutex>> {
			static DistributedNode<T, Mutex> from_json(const json& j) {
				return {
					j.at("id").get<DistributedId>(),
					std::move(j.at("data").get<T>()),
					j.at("weight").get<float>(),
				};
			}

			static void to_json(json& j, const DistributedNode<T, Mutex>& node) {
				j["id"] = node.getId();
				j["data"] = node.mutex().data();
				j["weight"] = node.getWeight();
			}
		};

}
#endif
