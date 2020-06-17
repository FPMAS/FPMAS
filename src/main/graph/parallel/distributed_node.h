#ifndef DISTRIBUTED_NODE_IMPL_H
#define DISTRIBUTED_NODE_IMPL_H

#include "api/graph/parallel/distributed_node.h"
#include "api/synchro/mutex.h"
#include "api/utils/ptr_wrapper.h"
#include "graph/base/node.h"

namespace FPMAS::graph::parallel {

	using FPMAS::api::graph::parallel::LocationState;

	template<typename T>
	class DistributedNode : 
		public graph::base::Node<DistributedId, api::graph::parallel::DistributedArc<T>>,
		public api::graph::parallel::DistributedNode<T> {
			typedef graph::base::Node<DistributedId, api::graph::parallel::DistributedArc<T>> NodeBase;
			typedef api::synchro::Mutex<T> Mutex;

		private:
			LocationState _state = LocationState::LOCAL;
			int location;
			T _data;
			Mutex* _mutex = nullptr;

		public:
			typedef T DataType;
			DistributedNode(const DistributedId& id)
				: NodeBase(id) {
				}

			DistributedNode(const DistributedId& id, const T& data)
				: NodeBase(id), _data(data) {
			}

			DistributedNode(const DistributedId& id, T&& data)
				: NodeBase(id), _data(std::move(data)) {
			}

			DistributedNode(const DistributedId& id, const T& data, float weight)
				: NodeBase(id, weight), _data(data) {
			}

			DistributedNode(const DistributedId& id, T&& data, float weight)
				: NodeBase(id, weight), _data(std::move(data)) {
			}

			int getLocation() const override {return location;}
			void setLocation(int location) override {this->location = location;}

			LocationState state() const override {return _state;}
			void setState(LocationState state) override {this->_state=state;}


			T& data() override {return _data;}
			const T& data() const override {return _data;}
			void setMutex(Mutex* mutex) override {_mutex=mutex;}
			Mutex& mutex() override {return *_mutex;}
			const Mutex& mutex() const override {return *_mutex;}

			~DistributedNode() {
				if(_mutex!=nullptr)
					delete _mutex;
			}
	};

	template<typename T>
	using NodePtrWrapper = FPMAS::api::utils::VirtualPtrWrapper<FPMAS::api::graph::parallel::DistributedNode<T>>;
}

namespace nlohmann {

	template<typename T>
		using NodePtrWrapper = FPMAS::graph::parallel::NodePtrWrapper<T>;

	template<typename T>
		struct adl_serializer<NodePtrWrapper<T>> {
			static NodePtrWrapper<T> from_json(const json& j) {
				return new FPMAS::graph::parallel::DistributedNode<T> {
					j.at("id").get<DistributedId>(),
					std::move(j.at("data").get<T>()),
					j.at("weight").get<float>(),
				};
			}

			static void to_json(json& j, const NodePtrWrapper<T>& node) {
				j["id"] = node->getId();
				j["data"] = node->data();
				j["weight"] = node->getWeight();
			}
		};
}
#endif
