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

				NodePtrWrapper<T> src = j.at("src")[0].template get<NodePtrWrapper<T>>();
				src->setLocation(j.at("src")[1].template get<int>());
				edge->setSourceNode(src);
				src->linkOut(edge);

				NodePtrWrapper<T> tgt = j.at("tgt")[0].template get<NodePtrWrapper<T>>();
				tgt->setLocation(j.at("tgt")[1].template get<int>());

				edge->setTargetNode(tgt);
				tgt->linkIn(edge);
				edge_ptr = {edge};
			}
		};
}

namespace fpmas { namespace communication {

	/**
	 * \anchor TypedMpiDistributedEdge
	 *
	 * \DistributedEdge TypedMpi specialization, used to bypass the default
	 * fpmas::communication::TypedMpi specialization based on nlohmann::json.
	 *
	 * Here, fpmas::io::json::light_json is used instead. This allows to use
	 * "light" serialization rules of source and target \DistributedNodes,
	 * since sending their data is not required when transmitting
	 * \DistributedEdges.
	 *
	 * @tparam T type of data contained in associated \DistributedNodes
	 */
	template<typename T>
		struct TypedMpi<fpmas::graph::EdgePtrWrapper<T>> : public detail::TypedMpi<fpmas::graph::EdgePtrWrapper<T>, fpmas::io::json::light_json> {
			using detail::TypedMpi<fpmas::graph::EdgePtrWrapper<T>, fpmas::io::json::light_json>::TypedMpi;
		};

}}
#endif
