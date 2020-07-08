#ifndef NODE_API_H
#define NODE_API_H

#include <vector>
#include "arc.h"
#include "id.h"
#include "fpmas/api/utils/callback.h"

namespace fpmas::api::graph::base {
	template<typename _IdType, typename _ArcType>
		class Node {
			public:
				typedef _IdType IdType;
				typedef _ArcType ArcType;
				typedef typename ArcType::LayerIdType LayerIdType;


				virtual IdType getId() const = 0;

				virtual float getWeight() const = 0;
				virtual void setWeight(float weight) = 0;

				virtual const std::vector<ArcType*> getIncomingArcs() const = 0;
				virtual const std::vector<ArcType*> getIncomingArcs(LayerIdType id) const = 0;
				virtual const std::vector<typename ArcType::NodeType*> inNeighbors() const = 0;
				virtual const std::vector<typename ArcType::NodeType*> inNeighbors(LayerId) const = 0;

				virtual const std::vector<ArcType*> getOutgoingArcs() const = 0;
				virtual const std::vector<ArcType*> getOutgoingArcs(LayerIdType id) const = 0;
				virtual const std::vector<typename ArcType::NodeType*> outNeighbors() const = 0;
				virtual const std::vector<typename ArcType::NodeType*> outNeighbors(LayerId) const = 0;

				virtual void linkIn(ArcType* arc) = 0;
				virtual void linkOut(ArcType* arc) = 0;

				virtual void unlinkIn(ArcType* arc) = 0;
				virtual void unlinkOut(ArcType* arc) = 0;

				virtual ~Node() {}
		};
}

#endif
