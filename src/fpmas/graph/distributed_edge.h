#ifndef FPMAS_DISTRIBUTED_EDGE_H
#define FPMAS_DISTRIBUTED_EDGE_H

/** \file src/fpmas/graph/distributed_edge.h
 * DistributedEdge implementation.
 */

#include "fpmas/graph/edge.h"
#include "fpmas/graph/distributed_node.h"

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
	 * DistributedEdge JSON serialization.
	 */
	template<typename T>
		struct adl_serializer<EdgePtrWrapper<T>> {
			/**
			 * DistributedEdge json serialization.
			 *
			 * @param j json
			 * @param edge wrapper of pointer to the DistributedEdge to serialize
			 */
			static void to_json(json& j, const EdgePtrWrapper<T>& edge) {
				j["id"] = edge->getId();
				j["layer"] = edge->getLayer();
				j["weight"] = edge->getWeight();
				j["src"] = {{}, edge->getSourceNode()->location()};
				NodePtrWrapper<T> src(edge->getSourceNode());
				fpmas::io::json::light_serializer<NodePtrWrapper<T>>::to_json(j["src"][0], src);
				j["tgt"] = {{}, edge->getTargetNode()->location()};
				NodePtrWrapper<T> tgt(edge->getTargetNode());
				fpmas::io::json::light_serializer<NodePtrWrapper<T>>::to_json(j["tgt"][0], tgt);
				/*
				 *j["src"] = {
				 *    edge->getSourceNode()->getId(),
				 *    edge->getSourceNode()->location()
				 *};
				 *j["tgt"] = {
				 *    edge->getTargetNode()->getId(),
				 *    edge->getTargetNode()->location()
				 *};
				 */
			}

			/**
			 * DistributedEdge json unserialization.
			 *
			 * @param j json
			 * @return wrapped heap allocated unserialized
			 * fpmas::graph::DistributedEdge. Source and target nodes are
			 * unserialized using nlohmann::adl_serializer<NodePtrWrapper>
			 *
			 * @see \ref adl_serializer_node "nlohmann::adl_serializer<NodePtrWrapper>"
			 */
			static EdgePtrWrapper<T> from_json(const json& j) {
				fpmas::api::graph::DistributedEdge<T>* edge
					= new fpmas::graph::DistributedEdge<T> {
					j.at("id").get<DistributedId>(),
					j.at("layer").get<typename fpmas::graph::LayerId>()
				};
				edge->setWeight(j.at("weight").get<float>());

				NodePtrWrapper<T> src = fpmas::io::json::light_serializer<NodePtrWrapper<T>>::from_json(j.at("src")[0]);
				/*
				 *NodePtrWrapper<T> src = {
				 *    new fpmas::graph::DistributedNode<T>(
				 *        j.at("src").at(0).get<DistributedId>(), {}
				 *        )
				 *};
				 */
				src->setLocation(j.at("src")[1].get<int>());
				edge->setSourceNode(src);
				src->linkOut(edge);

				NodePtrWrapper<T> tgt = fpmas::io::json::light_serializer<NodePtrWrapper<T>>::from_json(j.at("tgt")[0]);
				/*
				 *NodePtrWrapper<T> tgt = {
				 *    new fpmas::graph::DistributedNode<T>(
				 *        j.at("tgt").at(0).get<DistributedId>(), {}
				 *        )
				 *};
				 */
				tgt->setLocation(j.at("tgt")[1].get<int>());

				edge->setTargetNode(tgt);
				tgt->linkIn(edge);
				return edge;
			}
		};
}
#endif
