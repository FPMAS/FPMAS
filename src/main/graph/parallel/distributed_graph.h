#ifndef DISTRIBUTED_GRAPH_H
#define DISTRIBUTED_GRAPH_H

#include "utils/log.h"

#include "zoltan_cpp.h"
#include "olz.h"

#include "distributed_graph_base.h"

#include "utils/config.h"
#include "communication/communication.h"
#include "communication/resource_container.h"


namespace FPMAS {
	using communication::SyncMpiCommunicator;
	using graph::base::Graph;

	/**
	 * The FPMAS::graph::parallel contains all features relative to the
	 * parallelization and synchronization of a graph structure.
	 */
	namespace graph::parallel {
		namespace synchro::modes {
			template<typename, int> class GhostMode; 
		}

		using base::NodeId;
		using base::ArcId;
		using proxy::Proxy;
		using synchro::wrappers::SyncData;
		using synchro::modes::GhostMode;

		/** A DistributedGraph is a special graph instance that can be
		 * distributed across available processors using Zoltan.
		 *
		 * The synchronization mode can be set up thanks to an optional
		 * template parameter. Actually, the specified class represents the
		 * wrapper of data located on an other proc. Possible values are :
		 *
		 * - FPMAS::graph::synchro::modes::NoSyncMode : no distant data representation. It
		 *   is not possible to access data from nodes on other procs. In
		 *   consequence, some arcs may be deleted when the graph is
		 *   distributed, what results in information loss.
		 *
		 * - FPMAS::graph::synchro::modes::GhostMode : Distant nodes are represented
		 *   as "ghosts", that can be punctually synchronized using the
		 *   GhostGraph::synchronize() function. When called, this function
		 *   asks updates to other procs and updates its local "ghost" data. In
		 *   this mode, no information is lost about the graph connectivity.
		 *   However, information accessed in the ghost nodes might not be up
		 *   to date. It is interesting to note that from a local node, there
		 *   is no difference between local or ghost nodes when exploring the
		 *   graph through incoming or outgoing arcs lists.
		 *
		 * - FPMAS::graph::synchro::modes::HardSyncMode : TODO
		 *
		 * @tparam T associated data type
		 * @tparam S synchronization mode
		 * @tparam N layer count
		 */
		template<typename T, SYNC_MODE = GhostMode, int N = 1>
		class DistributedGraph : public DistributedGraphBase<T, S, N> {
			private:
				void distribute(int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int*, int*, ZOLTAN_ID_PTR, int*);

			public:
				using DistributedGraphBase<T,S,N>::link; // Allows usage of link(NodeId, NodeId, ArcId)
				using DistributedGraphBase<T,S,N>::unlink; // Allows usage of unlink(ArcId)

				DistributedGraph<T, S, N>();
				DistributedGraph<T, S, N>(std::initializer_list<int>);

				DistributedGraph<T, S, N>(const DistributedGraph<T,S,N>&) = delete;
				DistributedGraph<T, S, N>& operator=(const DistributedGraph<T, S, N>&) = delete;

				Arc<std::unique_ptr<SyncData<T,N,S>>, N>* link(NodeId, NodeId, ArcId, LayerId) override;
				void unlink(Arc<std::unique_ptr<SyncData<T,N,S>>, N>*) override;

				void distribute() override;
				void distribute(std::unordered_map<NodeId, std::pair<int, int>>) override;
				void synchronize() override;
		};

		/**
		 * Builds a DistributedGraph over all the available procs.
		 */
		template<class T, SYNC_MODE, int N> DistributedGraph<T, S, N>::DistributedGraph()
			: DistributedGraphBase<T,S,N>() {
			}

		/**
		 * Builds a DistributedGraph over the specified procs.
		 *
		 * @param ranks ranks of the procs on which the DistributedGraph is
		 * built
		 */
		template<class T, SYNC_MODE, int N> DistributedGraph<T, S, N>::DistributedGraph(std::initializer_list<int> ranks)
			: DistributedGraphBase<T, S, N>(ranks) {}

