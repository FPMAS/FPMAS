#ifndef FPMAS_DISTRIBUTED_EDGE_H
#define FPMAS_DISTRIBUTED_EDGE_H

/** \file src/fpmas/graph/distributed_edge.h
 * DistributedEdge implementation.
 */

#include "fpmas/graph/edge.h"
#include "fpmas/graph/distributed_node.h"
#include "fpmas/communication/communication.h"

namespace fpmas { namespace graph {

	using api::graph::LayerId;

	/**
	 * api::graph::DistributedEdge implementation.
	 */
	template<typename T>
		class DistributedEdge :
			public graph::Edge<DistributedId, api::graph::DistributedNode<T>>,
			public api::graph::DistributedEdge<T> {
				typedef graph::Edge<DistributedId, api::graph::DistributedNode<T>>
					EdgeBase;

				private:
					LocationState _state = LocationState::LOCAL;
					std::unique_ptr<api::graph::TemporaryNode<T>> temp_src;
					std::unique_ptr<api::graph::TemporaryNode<T>> temp_tgt;

				public:
					using typename graph::Edge<DistributedId, api::graph::DistributedNode<T>>::IdType;

					/**
					 * DistributedEdge constructor.
					 *
					 * @param id edge id
					 * @param layer id of the layer on which the edge is located
					 */
					DistributedEdge(DistributedId id, LayerId layer)
						: EdgeBase(id, layer) {}

					api::graph::LocationState state() const override {return _state;}
					void setState(api::graph::LocationState state) override {this->_state = state;}

					/**
					 * \copydoc fpmas::api::graph::DistributedEdge<T>::setTempSourceNode(std::unique_ptr<api::graph::TemporaryNode<T>>)
					 */
					void setTempSourceNode(
							std::unique_ptr<api::graph::TemporaryNode<T>> temp_src
							) override {
						this->temp_src = std::move(temp_src);
					}

					std::unique_ptr<api::graph::TemporaryNode<T>>
						getTempSourceNode() override {
						return std::move(temp_src);
					}

					/**
					 * \copydoc fpmas::api::graph::DistributedEdge<T>::setTempTargetNode(std::unique_ptr<api::graph::TemporaryNode<T>>)
					 */
					void setTempTargetNode(
							std::unique_ptr<api::graph::TemporaryNode<T>> temp_tgt
							) override {
						this->temp_tgt = std::move(temp_tgt);
					}

					std::unique_ptr<api::graph::TemporaryNode<T>>
						getTempTargetNode() override {
						return std::move(temp_tgt);
					}

			};


	/**
	 * ObjectPack based api::graph::TemporaryNode implementation.
	 *
	 * @tparam T node data type
	 * @tparam PackType ObjectPack specialization
	 */
	template<typename T, typename PackType>
		class TemporaryNode : public api::graph::TemporaryNode<T> {
			private:
				PackType pack;
				DistributedId id;
				int location;

			public:
				/**
				 * TemporaryNode constructor.
				 *
				 * The specified PackType is
				 * [forwarded](https://en.cppreference.com/w/cpp/utility/forward)
				 * to the internal PackType to prevent useless copies.
				 *
				 * @param id node id
				 * @param location node location rank
				 * @param p PackType representing the node
				 */
				template<typename _PackType>
					TemporaryNode(DistributedId id, int location, _PackType&& p) :
						pack(std::forward<_PackType>(p)), id(id), location(location) {
						}

				DistributedId getId() const override {
					return id;
				}

				int getLocation() const override {
					return location;
				}

				api::graph::DistributedNode<T>* build() override {
					NodePtrWrapper<T> node = pack.template get<NodePtrWrapper<T>>();
					node->setLocation(location);
					return node;
				}
		};

	/**
	 * Alias for a DistributedEdge PtrWrapper
	 */
	template<typename T>
	using EdgePtrWrapper = api::utils::PtrWrapper<api::graph::DistributedEdge<T>>;
}}

namespace nlohmann {
	using fpmas::graph::EdgePtrWrapper;

	/**
	 * \anchor adl_serializer_DistributedEdge
	 *
	 * DistributedEdge JSON serialization.
	 */
	template<typename T>
		struct adl_serializer<EdgePtrWrapper<T>> {
			/**
			 * DistributedEdge json serialization.
			 *
			 * Source and target \DistributedNode are unserialized depending on
			 * the provided `JsonType`, that might be the classic
			 * `nlohmann::json` type or fpmas::io::json::light_json.
			 *
			 * @see \ref adl_serializer_DistributedNode_to_json
			 * "adl_serializer<NodePtrWrapper<T>>::to_json()"
			 * @see \ref light_serializer_DistributedNode_to_json
			 * "light_serializer<NodePtrWrapper<T>>::to_json()"
			 *
			 * @param j json
			 * @param edge wrapper of pointer to the DistributedEdge to serialize
			 */
			template<typename JsonType>
			static void to_json(JsonType& j, const EdgePtrWrapper<T>& edge) {
				j["id"] = edge->getId();
				j["layer"] = edge->getLayer();
				j["weight"] = edge->getWeight();
				j["src"] = {{}, edge->getSourceNode()->location()};
				NodePtrWrapper<T> src(edge->getSourceNode());
				j["src"][0] = src;
				j["tgt"] = {{}, edge->getTargetNode()->location()};
				NodePtrWrapper<T> tgt(edge->getTargetNode());
				j["tgt"][0] = tgt;
			}

