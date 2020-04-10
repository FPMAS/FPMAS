#ifndef GRAPH_API_H
#define GRAPH_API_H

#include <unordered_map>
#include "node.h"

#define GRAPH_PARAMS \
	typename T,\
	typename IdType,\
	template<typename, typename> class node_type_template,\
	template<typename, typename> class arc_type_template

#define GRAPH_PARAMS_SPEC T, IdType, node_type_template, arc_type_template

namespace FPMAS::api::graph::base {

	template<GRAPH_PARAMS> class Graph {
		public:
			typedef node_type_template<T, IdType> node_type;
			typedef arc_type_template<T, IdType> arc_type;
			typedef FPMAS::api::graph::base::IdHash<IdType> id_hash;

			virtual node_type* buildNode() = 0;
			virtual node_type* buildNode(T&& data) = 0;
			virtual node_type* buildNode(T&& data, float weight) = 0;

			arc_type* link(IdType, IdType, LayerId);
			virtual arc_type* link(Node<T, IdType>*, Node<T, IdType>*, LayerId) = 0;

			void removeNode(IdType);
			virtual void removeNode(Node<T, IdType>* node) = 0;

			void unlink(IdType);
			virtual void unlink(Arc<T, IdType>*) = 0;

			virtual const IdType& currentNodeId() const = 0;
			virtual const IdType& currentArcId() const = 0;

			// Node getters
			virtual node_type* getNode(IdType) = 0;
			virtual const node_type* getNode(IdType) const = 0;
			virtual const std::unordered_map<IdType, node_type*, id_hash>& getNodes() const = 0;

			// Arc getters
			virtual arc_type* getArc(IdType) = 0;
			virtual const arc_type* getArc(IdType) const = 0;
			virtual const std::unordered_map<IdType, arc_type*, id_hash>& getArcs() const = 0;
			
			virtual ~Graph() {}
	};

	template<GRAPH_PARAMS>
		typename Graph<GRAPH_PARAMS_SPEC>::arc_type* Graph<GRAPH_PARAMS_SPEC>::link(
				IdType source, IdType target, LayerId layer
				) {
			return this->link(this->getNode(source), this->getNode(target), layer);
		}

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::removeNode(IdType id) {
			this->removeNode(this->getNode(id));
		}

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::unlink(IdType id) {
			this->unlink(this->getArc(id));
		}

}
#endif
