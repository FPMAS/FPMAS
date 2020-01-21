#ifndef DISTRIBUTED_GRAPH_H
#define DISTRIBUTED_GRAPH_H

#include "../base/graph.h"
#include "zoltan_cpp.h"
#include "olz.h"
#include "olz_graph.h"
#include "zoltan/zoltan_lb.h"
#include "zoltan/zoltan_utils.h"
#include "zoltan/zoltan_node_migrate.h"
#include "zoltan/zoltan_arc_migrate.h"

#include "utils/config.h"
#include "communication/communication.h"

#include "sync_data.h"
#include "proxy.h"

namespace FPMAS {
	using communication::MpiCommunicator;

	namespace graph {

		using proxy::Proxy;
		using synchro::SyncData;
		using synchro::LocalData;
		using synchro::GhostData;

		using synchro::GhostGraph;

		using synchro::SyncData;
		using synchro::LocalData;
		using synchro::GhostData;

		/**
		 * A DistributedGraph is a special graph instance that can be
		 * distributed across available processors using Zoltan.
		 *
		 * @tparam T associated data type
		 */
		template<class T, template<typename> class S = GhostData> class DistributedGraph : public Graph<SyncData<T>> {
			friend void zoltan::node::obj_size_multi_fn<T, S>(ZOLTAN_OBJ_SIZE_ARGS);
			friend void zoltan::arc::obj_size_multi_fn<T, S>(ZOLTAN_OBJ_SIZE_ARGS);

			friend void zoltan::node::pack_obj_multi_fn<T, S>(ZOLTAN_PACK_OBJ_ARGS);
			friend void zoltan::arc::pack_obj_multi_fn<T, S>(ZOLTAN_PACK_OBJ_ARGS);

			friend void zoltan::node::unpack_obj_multi_fn<T, S>(ZOLTAN_UNPACK_OBJ_ARGS);
			friend void zoltan::arc::unpack_obj_multi_fn<T, S>(ZOLTAN_UNPACK_OBJ_ARGS);

			friend void zoltan::node::post_migrate_pp_fn_no_sync<T>(ZOLTAN_MID_POST_MIGRATE_ARGS);
			friend void zoltan::node::post_migrate_pp_fn_olz<T, S>(ZOLTAN_MID_POST_MIGRATE_ARGS);

			friend void zoltan::arc::mid_migrate_pp_fn<T, S>(ZOLTAN_MID_POST_MIGRATE_ARGS);
			friend void zoltan::arc::post_migrate_pp_fn_olz<T, S>(ZOLTAN_MID_POST_MIGRATE_ARGS);
			friend void zoltan::arc::post_migrate_pp_fn_no_sync<T>(ZOLTAN_MID_POST_MIGRATE_ARGS);

			private:
				MpiCommunicator mpiCommunicator;
				Zoltan zoltan;
				Proxy proxy;
				GhostGraph<T, S> ghost;

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
				// the real node has been imported. It is safely deleted in
				// arc::mid_migrate_pp_fn once associated arcs have eventually 
				// been exported
				std::set<unsigned long> obsoleteGhosts;

				// Arc migration buffers
				int export_arcs_num;
				ZOLTAN_ID_PTR export_arcs_global_ids;
				int* export_arcs_procs;
				void setUpZoltan();

			public:
				DistributedGraph<T, S>();
				DistributedGraph<T, S>(std::initializer_list<int>);
				MpiCommunicator getMpiCommunicator() const;
				Proxy* getProxy();
				GhostGraph<T, S>* getGhost();

				Node<SyncData<T>>* buildNode(unsigned long id, T data);
				Node<SyncData<T>>* buildNode(unsigned long id, float weight, T data);

				void distribute();
		};

		template<class T, template<typename> class S> void DistributedGraph<T, S>::setUpZoltan() {
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
		template<class T, template<typename> class S> DistributedGraph<T, S>::DistributedGraph()
			: ghost(this), proxy(mpiCommunicator.getRank()), zoltan(mpiCommunicator.getMpiComm()) {
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
		template<class T, template<typename> class S> DistributedGraph<T, S>::DistributedGraph(std::initializer_list<int> ranks)
			: mpiCommunicator(ranks), ghost(this, ranks), proxy(mpiCommunicator.getRank(), ranks), zoltan(mpiCommunicator.getMpiComm()) {
				this->setUpZoltan();
			}

		/**
		 * Returns the MpiCommunicator used by the Zoltan instance of this
		 * DistrDistributedGraph.
		 *
		 * @return mpiCommunicator associated to this graph
		 */
		template<class T, template<typename> class S> MpiCommunicator DistributedGraph<T, S>::getMpiCommunicator() const {
			return this->mpiCommunicator;
		}

		/**
		 * Returns a pointer to the proxy associated to this DistributedGraph.
		 *
		 * @return pointer to the current proxy
		 */
		template<class T, template<typename> class S> Proxy* DistributedGraph<T, S>::getProxy() {
			return &this->proxy;
		}

		/**
		 * Returns a pointer to the GhostGraph currently associated to this
		 * DistributedGraph.
		 *
		 * @return pointer to the current GhostGraph
		 */
		template<class T, template<typename> class S> GhostGraph<T, S>* DistributedGraph<T, S>::getGhost() {
			return &this->ghost;
		}

		/**
		 * Configures Zoltan to migrate nodes.
		 */
		template<class T, template<typename> class S> void DistributedGraph<T, S>::setZoltanNodeMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::node::obj_size_multi_fn<T>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::node::pack_obj_multi_fn<T>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::node::unpack_obj_multi_fn<T>, this);
			zoltan.Set_Mid_Migrate_PP_Fn(NULL);
			zoltan.Set_Post_Migrate_PP_Fn(S<T>::config.node_post_migrate_fn, this);
		}

		/**
		 * Configures Zoltan to migrate arcs.
		 */
		template<class T, template<typename> class S> void DistributedGraph<T, S>::setZoltanArcMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T>, this);

			zoltan.Set_Mid_Migrate_PP_Fn(S<T>::config.arc_mid_migrate_fn, this);
			zoltan.Set_Post_Migrate_PP_Fn(S<T>::config.arc_post_migrate_fn, this);
		}

		template<class T, template<typename> class S> Node<SyncData<T>>* DistributedGraph<T, S>::buildNode(unsigned long id, T data) {
			return Graph<SyncData<T>>::buildNode(id, LocalData<T>(data));
		}

		template<class T, template<typename> class S> Node<SyncData<T>>* DistributedGraph<T, S>::buildNode(unsigned long id, float weight, T data) {
			return Graph<SyncData<T>>::buildNode(id, weight, LocalData<T>(data));
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
		template<class T, template<typename> class S> void DistributedGraph<T, S>::distribute() {
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
