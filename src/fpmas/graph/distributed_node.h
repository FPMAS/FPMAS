#ifndef DISTRIBUTED_NODE_IMPL_H
#define DISTRIBUTED_NODE_IMPL_H

#include "fpmas/api/graph/distributed_node.h"
#include "fpmas/api/synchro/mutex.h"
#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/graph/node.h"

namespace fpmas { namespace graph {

	using api::graph::LocationState;

	template<typename T>
	class DistributedNode : 
		public graph::Node<DistributedId, api::graph::DistributedEdge<T>>,
		public api::graph::DistributedNode<T> {
			typedef graph::Node<DistributedId, api::graph::DistributedEdge<T>> NodeBase;
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
			Mutex* mutex() override {return _mutex;}
			const Mutex* mutex() const override {return _mutex;}

			~DistributedNode() {
				if(_mutex!=nullptr)
					delete _mutex;
			}
	};

	template<typename T>
	using NodePtrWrapper = api::utils::VirtualPtrWrapper<api::graph::DistributedNode<T>>;
}}

namespace nlohmann {

	template<typename T>
		using NodePtrWrapper = fpmas::graph::NodePtrWrapper<T>;

	template<typename T>
		struct adl_serializer<NodePtrWrapper<T>> {
			static NodePtrWrapper<T> from_json(const json& j) {
				return new fpmas::graph::DistributedNode<T> {
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
