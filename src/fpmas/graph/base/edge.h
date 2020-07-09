#ifndef BASIC_EDGE_H
#define BASIC_EDGE_H

#include "fpmas/api/graph/base/node.h"

namespace fpmas::graph::base {

	template<typename IdType, typename _NodeType>
		class Edge : public virtual fpmas::api::graph::base::Edge<IdType, _NodeType> {
			public:
				typedef fpmas::api::graph::base::Edge<IdType, _NodeType> EdgeBase;
				using typename EdgeBase::NodeType;
				using typename EdgeBase::LayerIdType;

			private:
				IdType id;
				LayerIdType layer;
				float weight;

				NodeType* source;
				NodeType* target;

			public:
				Edge(IdType id, LayerIdType layer) :
					Edge(id, layer, 1.f) {};
				Edge(IdType id, LayerIdType layer, float weight)
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
