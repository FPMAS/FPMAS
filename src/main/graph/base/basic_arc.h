#ifndef BASIC_ARC_H
#define BASIC_ARC_H

#include "api/graph/base/node.h"

namespace FPMAS::graph::base {

	template<typename IdType, typename NodeType>
		class BasicArc : public FPMAS::api::graph::base::Arc<IdType, NodeType> {
			public:
				typedef FPMAS::api::graph::base::Arc<IdType, NodeType> arc_base;
				typedef IdType id_type;
				typedef NodeType node_type;
				using typename arc_base::layer_id_type;

			private:
				IdType id;
				layer_id_type layer;
				float weight;

				node_type* source;
				node_type* target;

			public:
				BasicArc(node_type*, node_type*, layer_id_type);
				BasicArc(node_type*, node_type*, layer_id_type, float);

				IdType getId() const override {return id;};
				layer_id_type getLayer() const override {return layer;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight=weight;};

				node_type* const getSourceNode() const override {return source;};
				node_type* const getTargetNode() const override {return source;};

				void unlink() override;
		};

	template<typename IdType, typename NodeType>
		BasicArc<IdType, NodeType>::BasicArc(node_type* source, node_type* target, layer_id_type id)
			: BasicArc(source, target, id, 1.) {
			}

	template<typename IdType, typename NodeType>
		BasicArc<IdType, NodeType>::BasicArc(node_type* source, node_type* target, layer_id_type id, float weight)
			: source(source), target(target), id(id), weight(weight) {
				source->linkOut(this);
				target->linkIn(this);
			}

	template<typename IdType, typename NodeType>
		void BasicArc<IdType, NodeType>::unlink() {
			this->getTargetNode()->unlinkIn(this, this->getLayer());
			this->getSourceNode()->unlinkOut(this, this->getLayer());
		}
}
#endif
