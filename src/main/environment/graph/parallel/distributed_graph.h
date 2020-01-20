#ifndef DISTRIBUTED_GRAPH_H
#define DISTRIBUTED_GRAPH_H

#include "../base/graph.h"
#include "zoltan_cpp.h"
#include "olz.h"
#include "zoltan/zoltan_lb.h"
#include "zoltan/zoltan_utils.h"
#include "zoltan/zoltan_node_migrate.h"
#include "zoltan/zoltan_arc_migrate.h"

#include "utils/config.h"
#include "communication/communication.h"

#include "proxy.h"

using FPMAS::communication::MpiCommunicator;
using FPMAS::graph::proxy::Proxy;

#define ZOLTAN_OBJ_SIZE_ARGS void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
#define ZOLTAN_PACK_OBJ_ARGS void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *
#define ZOLTAN_UNPACK_OBJ_ARGS void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
#define ZOLTAN_MID_POST_MIGRATE_ARGS \
	void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR , int *, \
    int *, int , ZOLTAN_ID_PTR , ZOLTAN_ID_PTR , int *, int *,int *


namespace FPMAS {
	namespace graph {

		template<class T> class GhostNode;
		template<class T> class GhostArc;
		template<class T> class GhostGraph;

		/**
		 * Describes the different graph synchronization modes available.
		 *
		 * Once the synchronization mode has been setup, required operations
		 * when migrating nodes are automatically performed.
		 */
		enum SyncMode {
			/**
			 * In this mode, no overlapping zone is used.
			 *
			 * When a node is exported, its connections with nodes that are not
			 * exported on the same process are lost.
			 */
			NONE,
			/**
			 * In this mode, overlapping zones (represented as a "ghost graph")
			 * are built and synchronized on each proc.
			 *
			 * When a node is exported, ghost arcs and nodes are built to keep
			 * consistency across processes, and ghost nodes data is fetched
			 * at each GhostGraph::synchronize() call.
			 */
			OLZ
		};

		/**
		 * A DistributedGraph is a special graph instance that can be
		 * distributed across available processors using Zoltan.
		 */
		template<class T> class DistributedGraph : public Graph<T> {
			friend void zoltan::node::obj_size_multi_fn<T>(ZOLTAN_OBJ_SIZE_ARGS);
			friend void zoltan::arc::obj_size_multi_fn<T>(ZOLTAN_OBJ_SIZE_ARGS);

			friend void zoltan::node::pack_obj_multi_fn<T>(ZOLTAN_PACK_OBJ_ARGS);
			friend void zoltan::arc::pack_obj_multi_fn<T>(ZOLTAN_PACK_OBJ_ARGS);

			friend void zoltan::node::unpack_obj_multi_fn<T>(ZOLTAN_UNPACK_OBJ_ARGS);
			friend void zoltan::arc::unpack_obj_multi_fn<T>(ZOLTAN_UNPACK_OBJ_ARGS);

			friend void zoltan::node::post_migrate_pp_fn_no_sync<T>(ZOLTAN_MID_POST_MIGRATE_ARGS);
			friend void zoltan::node::post_migrate_pp_fn_olz<T>(ZOLTAN_MID_POST_MIGRATE_ARGS);

			friend void zoltan::arc::mid_migrate_pp_fn<T>(ZOLTAN_MID_POST_MIGRATE_ARGS);
			friend void zoltan::arc::post_migrate_pp_fn_olz<T>(ZOLTAN_MID_POST_MIGRATE_ARGS);
			friend void zoltan::arc::post_migrate_pp_fn_no_sync<T>(ZOLTAN_MID_POST_MIGRATE_ARGS);

			private:
				SyncMode syncMode;
				ZOLTAN_POST_MIGRATE_PP_FN* node_post_migrate_fn; // Defined according to sync mode
				ZOLTAN_POST_MIGRATE_PP_FN* arc_post_migrate_fn; // Defined according to sync mode
				MpiCommunicator mpiCommunicator;
				Zoltan zoltan;
				Proxy proxy;
				GhostGraph<T> ghost;

				void setZoltanNodeMigration();
				void setZoltanArcMigration();

				// A set of node ids that are currently local and have been
				// imported from an other proc. Data memory of such a node
				// (ghost or local) have been allocated dynamically at
				// deserialization, and this set allows us to delete it when
				// those nodes are exported / removed.
				// std::set<unsigned long> importedNodeIds;

				// Serialization caches used to pack objects
				std::unordered_map<unsigned long, std::string> node_serialization_cache;
				std::unordered_map<unsigned long, std::string> arc_serialization_cache;

				/*
				 * Zoltan structures used to manage nodes and arcs migration
				 */
				// Node export buffer
				int export_node_num;
				ZOLTAN_ID_PTR export_node_global_ids;
				int* export_node_procs;
				// When importing nodes, obsolete ghost nodes are stored when
				// the real node has been imported. It is the safely deleted in
				// arc::mid_migrate_pp_fn once associated arcs have eventually 
				// been exported
				std::set<unsigned long> obsoleteGhosts;

				// Arc migration buffers
				int export_arcs_num;
				ZOLTAN_ID_PTR export_arcs_global_ids;
				int* export_arcs_procs;
				void setUpZoltan();

			public:
				DistributedGraph<T>(SyncMode syncMode = OLZ);
				DistributedGraph<T>(std::initializer_list<int>, SyncMode syncMode = OLZ);
				SyncMode getSyncMode() const;
				MpiCommunicator getMpiCommunicator() const;
				Proxy* getProxy();
				GhostGraph<T>* getGhost();

				void distribute();
		};

