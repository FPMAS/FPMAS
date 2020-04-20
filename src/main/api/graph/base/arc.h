#ifndef ARC_API_H
#define ARC_API_H

#include "id.h"

namespace FPMAS::api::graph::base {

	/**
	 * Type used to index layers.
	 */
	typedef int LayerId;

	static constexpr LayerId DefaultLayer = 0;

	template<typename IdType, typename NodeType>
	class Arc {
		public:
			typedef IdType id_type;
			typedef NodeType node_type;
			typedef int layer_id_type;

			virtual id_type getId() const = 0;
			virtual layer_id_type getLayer() const = 0;

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
