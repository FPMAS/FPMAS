#ifndef FOSSIL_H
#define FOSSIL_H

#include <set>

#include "node.h"
#include "arc.h"

namespace FPMAS {
	namespace graph {

		/**
		 * Fossils are used to represent Node s and Arc s that do not belong to
		 * any graph any more, but that should be `deleted` externally for
		 * some reason.
		 *
		 * For example, they are returned by the Graph<T>::removeNode()
		 * function. In the case of a base Graph, the Fossil will always be
		 * void.
		 *
		 * A particular use case for example occurs within the
		 * DistributedGraph.
		 * Normally, when Graph<T>::removeNode() is called, all the arcs
		 * connected to the node are erased from the graph arcs register, and
		 * `deleted` by the same function. However, a distributed graph maintains
		 * GhostNode s and GhostArc s registers, and such items can be connected
		 * to "normal" nodes.
		 * In consequence, when Graph<T>::removeNode() is called on a normal node
		 * connected by a GhostArc, the arc can't be erased from the GhostArc
		 * register, because a base Graph has no information about it. So
		 * instead, the GhostArc is unlinked as a normal arc, but instead of
		 * being `deleted` it is added to the returned Fossil instance. The
		 * calling DistributedGraph can then handle those fossil arcs, removing
		 * them from the GhostArc register and properly `deleting` them.
		 *
		 * See the DistributedGraph<T>::distribute() and Graph<T>::removeNode()
		 * functions implementation for concrete usage examples.
		 */
		template<class T> class Fossil {
			public:
			/**
			 * Nodes contained in this fossil.
			 */
			std::set<Node<T>*> nodes;
			/**
			 * Arcs contained in this fossil.
			 */
			std::set<Arc<T>*> arcs;
			void merge(Fossil<T> fossil);
		};

		/**
		 * Merge this Fossil with the argument fossil, adding the arcs and
		 * nodes of the Fossil argument to this fossil. The set structures used
		 * allow to avoid duplicates.
		 *
		 * @param fossil Fossil to merge into this fossil
		 */
		template<class T> void Fossil<T>::merge(Fossil<T> fossil) {
			for(auto arc : fossil.arcs) {
				this->arcs.insert(arc);
			}
			for(auto node : fossil.nodes) {
				this->nodes.insert(node);
			}
		}
	}
}
#endif