		template<class T> void DistributedGraph<T>::setUpZoltan() {
			switch(this->syncMode) {
				case NONE:
					this->node_post_migrate_fn = &FPMAS::graph::zoltan::node::post_migrate_pp_fn_no_sync<T>;
					this->arc_post_migrate_fn = &FPMAS::graph::zoltan::arc::post_migrate_pp_fn_no_sync<T>;
					break;
				case OLZ:
					this->node_post_migrate_fn = &FPMAS::graph::zoltan::node::post_migrate_pp_fn_olz<T>;
					this->arc_post_migrate_fn = &FPMAS::graph::zoltan::arc::post_migrate_pp_fn_olz<T>;
					break;
			}

			FPMAS::config::zoltan_config(&this->zoltan);

			// Initializes Zoltan Node Load Balancing functions
			this->zoltan.Set_Num_Obj_Fn(FPMAS::graph::zoltan::num_obj<T>, this);
			this->zoltan.Set_Obj_List_Fn(FPMAS::graph::zoltan::obj_list<T>, this);
			this->zoltan.Set_Num_Edges_Multi_Fn(FPMAS::graph::zoltan::num_edges_multi_fn<T>, this);
			this->zoltan.Set_Edge_List_Multi_Fn(FPMAS::graph::zoltan::edge_list_multi_fn<T>, this);
		}

		/**
		 * Builds a DistributedGraph over all the available procs, using the
		 * specified synchronization mode (default to OLZ).
		 * 
		 * @param syncMode synchronization mode
		 */
		template<class T> DistributedGraph<T>::DistributedGraph(SyncMode syncMode)
			: syncMode(syncMode), ghost(this), proxy(mpiCommunicator.getRank()), zoltan(mpiCommunicator.getMpiComm()) {
				this->setUpZoltan();
			}

		/**
		 * Builds a DistributedGraph over the specified procs, using the
		 * specified synchronization mode (default to OLZ).
		 *
		 * @param ranks ranks of the procs on which the DistributedGraph is
		 * built
		 * @param syncMode synchronization mode
		 */
		template<class T> DistributedGraph<T>::DistributedGraph(std::initializer_list<int> ranks, SyncMode syncMode)
			: syncMode(syncMode), mpiCommunicator(ranks), ghost(this, ranks), proxy(mpiCommunicator.getRank(), ranks), zoltan(mpiCommunicator.getMpiComm()) {
				this->setUpZoltan();
			}

		/**
		 * Returns the current synchronization mode.
		 *
		 * @return synchronization mode
		 */
		template<class T> SyncMode DistributedGraph<T>::getSyncMode() const {
			return this->syncMode;
		}

		/**
		 * Returns the MpiCommunicator used by the Zoltan instance of this
		 * DistrDistributedGraph.
		 *
		 * @return mpiCommunicator associated to this graph
		 */
		template<class T> MpiCommunicator DistributedGraph<T>::getMpiCommunicator() const {
			return this->mpiCommunicator;
		}

		/**
		 * Returns a pointer to the proxy associated to this DistributedGraph.
		 *
		 * @return pointer to the current proxy
		 */
		template<class T> Proxy* DistributedGraph<T>::getProxy() {
			return &this->proxy;
		}

		/**
		 * Returns a pointer to the GhostGraph currently associated to this
		 * DistributedGraph.
		 *
		 * @return pointer to the current GhostGraph
		 */
		template<class T> GhostGraph<T>* DistributedGraph<T>::getGhost() {
			return &this->ghost;
		}

		/**
		 * Configures Zoltan to migrate nodes.
		 */
		template<class T> void DistributedGraph<T>::setZoltanNodeMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::node::obj_size_multi_fn<T>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::node::pack_obj_multi_fn<T>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::node::unpack_obj_multi_fn<T>, this);
			zoltan.Set_Mid_Migrate_PP_Fn(NULL);
			zoltan.Set_Post_Migrate_PP_Fn(this->node_post_migrate_fn, this);
		}

		/**
		 * Configures Zoltan to migrate arcs.
		 */
		template<class T> void DistributedGraph<T>::setZoltanArcMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T>, this);

			if(this->syncMode == OLZ) {
				zoltan.Set_Mid_Migrate_PP_Fn(zoltan::arc::mid_migrate_pp_fn<T>, this);
			}
			else {
				zoltan.Set_Post_Migrate_PP_Fn(NULL);
			}

			zoltan.Set_Post_Migrate_PP_Fn(this->arc_post_migrate_fn, this);
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
		 * Because of the Zoltan behavior, the current process will block until
		 * all other involved process also call the distribute() function.
		 *
		 */
		// TODO: describe the full migration process
		template<class T> void DistributedGraph<T>::distribute() {
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


			if(changes > 0) {

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

				// Prepares Zoltan to migrate arcs associated to the exported
				// nodes.
				this->setZoltanArcMigration();

				// Unused buffer
				ZOLTAN_ID_TYPE export_arcs_local_ids[0];

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

			// Updates nodes locations according to last migrations
			this->proxy.synchronize();

			// When called for the first time, PARTITION is used, and so
			// REPARTITION will be used for all other calls.
			this->zoltan.Set_Param("LB_APPROACH", "REPARTITION");
		}
	}
}
#endif
