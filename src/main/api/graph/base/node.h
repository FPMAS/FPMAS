#ifndef NODE_API_H
#define NODE_API_H

#include <vector>
#include "arc.h"
#include "id.h"

namespace FPMAS::api::graph::base {

	template<typename _IdType, typename _ArcType>
		class Node {
			public:
				//typedef T Data;
				typedef _IdType IdType;
				typedef _ArcType ArcType;
				typedef typename ArcType::LayerIdType LayerIdType;


				virtual IdType getId() const = 0;

				//virtual T& data() = 0;
				//virtual const T& data() const = 0;

				virtual float getWeight() const = 0;
				virtual void setWeight(float weight) = 0;

				virtual const std::vector<ArcType*> getIncomingArcs() = 0;
				virtual const std::vector<ArcType*> getIncomingArcs(LayerIdType id) = 0;
				const std::vector<typename ArcType::NodeType*> inNeighbors();
				const std::vector<typename ArcType::NodeType*> inNeighbors(LayerId);

				virtual const std::vector<ArcType*> getOutgoingArcs() = 0;
				virtual const std::vector<ArcType*> getOutgoingArcs(LayerIdType id) = 0;
				const std::vector<typename ArcType::NodeType*> outNeighbors();
				const std::vector<typename ArcType::NodeType*> outNeighbors(LayerId);

				virtual void linkIn(ArcType* arc) = 0;
				virtual void linkOut(ArcType* arc) = 0;

				virtual void unlinkIn(ArcType* arc) = 0;
				virtual void unlinkOut(ArcType* arc) = 0;

				virtual ~Node() {}
		};

	template<typename _IdType, typename _ArcType>
		const std::vector<typename Node<_IdType, _ArcType>::ArcType::NodeType*>
		Node<_IdType, _ArcType>::inNeighbors() {
			std::vector<typename ArcType::NodeType*> neighbors;
			for(auto arc : this->getIncomingArcs()) {
				neighbors.push_back(arc->getSourceNode());
			}
			return neighbors;
		}

	template<typename _IdType, typename _ArcType>
		const std::vector<typename Node<_IdType, _ArcType>::ArcType::NodeType*>
		Node<_IdType, _ArcType>::inNeighbors(LayerId layer) {
			std::vector<typename ArcType::NodeType*> neighbors;
			for(auto arc : this->getIncomingArcs(layer)) {
				neighbors.push_back(arc->getSourceNode());
			}
			return neighbors;
		}

	template<typename _IdType, typename _ArcType>
		const std::vector<typename Node<_IdType, _ArcType>::ArcType::NodeType*>
		Node<_IdType, _ArcType>::outNeighbors() {
			std::vector<typename ArcType::NodeType*> neighbors;
			for(auto arc : this->getOutgoingArcs()) {
				neighbors.push_back(arc->getTargetNode());
			}
			return neighbors;
		}

	template<typename _IdType, typename _ArcType>
		const std::vector<typename Node<_IdType, _ArcType>::ArcType::NodeType*>
		Node<_IdType, _ArcType>::outNeighbors(LayerId layer) {
			std::vector<typename ArcType::NodeType*> neighbors;
			for(auto arc : this->getOutgoingArcs(layer)) {
				neighbors.push_back(arc->getTargetNode());
			}
			return neighbors;
		}
}

#endif
