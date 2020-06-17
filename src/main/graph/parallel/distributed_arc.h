#ifndef DISTRIBUTED_ARC_IMPL_H
#define DISTRIBUTED_ARC_IMPL_H

#include "api/graph/parallel/distributed_arc.h"
#include "api/utils/ptr_wrapper.h"
#include "graph/base/arc.h"
#include "graph/parallel/distributed_node.h"

namespace FPMAS::graph::parallel {

	template<typename> class DistributedNode;
	using api::graph::parallel::LocationState;
	template<typename T>
		class DistributedArc :
			public graph::base::Arc<DistributedId, api::graph::parallel::DistributedNode<T>>,
			public FPMAS::api::graph::parallel::DistributedArc<T> {
				typedef graph::base::Arc<DistributedId, api::graph::parallel::DistributedNode<T>>
					ArcBase;

				private:
					LocationState _state = LocationState::LOCAL;

				public:
					using typename ArcBase::IdType;
					using typename ArcBase::LayerIdType;

					DistributedArc(IdType id, LayerIdType layer)
						: ArcBase(id, layer) {}

					DistributedArc(IdType id, LayerIdType layer, float weight)
						: ArcBase(id, layer, weight) {}

					LocationState state() const override {return _state;}
					void setState(LocationState state) override {this->_state = state;}
			};


	template<typename T>
	using ArcPtrWrapper = FPMAS::api::utils::VirtualPtrWrapper<FPMAS::api::graph::parallel::DistributedArc<T>>;
}

namespace nlohmann {
	template<typename T>
		using ArcPtrWrapper = FPMAS::graph::parallel::ArcPtrWrapper<T>;
	template<typename T>
		using NodePtrWrapper = FPMAS::graph::parallel::NodePtrWrapper<T>;

	template<typename T>
		struct adl_serializer<ArcPtrWrapper<T>> {
			static ArcPtrWrapper<T> from_json(const json& j) {
				FPMAS::api::graph::parallel::DistributedArc<T>* arc
					= new FPMAS::graph::parallel::DistributedArc<T> {
					j.at("id").get<DistributedId>(),
					j.at("layer").get<typename FPMAS::api::graph::parallel::DistributedArc<T>::LayerIdType>(),
					j.at("weight").get<float>()
				};
				/*auto* src = new FPMAS::graph::parallel::DistributedNode<T>(*/
						//j.at("src").get<DistributedId>()
					  /*  )*/;
				NodePtrWrapper<T> src = j.at("src").get<NodePtrWrapper<T>>();
				arc->setSourceNode(src);
				src->linkOut(arc);

				//auto* tgt = new FPMAS::graph::parallel::DistributedNode<T>(
						//j.at("tgt").get<DistributedId>()
						//);
				NodePtrWrapper<T> tgt = j.at("tgt").get<NodePtrWrapper<T>>();
				arc->setTargetNode(tgt);
				tgt->linkIn(arc);
				return arc;
			}

			static void to_json(json& j, const ArcPtrWrapper<T>& arc) {
				j["id"] = arc->getId();
				j["layer"] = arc->getLayer();
				j["weight"] = arc->getWeight();
				j["src"] = NodePtrWrapper<T>(arc->getSourceNode());
				j["tgt"] = NodePtrWrapper<T>(arc->getTargetNode());
			}
		};

}

#endif