			/**
			 * DistributedEdge json unserialization.
			 *
			 * Source and target \DistributedNode are unserialized depending on
			 * the provided `JsonType`, that might be the classic
			 * `nlohmann::json` type or fpmas::io::json::light_json.
			 *
			 * @see \ref adl_serializer_DistributedNode_from_json
			 * "nlohmann::adl_serializer<NodePtrWrapper<T>>::from_json()"
			 * @see \ref light_serializer_DistributedNode_from_json
			 * "light_serializer<NodePtrWrapper<T>>::from_json()"
			 *
			 * @param j json
			 * @param edge_ptr output edge
			 */
			template<typename JsonType>
			static void from_json(const JsonType& j, EdgePtrWrapper<T>& edge_ptr) {
				fpmas::api::graph::DistributedEdge<T>* edge
					= new fpmas::graph::DistributedEdge<T> {
					j.at("id").template get<DistributedId>(),
					j.at("layer").template get<typename fpmas::graph::LayerId>()
				};
				edge->setWeight(j.at("weight").template get<float>());

				edge->setTempSourceNode(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
						new fpmas::graph::TemporaryNode<T, JsonType>(
							j.at("src")[0].at("id").template get<DistributedId>(),
							j.at("src")[1].template get<int>(),
							std::move(j.at("src")[0])
							)
						));

				edge->setTempTargetNode(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
						new fpmas::graph::TemporaryNode<T, JsonType>(
							j.at("tgt")[0].at("id").template get<DistributedId>(),
							j.at("tgt")[1].template get<int>(),
							std::move(j.at("tgt")[0])
							)
						));

				edge_ptr = {edge};
			}
		};
}

namespace fpmas { namespace io { namespace datapack {
	using fpmas::graph::EdgePtrWrapper;

	/**
	 * DistributedEdge ObjectPack serialization.
	 *
	 * | Serialization scheme ||||||||||
	 * |----------------------||||||||||
	 * | edge->getId() | edge->getLayer() | edge->getWeight() | source_node->getId() | source_node->location() | source_node | target_node->getId() | target_node->location() | target_node |
	 *
	 * The source and target node serialization process depends on the current
	 * `PackType`. For example, if the edge is serialized in a
	 * `LightObjectPack`, the LightSerializer<NodePtrWrapper<T>> specialization
	 * is used to serialize source and target nodes.
	 *
	 * @tparam T node data type
	 */
	template<typename T>
		struct Serializer<EdgePtrWrapper<T>> {
			/**
			 * Returns the buffer size required to serialize `edge` into `p`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p, const EdgePtrWrapper<T>& edge) {
					return p.template size<DistributedId>() + // edge id
						p.template size<int>() + // layer
						p.template size<float>() + // edge weight
						p.template size<DistributedId>() + // src id
						p.template size<int>() + // src location
						p.template size<std::size_t>() + // src pack size
						p.template size(NodePtrWrapper<T>(edge->getSourceNode())) + // src pack
						p.template size<DistributedId>() + // tgt id
						p.template size<int>() + // tgt location
						p.template size<std::size_t>() + // tgt pack size
						p.template size(NodePtrWrapper<T>(edge->getTargetNode())); // tgt pack
				}

			/**
			 * DistributedEdge ObjectPack serialization.
			 *
			 * @param pack destination pack
			 * @param edge edge to serialize
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const EdgePtrWrapper<T>& edge) {
					pack.put(edge->getId());
					pack.put(edge->getLayer());
					pack.put(edge->getWeight());
					pack.put(edge->getSourceNode()->getId());
					pack.put(edge->getSourceNode()->location());
					{
						PackType src_pack(NodePtrWrapper<T>(edge->getSourceNode()));
						pack.put(src_pack);
					}
					pack.put(edge->getTargetNode()->getId());
					pack.put(edge->getTargetNode()->location());
					{
						PackType tgt_pack(NodePtrWrapper<T>(edge->getTargetNode()));
						pack.put(tgt_pack);
					}
				}

			/**
			 * DistributedEdge ObjectPack deserialization.
			 *
			 * @param pack source pack
			 * @return deserialized and dynamically allocated edge
			 */
			template<typename PackType>
				static EdgePtrWrapper<T> from_datapack(const PackType& pack) {
					fpmas::api::graph::DistributedEdge<T>* edge
						= new fpmas::graph::DistributedEdge<T>(
								pack.template get<DistributedId>(),
								pack.template get<int>()
								);
					edge->setWeight(pack.template get<float>());

					edge->setTempSourceNode(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
								new fpmas::graph::TemporaryNode<T, PackType>(
									pack.template get<DistributedId>(),
									pack.template get<int>(),
									pack.extract(pack.template get<std::size_t>())
									)
								));

					edge->setTempTargetNode(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
								new fpmas::graph::TemporaryNode<T, PackType>(
									pack.template get<DistributedId>(),
									pack.template get<int>(),
									pack.extract(pack.template get<std::size_t>())
									)
								));

					return {edge};
				}
		};
}}}

namespace fpmas { namespace communication {

	/**
	 * \anchor TypedMpiDistributedEdge
	 *
	 * \DistributedEdge TypedMpi specialization, used to bypass the default
	 * fpmas::communication::TypedMpi specialization based on
	 * fpmas::io::datapack::Serializer.
	 *
	 * Here, fpmas::io::datapack::LightObjectPack is used instead. This allows
	 * to use "light" serialization rules of source and target
	 * \DistributedNodes, since sending their data is not required when
	 * transmitting \DistributedEdges.
	 *
	 * @tparam T type of data contained in associated \DistributedNodes
	 */
	template<typename T>
		struct TypedMpi<fpmas::graph::EdgePtrWrapper<T>> :
		public detail::TypedMpi<fpmas::graph::EdgePtrWrapper<T>, io::datapack::LightObjectPack> {
			using detail::TypedMpi<fpmas::graph::EdgePtrWrapper<T>, io::datapack::LightObjectPack>::TypedMpi;
		};
}}
#endif
