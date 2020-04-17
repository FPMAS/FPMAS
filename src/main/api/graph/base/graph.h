#ifndef GRAPH_API_H
#define GRAPH_API_H

#include <unordered_map>
#include "node.h"

//#define GRAPH_PARAMS \
	//typename T,\
	//typename IdType
#define GRAPH_PARAMS \
	typename GraphImplem,\
	typename NodeType,\
	typename ArcType

//#define GRAPH_PARAMS_SPEC T, IdType
#define GRAPH_PARAMS_SPEC GraphImplem, NodeType, ArcType

namespace FPMAS::api::graph::base {

	template<GRAPH_PARAMS> class Graph {
		static_assert(std::is_base_of<typename NodeType::arc_type, ArcType>::value);
		static_assert(std::is_base_of<typename ArcType::node_type, NodeType>::value);
		public:
			typedef NodeType node_type;
			typedef typename ArcType::node_type node_base; // Maybe node_type == node_base, but, in any case, node_type is convertible to node_base.
			typedef ArcType arc_type;
			typedef typename NodeType::arc_type arc_base;

			typedef typename NodeType::id_type node_id_type;
			typedef typename ArcType::id_type arc_id_type;
			typedef typename arc_type::layer_id_type layer_id_type;

			typedef FPMAS::api::graph::base::IdHash<typename NodeType::id_type> node_id_hash;
			typedef FPMAS::api::graph::base::IdHash<typename ArcType::id_type> arc_id_hash;
			typedef std::unordered_map<
				node_id_type, node_type*, node_id_hash
				> node_map;
			typedef std::unordered_map<
				arc_id_type, arc_type*, arc_id_hash
				> arc_map;


		protected:
			node_id_type nodeId;
			virtual void insert(node_type* node) = 0;
			virtual void insert(arc_type* arc) = 0;

			arc_id_type arcId;
			virtual void erase(node_base* node) = 0;
			virtual void erase(arc_base* arc) = 0;

		public:
			// Node getters
			const node_id_type& currentNodeId() const {return nodeId;};
			virtual node_type* getNode(node_id_type) = 0;
			virtual const node_type* getNode(node_id_type) const = 0;
			virtual const node_map& getNodes() const = 0;

			// Arc getters
			virtual const arc_id_type& currentArcId() const {return arcId;};
			virtual arc_type* getArc(arc_id_type) = 0;
			virtual const arc_type* getArc(arc_id_type) const = 0;
			virtual const arc_map& getArcs() const = 0;

			template<typename... Args> node_type* buildNode(Args... args) {
				auto node = 
					dynamic_cast<GraphImplem*>(this)
					->_buildNode(nodeId++, std::forward<Args>(args)...);
				this->insert(node);
				return node;
			}

			template<typename... Args> arc_type* link(
					node_base* src, node_base* tgt, layer_id_type layer, Args... args) {
				auto arc =
					dynamic_cast<GraphImplem*>(this)->template _link<Args...>(
							arcId++, src, tgt, layer, args...);
				this->insert(arc);
				return arc;
			}

			virtual void removeNode(node_type*) = 0;
			void removeNode(const node_id_type& id) {
				this->removeNode(this->getNode(id));
			};

			virtual void unlink(arc_type*) = 0;
			void unlink(const arc_id_type& id) {
				this->unlink(this->getArc(id));
			}
		
			virtual ~Graph() {}
	};
/*
 *
 *    template<GRAPH_PARAMS>
 *        typename Graph<GRAPH_PARAMS_SPEC>::arc_type* Graph<GRAPH_PARAMS_SPEC>::link(
 *                IdType source, IdType target, LayerId layer
 *                ) {
 *            return this->link(this->getNode(source), this->getNode(target), layer);
 *        }
 */

/*
 *    template<GRAPH_PARAMS>
 *        void Graph<GRAPH_PARAMS_SPEC>::removeNode(IdType id) {
 *            this->removeNode(this->getNode(id));
 *        }
 *
 *    template<GRAPH_PARAMS>
 *        void Graph<GRAPH_PARAMS_SPEC>::unlink(IdType id) {
 *            this->unlink(this->getArc(id));
 *        }
 */

}
#endif
