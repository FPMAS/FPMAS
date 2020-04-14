#ifndef NODE_API_H
#define NODE_API_H

#include <vector>
#include "arc.h"
#include "id.h"

namespace FPMAS::api::graph::base {

	template<typename T, typename IdType, typename ArcType>
		class Node {
			public:
				typedef T data_type;
				typedef IdType id_type;
				typedef ArcType arc_type;
				typedef typename arc_type::layer_id_type layer_id_type;


				virtual id_type getId() const = 0;

				virtual T& data() = 0;
				virtual const T& data() const = 0;

				virtual float getWeight() const = 0;
				virtual void setWeight(float weight) = 0;

				virtual std::vector<arc_type*> getIncomingArcs() = 0;
				virtual std::vector<arc_type*> getIncomingArcs(layer_id_type id) = 0;
				const std::vector<typename arc_type::node_type*> inNeighbors();
				const std::vector<typename arc_type::node_type*> inNeighbors(LayerId);

				virtual std::vector<arc_type*> getOutgoingArcs() = 0;
				virtual std::vector<arc_type*> getOutgoingArcs(layer_id_type id) = 0;
				const std::vector<typename arc_type::node_type*> outNeighbors();
				const std::vector<typename arc_type::node_type*> outNeighbors(LayerId);

				virtual void linkIn(arc_type* arc, LayerId layer) = 0;
				virtual void linkOut(arc_type* arc, LayerId layer) = 0;

				virtual void unlinkIn(arc_type* arc, LayerId layer) = 0;
				virtual void unlinkOut(arc_type* arc, LayerId layer) = 0;

				virtual ~Node() {}
		};

	template<typename T, typename IdType, typename ArcType>
		const std::vector<typename Node<T, IdType, ArcType>::arc_type::node_type*>
		Node<T, IdType, ArcType>::inNeighbors() {
			std::vector<typename arc_type::node_type*> neighbors;
			for(auto arc : this->getIncomingArcs()) {
				neighbors.push_back(arc->getSourceNode());
			}
			return neighbors;
		}

	template<typename T, typename IdType, typename ArcType>
		const std::vector<typename Node<T, IdType, ArcType>::arc_type::node_type*>
		Node<T, IdType, ArcType>::inNeighbors(LayerId layer) {
			std::vector<typename arc_type::node_type*> neighbors;
			for(auto arc : this->getIncomingArcs(layer)) {
				neighbors.push_back(arc->getSourceNode());
			}
			return neighbors;
		}

	template<typename T, typename IdType, typename ArcType>
		const std::vector<typename Node<T, IdType, ArcType>::arc_type::node_type*>
		Node<T, IdType, ArcType>::outNeighbors() {
			std::vector<Node<T, IdType, ArcType>*> neighbors;
			for(auto arc : this->getOutgoingArcs()) {
				neighbors.push_back(arc->getTargetNode());
			}
			return neighbors;
		}

	template<typename T, typename IdType, typename ArcType>
		const std::vector<typename Node<T, IdType, ArcType>::arc_type::node_type*>
		Node<T, IdType, ArcType>::outNeighbors(LayerId layer) {
			std::vector<typename arc_type::node_type*> neighbors;
			for(auto arc : this->getOutgoingArcs(layer)) {
				neighbors.push_back(arc->getTargetNode());
			}
			return neighbors;
		}
}

#endif
