#ifndef NODE_API_H
#define NODE_API_H

#include <vector>
#include "edge.h"
#include "id.h"
#include "fpmas/api/utils/callback.h"

namespace fpmas { namespace api { namespace graph {
	template<typename _IdType, typename _EdgeType>
		class Node {
			public:
				typedef _IdType IdType;
				typedef _EdgeType EdgeType;
				typedef typename EdgeType::LayerIdType LayerIdType;


				virtual IdType getId() const = 0;

				virtual float getWeight() const = 0;
				virtual void setWeight(float weight) = 0;

				virtual const std::vector<EdgeType*> getIncomingEdges() const = 0;
				virtual const std::vector<EdgeType*> getIncomingEdges(LayerIdType id) const = 0;
				virtual const std::vector<typename EdgeType::NodeType*> inNeighbors() const = 0;
				virtual const std::vector<typename EdgeType::NodeType*> inNeighbors(LayerId) const = 0;

				virtual const std::vector<EdgeType*> getOutgoingEdges() const = 0;
				virtual const std::vector<EdgeType*> getOutgoingEdges(LayerIdType id) const = 0;
				virtual const std::vector<typename EdgeType::NodeType*> outNeighbors() const = 0;
				virtual const std::vector<typename EdgeType::NodeType*> outNeighbors(LayerId) const = 0;

				virtual void linkIn(EdgeType* edge) = 0;
				virtual void linkOut(EdgeType* edge) = 0;

				virtual void unlinkIn(EdgeType* edge) = 0;
				virtual void unlinkOut(EdgeType* edge) = 0;

				virtual ~Node() {}
		};
}}}
#endif
