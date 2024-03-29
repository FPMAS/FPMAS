#ifndef FPMAS_DISTRIBUTED_NODE_H
#define FPMAS_DISTRIBUTED_NODE_H

/** \file src/fpmas/graph/distributed_node.h
 * DistributedNode implementation.
 */

#include "fpmas/api/graph/distributed_node.h"
#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/graph/node.h"
#include "fpmas/io/json.h"
#include "fpmas/io/datapack.h"

namespace fpmas { namespace graph {

	using api::graph::LocationState;
	using api::graph::DistributedId;

	/**
	 * api::graph::DistributedNode implementation.
	 */
	template<typename T>
	class DistributedNode :
		public Node<DistributedId, api::graph::DistributedEdge<T>>,
		public api::graph::DistributedNode<T> {
			typedef graph::Node<DistributedId, api::graph::DistributedEdge<T>> NodeBase;
			typedef api::synchro::Mutex<T> Mutex;

		private:
			LocationState _state = LocationState::LOCAL;
			int _location;
			T _data;
			Mutex* _mutex = nullptr;

		public:
			/**
			 * DistributedNode constructor.
			 *
			 * `data` is _copied_ into the DistributedNode.
			 *
			 * @param id node id
			 * @param data lvalue reference data to copy to the node
			 */
			DistributedNode(const DistributedId& id, const T& data)
				: NodeBase(id), _data(data) {
			}

			/**
			 * DistributedNode constructor.
			 *
			 * `data` is _moved_ into the DistributedNode.
			 *
			 * @param id node id
			 * @param data rvalue reference to data to move into the node
			 */
			DistributedNode(const DistributedId& id, T&& data)
				: NodeBase(id), _data(std::move(data)) {
			}

			int location() const override {return _location;}
			void setLocation(int location) override {this->_location = location;}

			api::graph::LocationState state() const override {return _state;}
			void setState(api::graph::LocationState state) override {this->_state=state;}


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

	/**
	 * Alias for a DistributedNode PtrWrapper.
	 */
	template<typename T>
	using NodePtrWrapper = api::utils::PtrWrapper<api::graph::DistributedNode<T>>;
}}

namespace nlohmann {
	using fpmas::graph::NodePtrWrapper;

	/**
	 * \anchor adl_serializer_DistributedNode
	 *
	 * DistributedNode JSON serialization.
	 */
	template<typename T>
		struct adl_serializer<NodePtrWrapper<T>> {
			/**
			 * \anchor adl_serializer_DistributedNode_to_json
			 *
			 * DistributedNode json serialization.
			 *
			 * @param j json
			 * @param node wrapper of pointer to the DistributedNode to serialize
			 */
			static void to_json(json& j, const NodePtrWrapper<T>& node) {
				j["id"] = node->getId();
				j["data"] = node->data();
				j["weight"] = node->getWeight();
			}

			/**
			 * \anchor adl_serializer_DistributedNode_from_json
			 *
			 * DistributedNode json unserialization.
			 *
			 * @param j json
			 * @return wrapped heap allocated unserialized fpmas::graph::DistributedNode
			 */
			static NodePtrWrapper<T> from_json(const json& j) {
				auto node = new fpmas::graph::DistributedNode<T> {
					j.at("id").get<DistributedId>(),
					std::move(j.at("data").get<T>())
				};
				node->setWeight(j.at("weight").get<float>());
				return node;
			}
		};
}

namespace fpmas { namespace io {
	namespace json {
		using fpmas::graph::NodePtrWrapper;

