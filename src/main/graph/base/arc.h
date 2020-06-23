#ifndef BASIC_ARC_H
#define BASIC_ARC_H

#include "api/graph/base/node.h"

namespace fpmas::graph::base {

	template<typename IdType, typename _NodeType>
		class Arc : public virtual fpmas::api::graph::base::Arc<IdType, _NodeType> {
			public:
				typedef fpmas::api::graph::base::Arc<IdType, _NodeType> ArcBase;
				using typename ArcBase::NodeType;
				using typename ArcBase::LayerIdType;

			private:
				IdType id;
				LayerIdType layer;
				float weight;

				NodeType* source;
				NodeType* target;

			public:
				Arc(IdType id, LayerIdType layer) :
					Arc(id, layer, 1.f) {};
				Arc(IdType id, LayerIdType layer, float weight)
					: id(id), layer(layer), weight(weight) {};

				IdType getId() const override {return id;};
				LayerIdType getLayer() const override {return layer;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight=weight;};

				void setSourceNode(NodeType* const src) override {this->source = src;};
				NodeType* const getSourceNode() const override {return source;};

				void setTargetNode(NodeType* const tgt) override {this->target = tgt;};
				NodeType* const getTargetNode() const override {return target;};
		};
}
#endif