		/**
		 * Links the specified source and target node within this
		 * DistributedGraph on the given layer.
		 *
		 * Source and target might refer to local nodes or ghost nodes. If at
		 * least one of the two nodes is a ghost node, the request is
		 * transmetted through the synchro::SyncMode S to handle distant
		 * linking, through the implementation of the
		 * synchro::SyncMode::initLink() and synchro::SyncMode::notifyLinked()
		 * functions.
		 *
		 * In consequence, this function can be used to link two nodes distant
		 * from this process, each one potentially living on two different
		 * procs. However, those two nodes **must** be represented as GhostNode
		 * s at the scale of this DistributedGraph instance.
		 *
		 * @param source source node id
		 * @param target target node id
		 * @param arcId new arc id
		 * @param layerId id of the Layer on which nodes should be linked
		 * @return pointer to the created arc
		 */
		template<class T, SYNC_MODE, int N>
		Arc<std::unique_ptr<SyncData<T,N,S>>, N>* DistributedGraph<T, S, N>
		::link(NodeId source, NodeId target, ArcId arcId, LayerId layerId) {
			if(this->getNodes().count(source) > 0) {
				// Source is local
				Node<std::unique_ptr<SyncData<T,N,S>>, N>* sourceNode
					= this->getNode(source);
				if(this->getNodes().count(target) > 0) {
					// Source and Target are local, completely local operation
					Node<std::unique_ptr<SyncData<T,N,S>>, N>* targetNode
						= this->getNode(target);
					return this->Graph<std::unique_ptr<SyncData<T,N,S>>, N>::link(
						sourceNode, targetNode, arcId, layerId
						);
				} else {
					// Target is distant
					GhostNode<T, N, S>* targetNode
						= this->getGhost().getNode(target);
					this->syncMode.initLink(source, target, arcId, layerId);
					// link local -> distant
					auto arc = this->getGhost().link(
						sourceNode, targetNode, arcId, layerId
						);
					this->syncMode.notifyLinked(arc);
					return arc;
				}
			} else {
				// Source is distant
				if(this->getNodes().count(target) > 0) {
					// Source is local
					GhostNode<T, N, S>* sourceNode
						= this->getGhost().getNode(source);
					Node<std::unique_ptr<SyncData<T,N,S>>, N>* targetNode
						= this->getNode(target);
					this->syncMode.initLink(source, target, arcId, layerId);
					// link distant -> local
					auto arc = this->getGhost().link(
							sourceNode, targetNode, arcId, layerId
							);
					this->syncMode.notifyLinked(arc);
					return arc;
				}
				else {
					// Source and target are distant
					// Allowed only if the two nodes are ghosts on this graph
					// TODO: better exception handling
					GhostNode<T, N, S>* sourceNode
						= this->getGhost().getNode(source);
					GhostNode<T, N, S>* targetNode
						= this->getGhost().getNode(target);

					this->syncMode.initLink(source, target, arcId, layerId);
					// link distant -> distant
					auto arc = this->getGhost().link(
							sourceNode, targetNode, arcId, layerId
							);
					this->syncMode.notifyLinked(arc);
					return arc;
				}
			}
		}

