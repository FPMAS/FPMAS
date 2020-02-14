#ifndef FOSSIL_H
#define FOSSIL_H

#include <set>

#include "node.h"
#include "arc.h"

namespace FPMAS::graph::base {

	/**
	 * FossilArcs are used to represent Arc s that remain partially connected 
	 * deleting its source and target Node. Those arcs should be deleted
	 * externally.
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
	 * being `deleted` it is added to the returned FossilArcs instance. The
	 * calling DistributedGraph can then handle those fossil arcs, removing
	 * them from the GhostArc register and properly `deleting` them.
	 *
	 * See the DistributedGraph<T>::distribute() and Graph<T>::removeNode()
	 * functions implementation for concrete usage examples.
	 */
	template<class A> class FossilArcs {
		public:
			/**
			 * Set of arcs that remain partially connected after their target
			 * nodes have been deleted.
			 * Trying to access the target node pointer of those arcs will
			 * raise an error.
			 */
			std::set<A*> incomingArcs;

			/**
			 * Set of arcs that remain partially connected after their source
			 * nodes have been deleted.
			 * Trying to access the source node pointer of those arcs will
			 * raise an error.
			 */
			std::set<A*> outgoingArcs;

			void merge(FossilArcs<A>);
	};

	/**
	 * Merge these FossilArcs with the argument fossil, adding the arcs
	 * of the Fossil argument to this fossil. The set structures used
	 * allow to avoid duplicates.
	 *
	 * @param fossil FossilArcs to add to this fossil
	 */
	template<class T> void FossilArcs<T>::merge(FossilArcs<T> fossil) {
		for(auto arc : fossil.incomingArcs) {
			this->incomingArcs.insert(arc);
		}
		for(auto arc : fossil.outgoingArcs) {
			this->outgoingArcs.insert(arc);
		}
	}
}
#endif
