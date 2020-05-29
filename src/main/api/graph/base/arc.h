#ifndef ARC_API_H
#define ARC_API_H

#include "id.h"

namespace FPMAS::api::graph::base {

	/**
	 * Type used to index layers.
	 */
	typedef int LayerId;

	//inline static constexpr LayerId DEFAULT_LAYER = 0;

	template<typename _IdType, typename _NodeType>
	class Arc {
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

			virtual ~Arc() {};
	};
}
#endif
