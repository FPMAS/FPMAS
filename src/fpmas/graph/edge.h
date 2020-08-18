#ifndef FPMAS_EDGE_H
#define FPMAS_EDGE_H

/** \file src/fpmas/graph/edge.h
 * Edge implementation.
 */

#include "fpmas/api/graph/node.h"

namespace fpmas { namespace graph {

	/**
	 * api::graph::Edge implementation.
	 */
	template<typename IdType, typename _NodeType>
		class Edge : public virtual api::graph::Edge<IdType, _NodeType> {
			public:
				typedef api::graph::Edge<IdType, _NodeType> EdgeBase;
				using typename EdgeBase::NodeType;
				typedef api::graph::LayerId LayerId;

			private:
				IdType id;
				LayerId layer;
				float weight;

				NodeType* source;
				NodeType* target;

			public:
				/**
				 * Edge constructor.
				 *
				 * The Edge is initialized with a default weight of 1.
				 *
				 * @param id edge id
				 * @param layer edge layer
				 */
				Edge(IdType id, LayerId layer) :
					id(id), layer(layer), weight(1.f) {};

				IdType getId() const override {return id;};
				LayerId getLayer() const override {return layer;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight=weight;};

				void setSourceNode(NodeType* const src) override {this->source = src;};
				NodeType* const getSourceNode() const override {return source;};

				void setTargetNode(NodeType* const tgt) override {this->target = tgt;};
				NodeType* const getTargetNode() const override {return target;};
		};
}}
#endif
