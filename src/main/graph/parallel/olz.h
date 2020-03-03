#ifndef OLZ_H
#define OLZ_H

#include "../base/graph.h"
#include "synchro/sync_data.h"
#include "synchro/local_data.h"
#include "synchro/ghost_data.h"

using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

namespace FPMAS::graph::parallel {

	using base::LayerId;
	using synchro::SyncData;
	using synchro::GhostData;

	template<typename T, int N, SYNC_MODE> class GhostGraph;

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
	template <typename T, int N, SYNC_MODE> class GhostNode : public Node<std::unique_ptr<SyncData<T>>, N> {
		friend GhostNode<T, N, S>* GhostGraph<T, N, S>
			::buildNode(unsigned long);
		friend GhostNode<T, N, S>* GhostGraph<T, N, S>
			::buildNode(const Node<std::unique_ptr<SyncData<T>>, N>&, std::set<unsigned long>);

		private:
		GhostNode(SyncMpiCommunicator&, Proxy&, unsigned long);
		GhostNode(SyncMpiCommunicator&, Proxy&, const Node<std::unique_ptr<SyncData<T>>, N>&);
	};

	template<typename T, int N, SYNC_MODE> GhostNode<T, N, S>
		::GhostNode(SyncMpiCommunicator& mpiComm, Proxy& proxy, unsigned long id)
		: Node<std::unique_ptr<SyncData<T>>, N>(
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
	template<typename T, int N, SYNC_MODE> GhostNode<T, N, S>
		::GhostNode(
			SyncMpiCommunicator& mpiComm,
			Proxy& proxy,
			const Node<std::unique_ptr<SyncData<T>>, N>& node
			)
		: Node<std::unique_ptr<SyncData<T>>, N>(
			node.getId(),
			node.getWeight(),
			std::unique_ptr<SyncData<T>>(new S<T>(
				node.getId(), mpiComm, proxy, std::move(node.data()->get())
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
	template<typename T, int N, SYNC_MODE> class GhostArc : public Arc<std::unique_ptr<SyncData<T>>, N> {
		friend void GhostGraph<T, N, S>::link(
				GhostNode<T, N, S>*,
				Node<std::unique_ptr<SyncData<T>>, N>*,
				unsigned long,
				LayerId);
		friend void GhostGraph<T, N, S>::link(
				Node<std::unique_ptr<SyncData<T>>, N>*,
				GhostNode<T, N, S>*,
				unsigned long,
				LayerId);

		private:
		GhostArc(
			unsigned long,
			GhostNode<T, N, S>*,
			Node<std::unique_ptr<SyncData<T>>, N>*,
			LayerId);
		GhostArc(
			unsigned long,
			Node<std::unique_ptr<SyncData<T>>, N>*,
			GhostNode<T, N, S>*,
			LayerId);

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
	template<typename T, int N, SYNC_MODE> GhostArc<T, N, S>::GhostArc(
			unsigned long arc_id,
			GhostNode<T, N, S>* source,
			Node<std::unique_ptr<SyncData<T>>, N>* target,
			LayerId layer
			)
		: Arc<std::unique_ptr<SyncData<T>>, N>(arc_id, source, target, layer) { };

	/**
	 * Builds a GhostArc linking the specified nodes. Notice that the
	 * GhostArc instance is added to the regular outgoing arcs list of the
	 * local source node.
	 *
	 * @param arc_id arc id
	 * @param source pointer to the local source node
	 * @param target pointer to the target ghost node
	 */
	template<typename T, int N, SYNC_MODE> GhostArc<T, N, S>::GhostArc(
			unsigned long arc_id,
			Node<std::unique_ptr<SyncData<T>>, N>* source,
			GhostNode<T, N, S>* target,
			LayerId layer
			)
		: Arc<std::unique_ptr<SyncData<T>>, N>(arc_id, source, target, layer) { };

}

#endif
