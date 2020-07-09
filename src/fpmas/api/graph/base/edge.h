#ifndef EDGE_API_H
#define EDGE_API_H

#include "id.h"

namespace fpmas { namespace api { namespace graph {

	/**
	 * Type used to index layers.
	 */
	typedef int LayerId;

	template<typename _IdType, typename _NodeType>
	class Edge {
		public:
			typedef _IdType IdType;
			typedef _NodeType NodeType;
			typedef int LayerIdType;

			virtual IdType getId() const = 0;
			virtual LayerIdType getLayer() const = 0;

			virtual float getWeight() const = 0;
			virtual void setWeight(float weight) = 0;

			virtual void setSourceNode(NodeType* const src) = 0;
			virtual NodeType* const getSourceNode() const = 0;
			virtual void setTargetNode(NodeType* const src) = 0;
			virtual NodeType* const getTargetNode() const = 0;

			virtual ~Edge() {};
	};
}}}
#endif
