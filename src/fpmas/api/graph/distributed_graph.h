#ifndef FPMAS_DISTRIBUTED_GRAPH_API_H
#define FPMAS_DISTRIBUTED_GRAPH_API_H

/** \file src/fpmas/api/graph/distributed_graph.h
 * DistributedGraph API
 */

#include "fpmas/api/communication/communication.h"
#include "fpmas/api/graph/location_manager.h"
#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/api/synchro/sync_mode.h"

namespace fpmas {namespace api {namespace graph {

	/**
	 * Node related event.
	 */
	template<typename T>
		struct NodeEvent {
			/**
			 * Node concerned by the event.
			 */
			DistributedNode<T>* node;

			/**
			 * NodeEvent constructor.
			 */
			NodeEvent(DistributedNode<T>* node) : node(node) {
			}
		};

	/**
	 * Event triggered when a node is set \LOCAL within a DistributedGraph.
	 *
	 * Such event can be handled with callbacks provided to
	 * DistributedGraph::addCallOnSetLocal().
	 *
	 * In addition to the node instance, information about the context of the
	 * event are provided.
	 */
	template<typename T>
		struct SetLocalNodeEvent : public NodeEvent<T> {
			/**
			 * Specifies the cause of the set \LOCAL operation.
			 */
			enum Context {
				/**
				 * Used when a node is set \LOCAL due to a
				 * DistributedGraph::buildNode() operation.
				 */
				BUILD_LOCAL,
				/**
				 * Used when a new \LOCAL node is imported in the graph, in the
				 * context of a DistributedGraph::distribute() operation.
				 */
				IMPORT_NEW_LOCAL,
				/**
				 * Used when a \LOCAL node is imported in the graph while the
				 * corresponding \DISTANT node was already contained in the
				 * graph. In this case, the existing \DISTANT node is set to
				 * \LOCAL. The node pointer contained in the SetLocalNodeEvent
				 * references the existing node, after its weight and data as
				 * been updated from the imported node.
				 */
				IMPORT_EXISTING_LOCAL,
				/**
				 * Can be used when the event context is unspecified or does
				 * not falls in the other explicit contexts. Can also be useful
				 * for tests that does not depend on the context.
				 */
				UNSPECIFIED
			};

			/**
			 * Event context.
			 */
			Context context;

			/**
			 * SetLocalNodeEvent constructor.
			 */
			SetLocalNodeEvent<T>(
					DistributedNode<T>* node,
					Context context
					) : NodeEvent<T>(node), context(context) {
			}
		};

	/**
	 * Event triggered when a node is set \DISTANT within a DistributedGraph.
	 *
	 * Such event can be handled with callbacks provided to
	 * DistributedGraph::addCallOnSetDistant().
	 *
	 * In addition to the node instance, information about the context of the
	 * event are provided.
	 */
	template<typename T>
		struct SetDistantNodeEvent : public NodeEvent<T> {
			/**
			 * Specifies the cause of the set \DISTANT operation.
			 */
			enum Context {
				/**
				 * Used when a \DISTANT node is inserted in the graph as a
				 * source or target node of a \DISTANT edge.
				 */
				IMPORT_NEW_DISTANT,
				/**
				 * Used when a \LOCAL node is exported because of a
				 * DistributedGraph::distribute() operation, so that the \LOCAL node
				 * becomes \DISTANT.
				 */
				EXPORT_DISTANT,
				/**
				 * Can be used when the event context is unspecified or does
				 * not falls in the other explicit contexts. Can also be useful
				 * for tests that does not depend on the context.
				 */
				UNSPECIFIED

			};

			/**
			 * Event context.
			 */
			Context context;

			/**
			 * SetDistantNodeEvent constructor.
			 */
			SetDistantNodeEvent<T>(
					DistributedNode<T>* node,
					Context context
					) : NodeEvent<T>(node), context(context) {
			}
		};

