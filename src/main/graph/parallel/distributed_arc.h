#ifndef DISTRIBUTED_ARC_IMPL_H
#define DISTRIBUTED_ARC_IMPL_H

#include "api/graph/parallel/distributed_arc.h"
#include "graph/base/basic_arc.h"
#include "utils/ptr_wrapper.h"

namespace FPMAS::graph::parallel {

	template<typename> class DistributedNode;
	using api::graph::parallel::LocationState;
	template<typename T>
		class DistributedArc :
			public graph::base::BasicArc<DistributedId, api::graph::parallel::DistributedNode<T>>,
			public FPMAS::api::graph::parallel::DistributedArc<T> {
				typedef graph::base::BasicArc<DistributedId, api::graph::parallel::DistributedNode<T>>
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
	using ArcPtrWrapper = FPMAS::utils::VirtualPtrWrapper<FPMAS::api::graph::parallel::DistributedArc<T>>;
}

namespace nlohmann {
	template<typename T>
		using ArcPtrWrapper = FPMAS::graph::parallel::ArcPtrWrapper<T>;

	template<typename T>
		struct adl_serializer<ArcPtrWrapper<T>> {
			static ArcPtrWrapper<T> from_json(const json& j) {
				FPMAS::api::graph::parallel::DistributedArc<T>* arc
					= new FPMAS::graph::parallel::DistributedArc<T> {
					j.at("id").get<DistributedId>(),
					j.at("layer").get<typename FPMAS::api::graph::parallel::DistributedArc<T>::LayerIdType>(),
					j.at("weight").get<float>()
				};
				arc->setSourceNode(
						new FPMAS::graph::parallel::DistributedNode<T>(
							j.at("src").get<DistributedId>()
							)
						);
				arc->setTargetNode(
						new FPMAS::graph::parallel::DistributedNode<T>(
							j.at("tgt").get<DistributedId>()
							)
						);
				return arc;
			}

			static void to_json(json& j, const ArcPtrWrapper<T>& arc) {
				j["id"] = arc->getId();
				j["layer"] = arc->getLayer();
				j["weight"] = arc->getWeight();
				j["src"] = arc->getSourceNode()->getId();
				j["tgt"] = arc->getTargetNode()->getId();
			}
		};

}

#endif
