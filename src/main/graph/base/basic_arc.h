#ifndef BASIC_ARC_H
#define BASIC_ARC_H

#include "api/graph/base/node.h"

namespace FPMAS::graph::base {

	using FPMAS::api::graph::base::LayerId;
	template<typename T, typename IdType>
		class BasicArc : public FPMAS::api::graph::base::Arc<T, IdType> {
			public:
				using typename FPMAS::api::graph::base::Arc<T, IdType>::abstract_node_ptr;

			private:
				IdType id;
				LayerId layer;
				float weight;

				abstract_node_ptr source;
				abstract_node_ptr target;

			public:
				BasicArc(abstract_node_ptr, abstract_node_ptr, LayerId);
				BasicArc(abstract_node_ptr, abstract_node_ptr, LayerId, float);

				IdType getId() const {return id;};
				LayerId getLayer() const {return layer;};

				float getWeight() const {return weight;};
				void setWeight(float weight) {this->weight=weight;};

				abstract_node_ptr* const getSourceNode() const {return source;};
				abstract_node_ptr* const getTargetNode() const {return source;};
		};

	template<typename T, typename IdType>
		BasicArc<T, IdType>::BasicArc(abstract_node_ptr source, abstract_node_ptr target, LayerId id)
			: BasicArc(source, target, id, 1.) {
			}

	template<typename T, typename IdType>
		BasicArc<T, IdType>::BasicArc(abstract_node_ptr source, abstract_node_ptr target, LayerId id, float weight)
			: source(source), target(target), id(id), weight(weight) {
				source->linkOut(this);
				target->linkIn(this);
			}
}
#endif