	/**
	 * DistributedGraph API.
	 *
	 * The DistributedGraph is an extension of the Graph API, specialized using
	 * DistributedNode and DistributedEdge, and introduces some distribution
	 * related concepts.
	 *
	 * A DistributedGraph is actually a graph distributed across the available
	 * MPI processes. The continuity of the graph is ensured thanks to a
	 * concept of "distant" nodes representations used to represent the borders
	 * of the local graph and allow interactions with nodes located on other
	 * processes.
	 *
	 * Features are also provided to dynamically distribute and synchronize the
	 * graph across processes.
	 *
	 * More particularly, the following **distant** functions are introduced :
	 * - buildNode()
	 * - removeNode()
	 * - link()
	 * - unlink()
	 *
	 * Such functions are considered "distant", in the sense that they might
	 * involve other processes or at least influence the global state of the
	 * graph.
	 *
	 * @tparam T node data type
	 */
	template<typename T>
	class DistributedGraph 
		: public virtual fpmas::api::graph::Graph<DistributedNode<T>, DistributedEdge<T>> {
		public:
			using typename fpmas::api::graph::Graph<DistributedNode<T>, DistributedEdge<T>>::NodeMap;
			using typename fpmas::api::graph::Graph<DistributedNode<T>, DistributedEdge<T>>::EdgeMap;

			/**
			 * Callback API to handle SetLocalNodeEvent.
			 */
			typedef api::utils::EventCallback<SetLocalNodeEvent<T>> SetLocalNodeCallback;
			/**
			 * Callback API to handle SetDistantNodeEvent.
			 */
			typedef api::utils::EventCallback<SetDistantNodeEvent<T>> SetDistantNodeCallback;

		public:
			/**
			 * Reference to the internal MpiCommunicator.
			 *
			 * @return internal MpiCommunicator
			 */
			virtual api::communication::MpiCommunicator& getMpiCommunicator() = 0;

			/**
			 * \copydoc getMpiCommunicator
			 */
			virtual const api::communication::MpiCommunicator& getMpiCommunicator() const = 0;

			/**
			 * Reference to the internal LocationManager.
			 *
			 * The LocationManager is notably used to easily access the \LOCAL
			 * and \DISTANT nodes in independent sets, thanks to the
			 * LocationManager::getLocalNodes() and
			 * LocationManager::getDistantNodes() functions.
			 *
			 * @return internal LocationManager
			 */
			virtual LocationManager<T>& getLocationManager() = 0;

			/**
			 * \copydoc getLocationManager()
			 */
			virtual const LocationManager<T>& getLocationManager() const = 0;

			/**
			 * Imports a DistributedNode into the local graph instance.
			 *
			 * When a node is imported into the graph, two main events occur :
			 * - the node is *inserted* into the graph, triggering insert node
			 *   callbacks, as specified in Graph::insert(NodeType*).
			 * - the node becomes *\LOCAL* to this graph, replacing an eventual
			 *   outdated *\DISTANT* representation of this node.
			 *
			 * Finally, the inserted node is returned.
			 *
			 * @param node node to import
			 * @return inserted node
			 */
			virtual DistributedNode<T>* importNode(DistributedNode<T>* node) = 0;

			/**
			 * Imports a \DistributedEdge into the local graph instance.
			 *
			 * The \DistributedEdge must comply with the following requirements:
			 * - dynamically allocated
			 * - defined getTempSourceNode() and getTempTargetNode()
			 *
			 * Moreover, at least the source or the target node of the imported
			 * edge must represent a \LOCAL node (since edges between two
			 * \DISTANT nodes are not represented in a \DistributedGraph
			 * anyway).
			 *
			 * Several situations might then occur : 
			 * - source and target nodes are \LOCAL, then edge is \LOCAL
			 * - source is \LOCAL and target is \DISTANT, then edge is \DISTANT
			 * - target is \LOCAL and source is \DISTANT, then edge is \DISTANT
			 *
			 * When source or target is \DISTANT, the corresponding node might
			 * already be represented in the local graph instance. In that
			 * case, the imported edge is linked to the existing
			 * representation, and the corresponding TemporaryNode is discarded
			 * (without a call to the TemporaryNode::build() method).
			 *
			 * Else, if no \DISTANT representation of the node is available, a
			 * \DISTANT representation is created into the local graph instance
			 * and the edge is linked to this new \DISTANT node, built using
			 * the TemporaryNode::build() method.
			 *
			 * Notice that a \DISTANT representation of the imported edge might
			 * already exist, notably if the two nodes are \LOCAL and at least
			 * one of them was imported in the current distribute() operation.
			 * In that case, the imported edge can be discarded and the edge
			 * already contained in the graph goes \LOCAL, without calling
			 * insert edge callbacks. Source and target TemporaryNodes are also
			 * be discarded.
			 *
			 * @param edge to import
			 * @return inserted edge
			 */
			virtual DistributedEdge<T>* importEdge(DistributedEdge<T>* edge) = 0;

			/**
			 * Returns a buffer containing nodes that have not been
			 * synchronized yet using the getDataSync() component.
			 *
			 * This set corresponds to nodes that were created while
			 * importing an importEdge() or insertDistant() statement.
			 *
			 * Nodes are added to the buffer when importEdge() or
			 * insertDistant() is called, each node is removed under two conditions:
			 * 1. When its data is synchronized with a call to
			 * `getDataSync().synchronize()` or
			 * `getDataSync().synchronize(node_set)`, that are themselves
			 * called by synchronize() and synchronize(std::unordered_set nodes).
			 * 2. When it is erased from the graph
			 *
			 * Notice that what is called "data synchronization" there is still
			 * dependent on the current \SyncMode: this does not necessarily
			 * mean message exchanges or data import.
			 *
			 * @return unsynchronized nodes buffer
			 */
			virtual std::unordered_set<DistributedNode<T>*> getUnsyncNodes() const = 0;

			/**
			 * Returns the current internal Node ID.
			 *
			 * The next node built with buildNode() will have an id equal to
			 * currentNodeId()++;
			 *
			 * @return current Node ID
			 */
			virtual DistributedId currentNodeId() const = 0;

			/**
			 * Sets the current internal Node ID to the specified value.
			 *
			 * @param id node id
			 */
			virtual void setCurrentNodeId(DistributedId id) = 0;

			/**
			 * Returns the current internal Edge ID.
			 *
			 * The next edge built with link() will have an id equal to
			 * currentEdgeId()++;
			 *
			 * @return current Edge ID
			 */
			virtual DistributedId currentEdgeId() const = 0;

			/**
			 * Sets the current internal Edge ID to the specified value.
			 *
			 * @param id edge id
			 */
			virtual void setCurrentEdgeId(DistributedId id) = 0;

			/**
			 * Builds a new \LOCAL node into the graph.
			 *
			 * The specified data is *moved* into the built node.
			 *
			 * The node is inserted into the graph, triggering insert node
			 * events as specified in Graph::insert(NodeType*).
			 *
			 * @param data node data
			 * @return pointer to built node
			 */
			virtual DistributedNode<T>* buildNode(T&& data) = 0;

			/**
			 * Builds a new \LOCAL node into the graph.
			 *
			 * The specified data is *copied* to the built node.
			 *
			 * The node is inserted into the graph, triggering insert node
			 * events as specified in Graph::insert(NodeType*).
			 *
			 * @param data node data
			 * @return pointer to built node
			 */
			virtual DistributedNode<T>* buildNode(const T& data) = 0;

			/**
			 * Inserts a temporary \DISTANT node into the graph.
			 *
			 * The node is guaranteed to live at least until the next
			 * synchronize() call.
			 *
			 * Once inserted, the temporary `node` can eventually be used as
			 * any other \DISTANT node, and so can be linked to existing \LOCAL
			 * and \DISTANT nodes or to query data.
			 *
			 * However, the DistributedNode::location() must be initialized
			 * manually, otherwise operations above might produce unexpected
			 * results.
			 *
			 * The `node` must be dynamically allocated, and the
			 * DistributedGraph implementation automatically takes its
			 * ownership.
			 *
			 * If a node with the same ID is already contained in the graph,
			 * the node is simply deleted and a pointer to the existing node is
			 * returned. Else, a pointer to the inserted node is returned.
			 *
			 * This method can notably be used to implement distributed graph
			 * initialization algorithms. Indeed, if a node 0 is built on
			 * process 0 and node 1 is built on process 1, it is at first
			 * glance impossible to create a link from node 0 to node 1.
			 *
			 * But, for example knowing that the node 1 is necessarily
			 * instanciated on process 1, it is possible to manually insert a
			 * \DISTANT representation of node 1 using insertDistant() on
			 * process 0, what allows to build a link from node 0 to node 1 on
			 * process 0. Such link will be committed at the latest at the next
			 * synchronize() call and imported on process 1.
			 *
			 *
			 * @param node temporary \DISTANT node to manually insert in the
			 * graph
			 */
			virtual DistributedNode<T>* insertDistant(DistributedNode<T>* node) = 0;

			/**
			 * Globally removes the specified node from the graph.
			 *
			 * The removed node might be \DISTANT or \LOCAL. The node is not
			 * guaranteed to be effectively removed upon return, since it might
			 * be involved in inter-process communications, depending on the
			 * synchronization mode.
			 *
			 * However, it is guaranteed that upon return of the next
			 * synchronize() call, the node (and its connected edges) are
			 * completely removed from **all** the processes.
			 *
			 * In any case, Erase node callbacks, as specified in
			 * Graph::erase(NodeType*), are called only when the node is
			 * effectively erased from the graph.
			 *
			 * The node is assumed to belong to the graph, behavior is
			 * unspecified otherwise.
			 *
			 * @param node pointer to node to remove
			 */
			virtual void removeNode(DistributedNode<T>* node) = 0;
			/**
			 * Globally removes the node with the specified id from the graph.
			 *
			 * Equivalent to
			 * ```cpp
			 * removeNode(getNode(id));
			 * ```
			 *
			 * @see removeNode(NodeType*)
			 * @param id of the node to remove
			 */
			virtual void removeNode(DistributedId id) = 0;

			/**
			 * Globally unlinks the specified edge.
			 *
			 * The specified edge might be \LOCAL or \DISTANT.
			 *
			 * In consequence the edge is not guaranteed to be effectively
			 * unlinked upon return, since it might be involved in
			 * inter-process communications, depending on the synchronization
			 * mode.
			 *
			 * However, it is guaranteed that upon return of the next
			 * synchronize() call, the edge is completely removed from **all**
			 * the processes.
			 *
			 * In any case, Erase edge callbacks, as specified in
			 * Graph::erase(EdgeType*), are called only when the edge is
			 * effectively erased from the graph.
			 *
			 * @param edge edge to remove
			 */
			virtual void unlink(DistributedEdge<T>* edge) = 0;
			/**
			 * Globally unlinks the edge with the specified id.
			 *
			 * Equivalent to
			 * ```cpp
			 * unlink(getEdge(id));
			 * ```
			 *
			 * @see unlink(EdgeType*)
			 * @param id of the edge to unlink
			 */
			virtual void unlink(DistributedId id) = 0;

			/**
			 * Links source to target on the specified layer.
			 *
			 * The two nodes might be \LOCAL, only one of them might be \DISTANT,
			 * or **both of them might be \DISTANT**. In this last case, it is
			 * however required that the two \DISTANT representations of source
			 * and target nodes already exist in the local graph, i.e. each
			 * node is already connected to at least one \LOCAL node.
			 *
			 * In consequence the edge is not guaranteed to be inserted in the
			 * graph upon return, since its creation might require
			 * inter-process communications, depending on the synchronization
			 * mode.
			 *
			 * However, it is guaranteed that upon return of the next
			 * synchronize() call, the edge is properly inserted in the graphs
			 * hosted by the processes that might require it, i.e. the
			 * processes that host at least one of its source and target nodes.
			 *
			 * In any case, insert edge callbacks, as specified in
			 * Graph::insert(EdgeType*), are called only when the edge is
			 * effectively inserted into the graph.
			 *
			 * @param source source node
			 * @param target target node
			 * @param layer_id layer id
			 */
			virtual DistributedEdge<T>* link(
					DistributedNode<T>* source, DistributedNode<T>* target,
					LayerId layer_id) = 0;

			/**
			 * Switches the edge to the specified layer.
			 *
			 * Currently, this operation is only available for \LOCAL edges.
			 *
			 * The edge is properly re-linked to its source and target node,
			 * while the Edge::setLayer() method does not.
			 *
			 * @param edge edge to switch
			 * @param layer_id new layer
			 */
			virtual void switchLayer(DistributedEdge<T>* edge, LayerId layer_id) = 0;

			/**
			 * Adds a set \LOCAL node callback.
			 *
			 * A DistributedNode is set \LOCAL in the following situations :
			 * - the node is imported (with importNode())
			 * - the node is built in the local graph (with buildNode())
			 *
			 * @param callback set local node callback
			 */
			virtual void addCallOnSetLocal(SetLocalNodeCallback* callback) = 0;

			/**
			 * Current set local callbacks list.
			 *
			 * @return set local callbacks
			 */
			virtual std::vector<SetLocalNodeCallback*> onSetLocalCallbacks() const = 0;

			/**
			 * Adds a set \DISTANT callback.
			 *
			 * A DistributedNode is set \DISTANT in the following situations :
			 * - the node is exported to an other process (i.e. in a
			 *   distribute() call) AND persist in the graph (i.e. at least one
			 *   \LOCAL node is still connected to this node)
			 * - the node is created as a missing source or target in the
			 *   importEdge process
			 *
			 * @param callback set distant node callback
			 */
			virtual void addCallOnSetDistant(SetDistantNodeCallback* callback) = 0;

			/**
			 * Current set distant callbacks list.
			 *
			 * @return set distant callbacks
			 */
			virtual std::vector<SetDistantNodeCallback*> onSetDistantCallbacks() const = 0;

			
			/**
			 * @deprecated
			 * Deprecated in favor of balance(LoadBalancing<T>&, PartitionMode)
			 */
			HEDLEY_DEPRECATED_FOR(1.2, balance(LoadBalancing<T>&, PartitionMode))
			virtual void balance(LoadBalancing<T>& lb) = 0;

			/**
			 * Balances the graph across processors using the specified load
			 * balancing algorithm and partitioning strategy.
			 *
			 * This function act as a synchronization barrier, so **all** the
			 * process must enter this function to ensure progress.
			 *
			 * @param lb load balancing algorithm
			 * @param partition_mode partitioning strategy
			 */
			virtual void balance(LoadBalancing<T>& lb, PartitionMode partition_mode) = 0;

			
			/**
			 * @deprecated
			 * Deprecated in favor of balance(FixedVerticesLoadBalancing<T>&, PartitionMap, PartitionMode)
			 */
			HEDLEY_DEPRECATED_FOR(1.2, balance(FixedVerticesLoadBalancing<T>&, PartitionMap, PartitionMode))
			virtual void balance(
					FixedVerticesLoadBalancing<T>& lb,
					PartitionMap fixed_vertices) = 0;

			/**
			 * Balances the graph using the specified load balancing algorithm
			 * with fixed vertices support.
			 *
			 * This function act as a synchronization barrier, so **all** the
			 * process must enter this function to ensure progress.
			 *
			 * @param lb load balancing algorithm
			 * @param fixed_vertices fixed vertices map
			 * @param partition_mode partitioning strategy
			 */
			virtual void balance(
					FixedVerticesLoadBalancing<T>& lb,
					PartitionMap fixed_vertices,
					PartitionMode partition_mode) = 0;

			/**
			 * Distributes the Graph across processors according to the
			 * specified partition.
			 *
			 * The partition describes nodes to *export*. In consequence,
			 * entries specified for nodes that are not \LOCAL to this graph are
			 * ignored.
			 *
			 * The partition is specified as a `{node_id, process_rank}` map.
			 *
			 * The process then handles node serialization and migrations.
			 * Exported nodes becomes \DISTANT.
			 *
			 * @param partition export partition
			 */
			virtual void distribute(PartitionMap partition) = 0;

			/**
			 * Synchronizes the DistributedGraph.
			 *
			 * This function acts as a synchronization barrier : **all**
			 * processes blocks until all other perform a synchronize() call.
			 *
			 * The synchronization process involves two main steps :
			 * 1. Link and Removed node synchronization (using
			 * synchronizationMode().getSyncLinker().synchronize())
			 * 2. Node Data synchronization (using
			 * synchronizationMode().getDataSync().synchronize())
			 *
			 * Different synchronization modes might define different
			 * policies to perform those synchronizations.
			 *
			 * In any case, as specified in the corresponding function, it is
			 * guaranteed that :
			 * - links created with link() are committed, i.e. they are
			 *   properly reported on other processes potentially involved, and
			 *   added to the local graph if not done yet.
			 * - links destroyed with unlink() are properly removed from
			 *   **all** the processes
			 * - nodes removed with removeNode() are properly removed from
			 *   **all** the processes
			 *
			 * Since all nodes are synchronized, the getUnsyncNodes() buffer is
			 * cleared.
			 */
			virtual void synchronize() = 0;

			/**
			 * Partially synchronizes the graph, as specified in the
			 * synchronize() method, except that the
			 * synchro::DataSync::synchronize(std::unordered_set<api::graph::DistributedNode<T>*>) method is used with the
			 * specified nodes instead.
			 *
			 * Links synchronization is regularly performed with
			 * `synchronizationMode().getSyncLinker().synchronize()`, unless
			 * `synchronize_links` is `false`.
			 *
			 * `nodes` are **not** required to be contained in getUnsyncNodes(),
			 * but are removed from this buffer if they do.
			 *
			 * The specified `nodes` list might contain \LOCAL and \DISTANT
			 * nodes: it is the responsibility of the implemented
			 * synchro::DataSync to eventually not do anything with \LOCAL
			 * nodes.
			 *
			 * @param nodes nodes to synchronize
			 * @param synchronize_links specifies if
			 * synchro::SyncLinker::synchronize() must be called
			 */
			virtual void synchronize(
					std::unordered_set<DistributedNode<T>*> nodes,
					bool synchronize_links = true
					) = 0;


			/**
			 * Returns the current synchro::SyncMode instance used to
			 * synchronize() the graph.
			 *
			 * Directly accessing
			 * synchronizationMode().getSyncLinker().synchronize() or
			 * synchronizationMode().getDataSync().synchronize() can be used to
			 * perform corresponding partial synchronizations (contrary to
			 * synchronize()) for performance purpose.
			 */
			virtual synchro::SyncMode<T>& synchronizationMode() = 0;
		};
}}}
#endif
