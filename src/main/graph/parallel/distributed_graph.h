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

		using proxy::Proxy;
		using synchro::wrappers::SyncData;
		using synchro::modes::GhostMode;

		/**
		 * A DistributedGraph is a special graph instance that can be
		 * distributed across available processors using Zoltan.
		 *
		 * The synchronization mode can be set up thanks to an optional
		 * template parameter. Actually, the specified class represents the
		 * wrapper of data located on an other proc. Possible values are :
		 *
		 * - FPMAS::graph::parallel::synchro::modes::NoSyncMode :
		 *
		 *   No distant data representation. It is not possible to access data
		 *   from nodes on other procs. In consequence, some arcs may be
		 *   deleted when the graph is distributed, what results in information
		 *   loss.
		 *
		 * - FPMAS::graph::parallel::synchro::modes::GhostMode :
		 *
		 *   Distant nodes are represented as "ghosts", that can be punctually
		 *   synchronized using the GhostGraph::synchronize() function. When
		 *   called, this function asks updates to other procs and updates its
		 *   local "ghost" data. In this mode, no information is lost about the
		 *   graph connectivity.  However, information accessed in the ghost
		 *   nodes might not be up to date. It is interesting to note that from
		 *   a local node, there is no difference between local or ghost nodes
		 *   when exploring the graph through incoming or outgoing arcs lists.
		 *
		 * - FPMAS::graph::parallel::synchro::modes::HardSyncMode : TODO
		 *
		 * @tparam T associated data type
		 * @tparam S synchronization mode
		 * @tparam N layer count
		 */
		template<typename T, SYNC_MODE = GhostMode, int N = 1>
		class DistributedGraph : public DistributedGraphBase<T, S, N> {
			public:
				/**
				 * Node pointer type.
				 */
				typedef typename DistributedGraphBase<T, S, N>::node_ptr node_ptr;
				/**
				 * Arc pointer type.
				 */
				typedef typename DistributedGraphBase<T, S, N>::arc_ptr arc_ptr;

			private:
				void distributeArcs();
				void balance(int&, int&, ZOLTAN_ID_PTR&, ZOLTAN_ID_PTR&, int*&, int*&, ZOLTAN_ID_PTR&, int*&);

			public:
				using DistributedGraphBase<T,S,N>::link; // Allows usage of link(DistributedId, DistributedId)
				using DistributedGraphBase<T,S,N>::unlink; // Allows usage of unlink(DistributedId)

				DistributedGraph<T, S, N>();
				DistributedGraph<T, S, N>(std::initializer_list<int>);

				DistributedGraph<T, S, N>(const DistributedGraph<T,S,N>&) = delete;
				DistributedGraph<T, S, N>& operator=(const DistributedGraph<T, S, N>&) = delete;

				arc_ptr link(node_ptr, node_ptr, LayerId) override;
				void unlink(arc_ptr) override;

				void distribute() override;
				void distribute(std::unordered_map<DistributedId, int>) override;
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
		template<class T, SYNC_MODE, int N> DistributedGraph<T, S, N>
			::DistributedGraph(std::initializer_list<int> ranks)
			: DistributedGraphBase<T, S, N>(ranks) {}

		/**
		 * Links the specified source and target node within this
		 * DistributedGraph on the given layer.
		 *
		 * Source and target might refer to local nodes or ghost nodes. If at
		 * least one of the two nodes is a ghost node, the request is
		 * transmitted through the currently used synchro::modes::SyncMode.
		 *
		 * More precisely :
		 *   1. synchro::modes::SyncMode::initLink is called
		 *   2. The two nodes are locally linked
		 *   3. synchro::modes::SyncMode::notifyLinked is called
		 *
		 * In consequence, this function can be used to link two distant nodes,
		 * each one potentially living on two different procs. However, those
		 * two nodes **must** be represented as GhostNode s at the scale of
		 * this DistributedGraph instance.
		 *
		 * @param source source node id
		 * @param target target node id
		 * @param layerId id of the Layer on which nodes should be linked
		 * @return pointer to the created arc
		 */
		template<class T, SYNC_MODE, int N>
		typename DistributedGraph<T, S, N>::arc_ptr DistributedGraph<T, S, N>
		::link(node_ptr source, node_ptr target, LayerId layerId) {
			if(this->getNodes().count(source->getId()) > 0) {
				// Source is local
				if(this->getNodes().count(target->getId()) > 0) {
					// Source and Target are local, completely local operation
					return this->Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>::link(
						source, target, layerId
						);
				} else {
					// Target is distant
					DistributedId id = this->currentArcId++;
					this->syncMode.initLink(source->getId(), target->getId(), id, layerId);
					// link local -> distant
					auto arc = this->getGhost().link(
						source, (GhostNode<T, N, S>*) target, id, layerId
						);
					this->syncMode.notifyLinked(arc);
					return arc;
				}
			} else {
				// Source is distant
				if(this->getNodes().count(target->getId()) > 0) {
					// Source is local
					DistributedId id = this->currentArcId++;
					this->syncMode.initLink(source->getId(), target->getId(), id, layerId);
					// link distant -> local
					auto arc = this->getGhost().link(
							(GhostNode<T, N, S>*) source, target, id, layerId
							);
					this->syncMode.notifyLinked(arc);
					return arc;
				}
				else {
					// Source and target are distant
					// Allowed only if the two nodes are ghosts on this graph
					// TODO: better exception handling
					DistributedId id = this->currentArcId++;
					this->syncMode.initLink(source->getId(), target->getId(), id, layerId);
					// link distant -> distant
					auto arc = this->getGhost().link(
							(GhostNode<T, N, S>*) source, (GhostNode<T, N, S>*) target, id, layerId
							);
					this->syncMode.notifyLinked(arc);
					return arc;
				}
			}
		}
		/**
		 * Unlinks the specified arc within this DistributedGraph instance.
		 *
		 * If the arc is local, the operation is performed completely locally.
		 * If the arc links a distant node (i.e. is a GhostArc), some
		 * additionnal distant operation might apply depending on the current
		 * synchro::modes::SyncMode used.
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
				arc_ptr arc
			) {
			DistributedId id = arc->getId();
			FPMAS_LOGD(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Unlinking arc %s", ID_C_STR(id));
			if(this->getArcs().count(arc->getId())>0) {
				// Arc is local
				this->Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>::unlink(arc);
			} else {
				DistributedId source = arc->getSourceNode()->getId();
				DistributedId target = arc->getTargetNode()->getId();
				DistributedId arcId = arc->getId();
				LayerId layer = arc->layer;

				this->syncMode.initUnlink(arc);

				this->getGhost().unlink((GhostArc<T,N,S>*) arc);
				this->syncMode.notifyUnlinked(source, target, arcId, layer);
			}
			FPMAS_LOGD(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Unlinked arc %s", ID_C_STR(id));
		}

		/*
		 * private distribute functions, calling low level zoltan migration
		 * functions.
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::distributeArcs() {
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
			// Frees arcs buffers
			std::free(this->export_arcs_global_ids);
			std::free(this->export_arcs_procs);

			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Synchronizing proxy...");
			// Updates nodes locations according to last migrations
			this->proxy.synchronize();

			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Synchronizing graph...");
			this->synchronize();

			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Migration done.");
		}

		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::balance(
				int& num_gid_entries,
				int& num_import,
				ZOLTAN_ID_PTR& import_global_ids,
				ZOLTAN_ID_PTR& import_local_ids,
				int*& import_procs,
				int*& import_to_part,
				ZOLTAN_ID_PTR& export_local_ids,
				int*& export_to_part
			) {

			int changes;
			// Prepares Zoltan to migrate nodes
			// Must be set up from there, because LB_Partition can call
			// obj_size_multi_fn when repartitionning
			this->setZoltanNodeMigration();

			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Computing Load Balancing...");

			int num_lid_entries;

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
		}
		/**
		 * DistributedGraphBase::distribute(std::unordered_map<DistributedId,
		 * std::pair<int, int>>) implementation.
		 *
		 * @param partition new partition
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::distribute(
				std::unordered_map<DistributedId, int> partition
				) {
			// Export lists
			this->export_node_num = 0;
			this->export_node_global_ids = (ZOLTAN_ID_PTR) std::malloc(0);
			this->export_node_procs = (int*) std::malloc(0);

			for(auto item : partition) {
				if(this->getNodes().count(item.first) > 0) {
					if(item.second != this->getMpiCommunicator().getRank()) {
						this->export_node_num++;
						this->export_node_global_ids = (ZOLTAN_ID_PTR)
							std::realloc(this->export_node_global_ids, 2 * this->export_node_num * sizeof(ZOLTAN_ID_TYPE));

						write_zoltan_id(item.first, &this->export_node_global_ids[2*(this->export_node_num-1)]);
						this->export_node_procs = (int*)
							std::realloc(this->export_node_procs, this->export_node_num * sizeof(int));
						this->export_node_procs[this->export_node_num-1] = item.second;
					}
				}
			}
			ZOLTAN_ID_TYPE export_local_ids[0];

			this->setZoltanNodeMigration();

			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Zoltan Node migration...");
			// Migrate nodes from the load balancing
			// Arcs to export are computed in the post_migrate_pp_fn step.
			this->zoltan.Migrate(
					-1,
					NULL,
					NULL,
					NULL,
					NULL,
					this->export_node_num,
					this->export_node_global_ids,
					export_local_ids,
					this->export_node_procs,
					this->export_node_procs // Parts = procs
					);
			FPMAS_LOGV(this->mpiCommunicator.getRank(), "DIST_GRAPH", "Zoltan Node migration done.");

			this->distributeArcs();

			// DistributeArcs uses export_node_global_ids, so free the buffers
			// after
			std::free(this->export_node_global_ids);
			std::free(this->export_node_procs);
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
			int num_gid_entries;

			int num_import; 
			ZOLTAN_ID_PTR import_global_ids;
			ZOLTAN_ID_PTR import_local_ids;
			int * import_procs;
			int * import_to_part;

			int* export_to_part;
			ZOLTAN_ID_PTR export_local_ids;

			auto periods = this->scheduler.gatherPeriods(
						this->getMpiCommunicator().getMpiComm());
			int free=false;
			for(auto schedule_period : periods) {
				for(auto n : this->scheduler.get(schedule_period)) {
					this->toBalance.insert(n);
				}
				if(free) {
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
				}
				free = true;
				balance(
					num_gid_entries,
					num_import,
					import_global_ids,
					import_local_ids,
					import_procs,
					import_to_part,
					export_local_ids,
					export_to_part
					);
				assert(num_gid_entries == 2);
				auto nodesToFix = this->scheduler.get(schedule_period);

				// By default, fixes the local nodes to this proc
				// Nodes that are not in the "export list" have implicitly been
				// assigned to this proc, so they should be fixed to it.
				for(auto nodeToFix : nodesToFix) {
					this->fixedVertices[nodeToFix->getId()]
						= this->getMpiCommunicator().getRank();

				}

				// Nodes to import are not "owned" by this proc, so they will
				// be fixed as "exported nodes" by their owning procs
				
				// Fixes nodes to export to the part where they should have
				// been exported. Even if they are not exported yet, the next
				// iterations of the load balancing algorithm will behave as if
				// they were already on the corresponding procs.
				for (int i = 0; i < this->export_node_num; i++) {
					auto nodeId = zoltan::utils::read_zoltan_id(
							&this->export_node_global_ids[i * num_gid_entries]);
					if(this->getNodes().count(nodeId) > 0
						&& nodesToFix.count(this->getNode(nodeId)) > 0) {
						this->fixedVertices[nodeId] = this->export_node_procs[i];
					}
				}
			}
			this->fixedVertices.clear();
			this->toBalance.clear();

			// The final load balancing is migrated in a single operation
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


			this->distributeArcs();

			// Frees node buffers
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

			// When called for the first time, PARTITION is used, and so
			// REPARTITION will be used for all other calls.
			this->zoltan.Set_Param("LB_APPROACH", "REPARTITION");
		}

		/**
		 * Synchronizes the DistributedGraph instances.
		 *
		 * More precisely:
		 * 	1. SyncMpiCommunicator::terminate() is called
		 * 	2. synchro::modes::SyncMode::termination() is called
		 *
		 * 	@see synchro::modes::GhostMode::termination()
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::synchronize() {
			this->DistributedGraphBase<T, S, N>::synchronize();
			this->syncMode.termination();
		}
	}
}

#endif
