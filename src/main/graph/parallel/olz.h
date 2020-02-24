#ifndef OLZ_H
#define OLZ_H

#include "../base/graph.h"
#include "synchro/sync_data.h"
#include "synchro/local_data.h"
#include "synchro/ghost_data.h"

using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

namespace FPMAS::graph::parallel {

	using synchro::SyncData;
	using synchro::GhostData;

	template<NODE_PARAMS, SYNC_MODE> class GhostGraph;

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
	template <NODE_PARAMS, SYNC_MODE> class GhostNode : public Node<std::unique_ptr<SyncData<T>>, LayerType, N> {
		friend GhostNode<NODE_PARAMS_SPEC, S>* GhostGraph<NODE_PARAMS_SPEC, S>
			::buildNode(unsigned long);
		friend GhostNode<NODE_PARAMS_SPEC, S>* GhostGraph<NODE_PARAMS_SPEC, S>
			::buildNode(const Node<std::unique_ptr<SyncData<T>>, LayerType, N>&, std::set<unsigned long>);

		private:
		GhostNode(SyncMpiCommunicator&, Proxy&, unsigned long);
		GhostNode(SyncMpiCommunicator&, Proxy&, const Node<std::unique_ptr<SyncData<T>>, LayerType, N>&);
	};

	template<NODE_PARAMS, SYNC_MODE> GhostNode<NODE_PARAMS_SPEC, S>
		::GhostNode(SyncMpiCommunicator& mpiComm, Proxy& proxy, unsigned long id)
		: Node<std::unique_ptr<SyncData<T>>, LayerType, N>(
				id,
				1.,
				std::unique_ptr<SyncData<T>>(
					new S<T>(id, mpiComm, proxy)
				)
			) {}

	/**
	 * Builds a GhostNode as a copy of the specified node.
	 *
	 * @param node original node
	 */
	template<NODE_PARAMS, SYNC_MODE> GhostNode<NODE_PARAMS_SPEC, S>
		::GhostNode(
			SyncMpiCommunicator& mpiComm,
			Proxy& proxy,
			const Node<std::unique_ptr<SyncData<T>>, LayerType, N>& node
			)
		: Node<std::unique_ptr<SyncData<T>>, LayerType, N>(
			node.getId(),
			node.getWeight(),
			std::unique_ptr<SyncData<T>>(new S<T>(
				node.getId(), mpiComm, proxy, node.data()->get()
				)
			)
		) {}

	/**
	 * GhostArcs should be used to link two nodes when at least one of the
	 * two nodes is a GhostNode.
	 *
	 * They behaves exactly as normal arcs. In consequence, when looking at
	 * a node outgoing arcs for example, no difference can directly be made
	 * between local arcs and ghost arcs.
	 *
	 */
	template<NODE_PARAMS, SYNC_MODE> class GhostArc : public Arc<std::unique_ptr<SyncData<T>>, LayerType, N> {
		friend void GhostGraph<NODE_PARAMS_SPEC, S>::link(
				GhostNode<NODE_PARAMS_SPEC, S>*,
				Node<std::unique_ptr<SyncData<T>>, LayerType, N>*,
				unsigned long,
				LayerType);
		friend void GhostGraph<NODE_PARAMS_SPEC, S>::link(
				Node<std::unique_ptr<SyncData<T>>, LayerType, N>*,
				GhostNode<NODE_PARAMS_SPEC, S>*,
				unsigned long,
				LayerType);

		private:
		GhostArc(
			unsigned long,
			GhostNode<NODE_PARAMS_SPEC, S>*,
			Node<std::unique_ptr<SyncData<T>>, LayerType, N>*,
			LayerType);
		GhostArc(
			unsigned long,
			Node<std::unique_ptr<SyncData<T>>, LayerType, N>*,
			GhostNode<NODE_PARAMS_SPEC, S>*,
			LayerType);

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
	template<NODE_PARAMS, SYNC_MODE> GhostArc<NODE_PARAMS_SPEC, S>::GhostArc(
			unsigned long arc_id,
			GhostNode<NODE_PARAMS_SPEC, S>* source,
			Node<std::unique_ptr<SyncData<T>>, LayerType, N>* target,
			LayerType layer
			)
		: Arc<std::unique_ptr<SyncData<T>>, LayerType, N>(arc_id, source, target, layer) { };

	/**
	 * Builds a GhostArc linking the specified nodes. Notice that the
	 * GhostArc instance is added to the regular outgoing arcs list of the
	 * local source node.
	 *
	 * @param arc_id arc id
	 * @param source pointer to the local source node
	 * @param target pointer to the target ghost node
	 */
	template<NODE_PARAMS, SYNC_MODE> GhostArc<NODE_PARAMS_SPEC, S>::GhostArc(
			unsigned long arc_id,
			Node<std::unique_ptr<SyncData<T>>, LayerType, N>* source,
			GhostNode<NODE_PARAMS_SPEC, S>* target,
			LayerType layer
			)
		: Arc<std::unique_ptr<SyncData<T>>, LayerType, N>(arc_id, source, target, layer) { };

}

#endif
