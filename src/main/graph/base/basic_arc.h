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
				BasicArc(layer_id_type);
				BasicArc(layer_id_type, float);

				IdType getId() const override {return id;};
				layer_id_type getLayer() const override {return layer;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight=weight;};

				void setSourceNode(const node_type* const src) {this->source = src;};
				node_type* const getSourceNode() const override {return source;};

				void setTargetNode(const node_type* const tgt) {this->target = tgt;};
				node_type* const getTargetNode() const override {return source;};

				void unlink() override;
		};

	template<typename IdType, typename NodeType>
		BasicArc<IdType, NodeType>::BasicArc(layer_id_type id)
			: BasicArc(id, 1.) {
			}

	template<typename IdType, typename NodeType>
		BasicArc<IdType, NodeType>::BasicArc(layer_id_type id, float weight)
			: id(id), weight(weight) {
			}

	template<typename IdType, typename NodeType>
		void BasicArc<IdType, NodeType>::unlink() {
			this->getTargetNode()->unlinkIn(this, this->getLayer());
			this->getSourceNode()->unlinkOut(this, this->getLayer());
		}
}
#endif