		/**
		 * Unlinks the specified arc within this DistributedGraph instance.
		 *
		 * The specified arc pointer might point to a local Arc, or to a
		 * GhostArc. This allows to directly unlink arcs from incoming /
		 * outgoing arcs lists of nodes without considering if they are local
		 * or distant. For example, the following code is perfectly valid and
		 * does not depend on the current location of `arc->getSourceNode()`.
		 *
		 * ```cpp
		 * DistributedGraph<int> dg;
		 *
		 * // ...
		 *
		 * for(auto arc : dg.getNode(1)->getIncomingArcs()) {
		 * 	if(arc->getSourceNode()->data()->read() > 10) {
		 * 		dg.unlink(arc);
		 * 	}
		 * }
		 * ```
		 * If the arc is local, the operation is performed completely locally.
		 * If the arc links a distant node (i.e. is a GhostArc), some
		 * additionnal distant operation might apply depending on the current
		 * SyncMode used.
		 *
		 * More precisely :
		 *   1. synchro::modes::SyncMode::initUnlink is called
		 *   2. The `arc` is locally unlinked and deleted
		 *   3. synchro::modes::SyncMode::notifyUnlinked is called
		 *
		 * @param arc pointer to the arc to unlink (might be a local Arc or distant
		 * GhostArc)
		 */
		template<typename T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::unlink(
				Arc<std::unique_ptr<SyncData<T,N,S>>,N>* arc
			) {
			ArcId id = arc->getId();
			FPMAS_LOGD(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Unlinking arc %lu", id);
			try {
				// Arc is local
				this->Graph<std::unique_ptr<SyncData<T,N,S>>, N>::unlink(arc);
			} catch(base::exceptions::arc_out_of_graph e) {
				NodeId source = arc->getSourceNode()->getId();
				NodeId target = arc->getTargetNode()->getId();
				ArcId arcId = arc->getId();
				LayerId layer = arc->layer;

				this->syncMode.initUnlink(arc);

				this->getGhost().unlink((GhostArc<T,N,S>*) arc);
				this->syncMode.notifyUnlinked(source, target, arcId, layer);
			}
			FPMAS_LOGD(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Unlinked arc %lu", id);
		}

		/*
		 * private distribute functions, calling low level zoltan migration
		 * functions.
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::distribute(
			int changes,
			int num_import,
			ZOLTAN_ID_PTR import_global_ids,
			ZOLTAN_ID_PTR import_local_ids,
			int* import_procs,
			int* import_to_part,
			ZOLTAN_ID_PTR export_local_ids,
			int* export_to_part
			) {
			if(changes > 0) {

				FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Zoltan Node migration...");
				// Migrate nodes from the load balancing
				// Arcs to export are computed in the post_migrate_pp_fn step.
				this->zoltan.Migrate(
						num_import,
						import_global_ids,
						import_local_ids,
						import_procs,
						import_to_part,
						this->export_node_num,
						this->export_node_global_ids,
						export_local_ids,
						this->export_node_procs,
						export_to_part
						);
				FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Zoltan Node migration done.");

				// Prepares Zoltan to migrate arcs associated to the exported
				// nodes.
				this->setZoltanArcMigration();

				// Unused buffer
				ZOLTAN_ID_TYPE export_arcs_local_ids[0];

				FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Zoltan Arc migration...");
				// Arcs to import
				this->zoltan.Migrate(
						-1,
						NULL,
						NULL,
						NULL,
						NULL,
						this->export_arcs_num,
						this->export_arcs_global_ids,
						export_arcs_local_ids,
						this->export_arcs_procs,
						this->export_arcs_procs // parts = procs
						);
				FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Zoltan Arc migration done.");
				std::free(this->export_arcs_global_ids);
				std::free(this->export_arcs_procs);
			}

			// Frees the zoltan buffers
			this->zoltan.LB_Free_Part(
					&import_global_ids,
					&import_local_ids,
					&import_procs,
					&import_to_part
					);

			this->zoltan.LB_Free_Part(
					&this->export_node_global_ids,
					&export_local_ids,
					&this->export_node_procs,
					&export_to_part
					);

			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Synchronizing proxy...");
			// Updates nodes locations according to last migrations
			this->proxy.synchronize();

			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Synchronizing graph...");
			this->synchronize();

			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Migration done.");
		}

		/**
		 * Distributed the graph accross the available cored using the
		 * specified partition.
		 *
		 * The partition is built as follow :
		 * - `{node_id : {current_location, new_location}}`
		 * - *The same partition must be passed to all the procs* (that must
		 *   all call the distribute() function)
		 * - If a node's id is not specified, its location is unchanged.
		 *
		 * @param partition new partition
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::distribute(
				std::unordered_map<NodeId, std::pair<int, int>> partition
				) {

			// Import lists
			int num_import = 0;
			ZOLTAN_ID_PTR import_global_ids = (ZOLTAN_ID_PTR) std::malloc(0);
			int* import_procs = (int*) std::malloc(0);

			// Export lists
			this->export_node_num = 0;
			this->export_node_global_ids = (ZOLTAN_ID_PTR) std::malloc(0);
			this->export_node_procs = (int*) std::malloc(0);

			for(auto item : partition) {
				if(item.second.first == this->getMpiCommunicator().getRank()) {
					if(item.second.second != this->getMpiCommunicator().getRank()) {
						this->export_node_num++;
						this->export_node_global_ids = (ZOLTAN_ID_PTR)
							std::realloc(this->export_node_global_ids, 2 * this->export_node_num * sizeof(ZOLTAN_ID_TYPE));

						write_zoltan_id(item.first, &this->export_node_global_ids[2*(this->export_node_num-1)]);
						this->export_node_procs = (int*)
							std::realloc(this->export_node_procs, this->export_node_num * sizeof(int));
						this->export_node_procs[this->export_node_num-1] = item.second.second;
					}

				} else {
					if(item.second.second == this->getMpiCommunicator().getRank()) {
						num_import++;
						import_global_ids = (ZOLTAN_ID_PTR)
							std::realloc(import_global_ids, 2 * num_import * sizeof(ZOLTAN_ID_TYPE));
						write_zoltan_id(item.first, &import_global_ids[2*(num_import-1)]);
						import_procs = (int*)
							std::realloc(import_procs, num_import * sizeof(int));
						import_procs[num_import-1] = item.second.first;

					}

				}
			}
			int* import_to_part = (int*) std::malloc(num_import * sizeof(int));
			for (int i = 0; i < num_import; ++i) {
				import_to_part[i] = import_procs[i];
			}
			int* export_to_part = (int*) std::malloc(this->export_node_num * sizeof(int));
			for (int i = 0; i < this->export_node_num; ++i) {
				export_to_part[i] = this->export_node_procs[i];
			}

			ZOLTAN_ID_PTR import_local_ids = (ZOLTAN_ID_PTR) std::malloc(0);
			ZOLTAN_ID_PTR export_local_ids = (ZOLTAN_ID_PTR) std::malloc(0);

			this->setZoltanNodeMigration();

			this->distribute(
					1,
					num_import,
					import_global_ids,
					import_local_ids,
					import_procs,
					import_to_part,
					export_local_ids,
					export_to_part);
		}
		/**
		 * Distributes the graph accross the available cores using Zoltan.
		 *
		 * The graph is distributed using
		 * [Zoltan::LB_Partition](https://cs.sandia.gov/Zoltan/ug_html/ug_interface_lb.html#Zoltan_LB_Partition)
		 * and
		 * [Zoltan::Migrate](https://cs.sandia.gov/Zoltan/ug_html/ug_interface_mig.html#Zoltan_Migrate).
		 * Query functions used are defined in the FPMAS::graph::zoltan namespace.
		 *
		 * The current process will block until all other involved procs also
		 * call the distribute() function.
		 *
		 * The distribution process is organized as follow :
		 *
		 * 1. Compute partitions
		 * 2. Migrate nodes
		 * 3. Migrate associated arcs, and build ghost nodes if necessary
		 * 4. Update nodes locations within each Proxy instance
		 *
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::distribute() {
			int changes;
			int num_gid_entries;
			int num_lid_entries;

			int num_import; 
			ZOLTAN_ID_PTR import_global_ids;
			ZOLTAN_ID_PTR import_local_ids;
			int * import_procs;
			int * import_to_part;

			int* export_to_part;
			ZOLTAN_ID_PTR export_local_ids;

			// Prepares Zoltan to migrate nodes
			// Must be set up from there, because LB_Partition can call
			// obj_size_multi_fn when repartitionning
			this->setZoltanNodeMigration();

			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Computing Load Balancing...");

			// Computes Zoltan partitioning
			int err = this->zoltan.LB_Partition(
					changes,
					num_gid_entries,
					num_lid_entries,
					num_import,
					import_global_ids,
					import_local_ids,
					import_procs,
					import_to_part,
					this->export_node_num,
					this->export_node_global_ids,
					export_local_ids,
					this->export_node_procs,
					export_to_part
					);
			this->distribute(
				changes,
				num_import,
				import_global_ids,
				import_local_ids,
				import_procs,
				import_to_part,
				export_local_ids,
				export_to_part);


			// When called for the first time, PARTITION is used, and so
			// REPARTITION will be used for all other calls.
			this->zoltan.Set_Param("LB_APPROACH", "REPARTITION");
		}

		/**
		 * Synchronizes the DistributedGraph instances, calling the
		 * SyncMpiCommunicator::terminate() method and extra processing
		 * that might be required by the synchronization mode S, implemented
		 * through the synchro::SyncMode::termination() function. (eg :
		 * synchronize the ghost graph data for the GhostData mode).
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::synchronize() {
			this->mpiCommunicator.terminate();
			this->syncMode.termination();
		}
	}
}

#endif
