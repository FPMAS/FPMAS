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
	template<typename _IdType, typename _NodeType>
		class Edge : public virtual api::graph::Edge<_IdType, _NodeType> {
			public:
				using typename api::graph::Edge<_IdType, _NodeType>::IdType;
				using typename api::graph::Edge<_IdType, _NodeType>::NodeType;

			private:
				IdType id;
				api::graph::LayerId layer;
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
				Edge(IdType id, api::graph::LayerId layer) :
					id(id), layer(layer), weight(1.f) {};

				IdType getId() const override {return id;};
				api::graph::LayerId getLayer() const override {return layer;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight=weight;};

				void setSourceNode(NodeType* const src) override {this->source = src;};
				NodeType* getSourceNode() const override {return source;};

				void setTargetNode(NodeType* const tgt) override {this->target = tgt;};
				NodeType* getTargetNode() const override {return target;};
		};
}}
#endif