		/**
		 * \anchor light_serializer_DistributedNode
		 *
		 * light_serializer specialization for \DistributedNode.
		 *
		 * Only the `id` of the node is considered for serialization, and also the node
		 * `data`, that is itself serialized using the light_serializer.
		 *
		 * @tparam T type of data contained in the \DistributedNode
		 */
		template<typename T>
			struct light_serializer<NodePtrWrapper<T>> {
				/**
				 * \anchor light_serializer_DistributedNode_to_json
				 *
				 * Minimalist serialization rules for a \DistributedNode. The
				 * `weight` is notably ignored, and `data` is serialized using the
				 * `light_serializer<T>` specialization.
				 *
				 * @param j light json output
				 * @param node distributed node to serialize
				 */
				static void to_json(light_json& j, const NodePtrWrapper<T>& node) {
					j["id"] = node->getId();
					j["data"] = node->data();
				}
				/**
				 * \anchor light_serializer_DistributedNode_from_json
				 *
				 * Minimalist unserialization rules for a \DistributedNode. The
				 * returned \DistributedNode is dynamically allocated and
				 * initialized with an `id`, and its `data` is initialized with the
				 * value produced by `light_serializer<T>::%from_json()`. Any other
				 * field is default initialized (including the `weight` of the
				 * node).
				 *
				 * @param j input json
				 * @param node_ptr distributed node output
				 */
				static void from_json(const light_json& j, NodePtrWrapper<T>& node_ptr) {
					node_ptr = {new fpmas::graph::DistributedNode<T>(
							j["id"].get<fpmas::graph::DistributedId>(),
							j["data"].get<T>()
							)};
				}
			};
	}

	namespace datapack {
		using fpmas::graph::NodePtrWrapper;

		/**
		 * Regular DistributedNode Serializer specialization.
		 *
		 * | Serialization scheme |||
		 * |----------------------|||
		 * | node->getId() | node->data() | node->getWeight() |
		 */
		template<typename T>
			struct Serializer<NodePtrWrapper<T>> {
				/**
				 * Returns the buffer size required to serialize `node` to `p`.
				 */
				static std::size_t size(
						const ObjectPack& p, const NodePtrWrapper<T>& node) {
					return p.size<DistributedId>() + p.size(node->data())
						+ p.size<float>();
				}
				/**
				 * DistributedNode ObjectPack serialization.
				 *
				 * @param pack destination pack
				 * @param node node to serialize
				 */
				static void to_datapack(ObjectPack& pack, const NodePtrWrapper<T>& node) {
					pack.put(node->getId());
					pack.put(node->data());
					pack.put(node->getWeight());
				}

				/**
				 * DistributedNode ObjectPack deserialization.
				 *
				 * @param pack source pack
				 * @return deserialized and dynamically allocated node
				 */
				static NodePtrWrapper<T> from_datapack(const ObjectPack& pack) {
					DistributedId id = pack.get<DistributedId>();
					T data = pack.get<T>();
					auto node = new fpmas::graph::DistributedNode<T>(
							std::move(id), std::move(data)
							);
					node->setWeight(pack.get<float>());
					return node;
				}
			};

		/**
		 * DistributedNode LightSerializer specialization, notably used to
		 * efficiently transmit DistributedEdge instances, where source and
		 * target node data are not required.
		 *
		 * Only the id of the node is serialized, and also the light version of
		 * its data. The light serialization of the node data can eventually
		 * produce an empty BasicObjectPack, so that only the node id is
		 * serialized.
		 *
		 * | Serialization scheme ||
		 * |----------------------||
		 * | node->getId() | node->data() |
		 */
		template<typename T>
			struct LightSerializer<NodePtrWrapper<T>> {
				/**
				 * Returns the buffer size required to serialize `node` into
				 * `p`, i.e. `p.size<DistributedId>() + p.size(node->data())`.
				 *
				 * Notice that in this light version, `p.size(node->data())`
				 * might be null.
				 */
				static std::size_t size(
						const LightObjectPack& p, const NodePtrWrapper<T>& node) {
					return p.size<DistributedId>() + p.size(node->data());
				}

				/**
				 * DistributedNode LightObjectPack serialization.
				 *
				 * @param pack destination LightObjectPack
				 * @param node node to serialize
				 */
				static void to_datapack(LightObjectPack& pack, const NodePtrWrapper<T>& node) {
					pack.put(node->getId());
					pack.put(node->data());
				}

				/**
				 * DistributedNode LightObjectPack deserialization.
				 *
				 * @param pack source LightObjectPack
				 * @return deserialized and dynamically allocated node
				 */
				static NodePtrWrapper<T> from_datapack(const LightObjectPack& pack) {
					// Call order guaranteed, DO NOT CALL gets FROM THE CONSTRUCTOR
					DistributedId id = pack.get<DistributedId>();
					T data = pack.get<T>();
					auto node = new fpmas::graph::DistributedNode<T>(
							std::move(id), std::move(data)
							);
					return node;
				}
			};
	}
}}
#endif
