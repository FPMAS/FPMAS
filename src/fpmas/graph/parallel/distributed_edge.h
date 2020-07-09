#ifndef DISTRIBUTED_EDGE_IMPL_H
#define DISTRIBUTED_EDGE_IMPL_H

#include "fpmas/api/graph/parallel/distributed_edge.h"
#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/graph/base/edge.h"
#include "fpmas/graph/parallel/distributed_node.h"

namespace fpmas::graph::parallel {

	template<typename> class DistributedNode;
	using api::graph::parallel::LocationState;
	template<typename T>
		class DistributedEdge :
			public graph::base::Edge<DistributedId, api::graph::parallel::DistributedNode<T>>,
			public fpmas::api::graph::parallel::DistributedEdge<T> {
				typedef graph::base::Edge<DistributedId, api::graph::parallel::DistributedNode<T>>
					EdgeBase;

				private:
					LocationState _state = LocationState::LOCAL;

				public:
					using typename EdgeBase::IdType;
					using typename EdgeBase::LayerIdType;

					DistributedEdge(IdType id, LayerIdType layer)
						: EdgeBase(id, layer) {}

					DistributedEdge(IdType id, LayerIdType layer, float weight)
						: EdgeBase(id, layer, weight) {}

					LocationState state() const override {return _state;}
					void setState(LocationState state) override {this->_state = state;}
			};


	template<typename T>
	using EdgePtrWrapper = fpmas::api::utils::VirtualPtrWrapper<fpmas::api::graph::parallel::DistributedEdge<T>>;
}

namespace nlohmann {
	template<typename T>
		using EdgePtrWrapper = fpmas::graph::parallel::EdgePtrWrapper<T>;
	template<typename T>
		using NodePtrWrapper = fpmas::graph::parallel::NodePtrWrapper<T>;

	template<typename T>
		struct adl_serializer<EdgePtrWrapper<T>> {
			static EdgePtrWrapper<T> from_json(const json& j) {
				fpmas::api::graph::parallel::DistributedEdge<T>* edge
					= new fpmas::graph::parallel::DistributedEdge<T> {
					j.at("id").get<DistributedId>(),
					j.at("layer").get<typename fpmas::api::graph::parallel::DistributedEdge<T>::LayerIdType>(),
					j.at("weight").get<float>()
				};
				NodePtrWrapper<T> src = j.at("src").get<NodePtrWrapper<T>>();
				edge->setSourceNode(src);
				src->linkOut(edge);
				src->setLocation(j.at("src_loc").get<int>());

				NodePtrWrapper<T> tgt = j.at("tgt").get<NodePtrWrapper<T>>();
				edge->setTargetNode(tgt);
				tgt->linkIn(edge);
				tgt->setLocation(j.at("tgt_loc").get<int>());
				return edge;
			}

			static void to_json(json& j, const EdgePtrWrapper<T>& edge) {
				j["id"] = edge->getId();
				j["layer"] = edge->getLayer();
				j["weight"] = edge->getWeight();
				j["src"] = NodePtrWrapper<T>(edge->getSourceNode());
				j["src_loc"] = edge->getSourceNode()->getLocation();
				j["tgt"] = NodePtrWrapper<T>(edge->getTargetNode());
				j["tgt_loc"] = edge->getTargetNode()->getLocation();
			}
		};

}

#endif
