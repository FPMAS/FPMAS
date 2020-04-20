#ifndef BASIC_ARC_H
#define BASIC_ARC_H

#include "api/graph/base/node.h"

namespace FPMAS::graph::base {

	template<typename IdType, typename NodeType>
		class BasicArc : public virtual FPMAS::api::graph::base::Arc<IdType, NodeType> {
			public:
				typedef FPMAS::api::graph::base::Arc<IdType, NodeType> arc_base;
				using typename arc_base::id_type;
				using typename arc_base::node_type;
				using typename arc_base::layer_id_type;

			private:
				id_type id;
				layer_id_type layer;
				float weight;

				node_type* source;
				node_type* target;

			public:
				BasicArc(id_type id, layer_id_type layer) :
					BasicArc(id, layer, 1.f) {};
				BasicArc(id_type id, layer_id_type layer, float weight)
					: id(id), layer(layer), weight(weight) {};

				id_type getId() const override {return id;};
				layer_id_type getLayer() const override {return layer;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight=weight;};

				void setSourceNode(node_type* const src) {this->source = src;};
				node_type* const getSourceNode() const override {return source;};

				void setTargetNode(node_type* const tgt) {this->target = tgt;};
				node_type* const getTargetNode() const override {return target;};
		};
}
#endif
