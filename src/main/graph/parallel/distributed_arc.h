#ifndef DISTRIBUTED_NODE_IMPL_H
#define DISTRIBUTED_NODE_IMPL_H

#include "api/graph/parallel/distributed_arc.h"
#include "graph/base/basic_arc.h"

namespace FPMAS::graph::parallel {

	template<typename> class DistributedNode;
	using api::graph::parallel::LocationState;
	template<typename T>
		class DistributedArc :
			public graph::base::BasicArc<DistributedId, DistributedNode<T>>,
			public FPMAS::api::graph::parallel::DistributedArc<T, DistributedNode<T>> {
				typedef graph::base::BasicArc<DistributedId, DistributedNode<T>>
					arc_base;

				private:
					LocationState _state = LocationState::LOCAL;

				public:
					using typename arc_base::id_type;
					using typename arc_base::layer_id_type;

					DistributedArc(id_type id, layer_id_type layer)
						: arc_base(id, layer) {}

					DistributedArc(id_type id, layer_id_type layer, float weight)
						: arc_base(id, layer, weight) {}

					LocationState state() const override {return _state;}
					void setState(LocationState state) override {this->_state = state;}

			};
}

namespace nlohmann {

	using FPMAS::graph::parallel::DistributedArc;
	template<typename T>
		struct adl_serializer<DistributedArc<T>> {
			static DistributedArc<T> from_json(const json& j) {
				DistributedArc<T> arc {
					j.at("id").get<DistributedId>(),
					j.at("layer").get<typename DistributedArc<T>::layer_id_type>(),
					j.at("weight").get<float>()
				};
				arc.setSourceNode(
						new FPMAS::graph::parallel::DistributedNode<T>(
							j.at("src").get<DistributedId>()
							)
						);
				arc.setTargetNode(
						new FPMAS::graph::parallel::DistributedNode<T>(
							j.at("tgt").get<DistributedId>()
							)
						);
				return arc;
			}

			static void to_json(json& j, const DistributedArc<T>& arc) {
				j["id"] = arc.getId();
				j["layer"] = arc.getLayer();
				j["weight"] = arc.getWeight();
				j["src"] = arc.getSourceNode()->getId();
				j["tgt"] = arc.getTargetNode()->getId();
			}
		};

}

#endif
