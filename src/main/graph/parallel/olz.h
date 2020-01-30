#ifndef OLZ_H
#define OLZ_H

#include "../base/graph.h"
#include "synchro/sync_data.h"
#include "synchro/local_data.h"
#include "synchro/ghost_data.h"

using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

namespace FPMAS::graph::parallel {

	using synchro::SyncDataPtr;
	using synchro::GhostData;

	template<class T, template<typename> class S> class GhostGraph;

	/**
	 * GhostNode s are used to locally represent nodes that are living on
	 * an other processor, to maintain data continuity and the global graph
	 * structure.
	 *
	 * They are designed to be used as normal nodes in the graph structure.
	 * For example, when looking at nodes linked to a particular node, no
	 * difference can be made between nodes really living locally and ghost
	 * nodes.
	 */
	template <class T, template<typename> class S> class GhostNode : public Node<SyncDataPtr<T>> {
		friend GhostNode<T, S>* GhostGraph<T, S>::buildNode(unsigned long);
		friend GhostNode<T, S>* GhostGraph<T, S>::buildNode(Node<SyncDataPtr<T>>, std::set<unsigned long>);

		private:
		GhostNode(SyncMpiCommunicator&, Proxy&, unsigned long);
		GhostNode(SyncMpiCommunicator&, Proxy&, Node<SyncDataPtr<T>>);
	};

	template<class T, template<typename> class S> GhostNode<T, S>::GhostNode(SyncMpiCommunicator& mpiComm, Proxy& proxy, unsigned long id)
		: Node<SyncDataPtr<T>>(id, 1., SyncDataPtr<T>(new S<T>(id, mpiComm, proxy))) {
		}

	/**
	 * Builds a GhostNode as a copy of the specified node.
	 *
	 * @param node original node
	 */
	template<class T, template<typename> class S> GhostNode<T, S>::GhostNode(SyncMpiCommunicator& mpiComm, Proxy& proxy, Node<SyncDataPtr<T>> node)
		: Node<SyncDataPtr<T>>(node.getId(), node.getWeight(), SyncDataPtr<T>(new S<T>(node.getId(), mpiComm, proxy, node.data()->get()))) {
		}

	/**
	 * GhostArcs should be used to link two nodes when at least one of the
	 * two nodes is a GhostNode.
	 *
	 * They behaves exactly as normal arcs. In consequence, when looking at
	 * a node outgoing arcs for example, no difference can directly be made
	 * between local arcs and ghost arcs.
	 *
	 */
	template<class T, template<typename> class S = GhostData> class GhostArc : public Arc<SyncDataPtr<T>> {

		friend void GhostGraph<T, S>::link(GhostNode<T, S>*, Node<SyncDataPtr<T>>*, unsigned long);
		friend void GhostGraph<T, S>::link(Node<SyncDataPtr<T>>*, GhostNode<T, S>*, unsigned long);

		private:
		GhostArc(unsigned long, GhostNode<T, S>*, Node<SyncDataPtr<T>>*);
		GhostArc(unsigned long, Node<SyncDataPtr<T>>*, GhostNode<T, S>*);

	};

	/**
	 * Builds a GhostArc linking the specified nodes. Notice that the
	 * GhostArc instance is added to the regular incoming arcs list of the
	 * local target node.
	 *
	 * @param arc_id arc id
	 * @param source pointer to the source ghost node
	 * @param target pointer to the local target node
	 */
	template<class T, template<typename> class S> GhostArc<T, S>::GhostArc(unsigned long arc_id, GhostNode<T, S>* source, Node<SyncDataPtr<T>>* target)
		: Arc<SyncDataPtr<T>>(arc_id, source, target) { };

	/**
	 * Builds a GhostArc linking the specified nodes. Notice that the
	 * GhostArc instance is added to the regular outgoing arcs list of the
	 * local source node.
	 *
	 * @param arc_id arc id
	 * @param source pointer to the local source node
	 * @param target pointer to the target ghost node
	 */
	template<class T, template<typename> class S> GhostArc<T, S>::GhostArc(unsigned long arc_id, Node<SyncDataPtr<T>>* source, GhostNode<T, S>* target)
		: Arc<SyncDataPtr<T>>(arc_id, source, target) { };

}

#endif
