#ifndef NODE_API_H
#define NODE_API_H

#include <vector>
#include "arc.h"
#include "id.h"

namespace FPMAS::api::graph::base {

	/**
	 * Type used to index layers.
	 */
	typedef int LayerId;

	static constexpr LayerId DefaultLayer = 0;

	template<typename T, typename IdType>
		class Node {
			public:
				typedef Arc<T, IdType> arc_type;
				virtual IdType getId() const = 0;

				virtual T& data() = 0;
				virtual const T& data() const = 0;

				virtual float getWeight() const = 0;
				virtual void setWeight(float weight) = 0;

				virtual std::vector<Arc<T, IdType>*> getIncomingArcs() = 0;
				virtual std::vector<Arc<T, IdType>*> getIncomingArcs(LayerId id) = 0;
				const std::vector<Node<T, IdType>*> inNeighbors();
				const std::vector<Node<T, IdType>*> inNeighbors(LayerId);

				virtual std::vector<Arc<T, IdType>*> getOutgoingArcs() = 0;
				virtual std::vector<Arc<T, IdType>*> getOutgoingArcs(LayerId id) = 0;
				const std::vector<Node<T, IdType>*> outNeighbors();
				const std::vector<Node<T, IdType>*> outNeighbors(LayerId);

				virtual void linkIn(Arc<T, IdType>* arc, LayerId layer) = 0;
				virtual void linkOut(Arc<T, IdType>* arc, LayerId layer) = 0;

				virtual void unlinkIn(Arc<T, IdType>* arc, LayerId layer) = 0;
				virtual void unlinkOut(Arc<T, IdType>* arc, LayerId layer) = 0;

				virtual ~Node() {}
		};

	template<typename T, typename IdType>
		const std::vector<Node<T, IdType>*> Node<T, IdType>::inNeighbors() {
			std::vector<Node<T, IdType>*> neighbors;
			for(auto arc : this->getIncomingArcs()) {
				neighbors.push_back(arc->getSourceNode());
			}
			return neighbors;
		}

	template<typename T, typename IdType>
		const std::vector<Node<T, IdType>*> Node<T, IdType>::inNeighbors(LayerId layer) {
			std::vector<Node<T, IdType>*> neighbors;
			for(auto arc : this->getIncomingArcs(layer)) {
				neighbors.push_back(arc->getSourceNode());
			}
			return neighbors;
		}

	template<typename T, typename IdType>
		const std::vector<Node<T, IdType>*> Node<T, IdType>::outNeighbors() {
			std::vector<Node<T, IdType>*> neighbors;
			for(auto arc : this->getOutgoingArcs()) {
				neighbors.push_back(arc->getTargetNode());
			}
			return neighbors;
		}

	template<typename T, typename IdType>
		const std::vector<Node<T, IdType>*> Node<T, IdType>::outNeighbors(LayerId layer) {
			std::vector<Node<T, IdType>*> neighbors;
			for(auto arc : this->getOutgoingArcs(layer)) {
				neighbors.push_back(arc->getTargetNode());
			}
			return neighbors;
		}
}

#endif
