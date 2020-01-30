#ifndef DISTRIBUTED_GRAPH_H
#define DISTRIBUTED_GRAPH_H

#include "utils/log.h"

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
#include "communication/resource_container.h"

#include "synchro/sync_data.h"
#include "proxy/proxy.h"

namespace FPMAS {
	using communication::SyncMpiCommunicator;
	using graph::base::Graph;

	/**
	 * The FPMAS::graph::parallel contains all features relative to the
	 * parallelization and synchronization of a graph structure.
	 */
	namespace graph::parallel {
		namespace synchro {
			template<class T> class GhostData;
		}

		using proxy::Proxy;
		using synchro::SyncData;
		using synchro::SyncDataPtr;
		using synchro::GhostData;

		/** A DistributedGraph is a special graph instance that can be
		 * distributed across available processors using Zoltan.
		 *
		 * The synchronization mode can be set up thanks to an optional
		 * template parameter. Actually, the specified class represents the
		 * wrapper of data located on an other proc. Possible values are :
		 *
		 * - FPMAS::graph::synchro::None : no distant data representation. It
		 *   is not possible to access data from nodes on other procs. In
		 *   consequence, some arcs may be deleted when the graph is
		 *   distributed, what results in information loss.
		 *
		 * - FPMAS::graph::synchro::GhostData : Distant nodes are represented
		 *   as "ghosts", that can be punctually synchronized using the
		 *   GhostGraph::synchronize() function. When called, this function
		 *   asks updates to other procs and updates its local "ghost" data. In
		 *   this mode, no information is lost about the graph connectivity.
		 *   However, information accessed in the ghost nodes might not be up
		 *   to date. It is interesting to note that from a local node, there
		 *   is no difference between local or ghost nodes when exploring the
		 *   graph through incoming or outgoing arcs lists.
		 *
		 * - HardSyncData : TODO
		 *
		 * @tparam T associated data type
		 * @tparam S synchronization mode
		 */
		template<class T, template<typename> class S = GhostData> class DistributedGraph : public Graph<SyncDataPtr<T>>, communication::ResourceContainer {
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
			SyncMpiCommunicator mpiCommunicator;
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

			SyncMpiCommunicator& getMpiCommunicator();
			const SyncMpiCommunicator& getMpiCommunicator() const;

			Proxy& getProxy();

			GhostGraph<T, S>& getGhost();
			const GhostGraph<T, S>& getGhost() const;

			Node<SyncDataPtr<T>>* buildNode(unsigned long id, T data);
			Node<SyncDataPtr<T>>* buildNode(unsigned long id, float weight, T data);

			std::string getLocalData(unsigned long) const override;
			std::string getUpdatedData(unsigned long) const override;
			void updateData(unsigned long, std::string) override;

			void distribute();
			void synchronize();
		};

		/*
		 * Initializes zoltan parameters and zoltan lb query functions.
		 */
		template<class T, template<typename> class S> void DistributedGraph<T, S>::setUpZoltan() {
			FPMAS::config::zoltan_config(&this->zoltan);

			// Initializes Zoltan Node Load Balancing functions
			this->zoltan.Set_Num_Obj_Fn(FPMAS::graph::parallel::zoltan::num_obj<T, S>, this);
			this->zoltan.Set_Obj_List_Fn(FPMAS::graph::parallel::zoltan::obj_list<T, S>, this);
			this->zoltan.Set_Num_Edges_Multi_Fn(FPMAS::graph::parallel::zoltan::num_edges_multi_fn<T, S>, this);
			this->zoltan.Set_Edge_List_Multi_Fn(FPMAS::graph::parallel::zoltan::edge_list_multi_fn<T, S>, this);
		}

		/**
		 * Builds a DistributedGraph over all the available procs.
		 */
		template<class T, template<typename> class S> DistributedGraph<T, S>::DistributedGraph()
			: mpiCommunicator(*this), ghost(this), proxy(mpiCommunicator.getRank()), zoltan(mpiCommunicator.getMpiComm()) {
				this->setUpZoltan();
			}

		/**
		 * Builds a DistributedGraph over the specified procs.
		 *
		 * @param ranks ranks of the procs on which the DistributedGraph is
		 * built
		 */
		template<class T, template<typename> class S> DistributedGraph<T, S>::DistributedGraph(std::initializer_list<int> ranks)
			: mpiCommunicator(*this, ranks), ghost(this, ranks), proxy(mpiCommunicator.getRank(), ranks), zoltan(mpiCommunicator.getMpiComm()) {
				this->setUpZoltan();
			}

		/**
		 * Returns a reference to the MpiCommunicator used by this DistributedGraph.
		 *
		 * @return const reference to the mpiCommunicator associated to this graph
		 */
		template<class T, template<typename> class S> SyncMpiCommunicator& DistributedGraph<T, S>::getMpiCommunicator() {
			return this->mpiCommunicator;
		}

		template<class T, template<typename> class S> const SyncMpiCommunicator& DistributedGraph<T, S>::getMpiCommunicator() const {
			return this->mpiCommunicator;
		}
		/**
		 * Returns a reference to the proxy associated to this DistributedGraph.
		 *
		 * @return reference to the current proxy
		 */
		template<class T, template<typename> class S> Proxy& DistributedGraph<T, S>::getProxy() {
			return this->proxy;
		}

		/**
		 * Returns a reference to the GhostGraph currently associated to this
		 * DistributedGraph.
		 *
		 * @return reference to the current GhostGraph
		 */
		template<class T, template<typename> class S> GhostGraph<T, S>& DistributedGraph<T, S>::getGhost() {
			return this->ghost;
		}

		template<class T, template<typename> class S> const GhostGraph<T, S>& DistributedGraph<T, S>::getGhost() const {
			return this->ghost;
		}

		/**
		 * Configures Zoltan to migrate nodes.
		 */
		template<class T, template<typename> class S> void DistributedGraph<T, S>::setZoltanNodeMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::node::obj_size_multi_fn<T, S>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::node::pack_obj_multi_fn<T, S>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::node::unpack_obj_multi_fn<T, S>, this);
			zoltan.Set_Mid_Migrate_PP_Fn(NULL);
			zoltan.Set_Post_Migrate_PP_Fn(S<T>::config.node_post_migrate_fn, this);
		}

		/**
		 * Configures Zoltan to migrate arcs.
		 */
		template<class T, template<typename> class S> void DistributedGraph<T, S>::setZoltanArcMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T, S>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T, S>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T, S>, this);

			zoltan.Set_Mid_Migrate_PP_Fn(S<T>::config.arc_mid_migrate_fn, this);
			zoltan.Set_Post_Migrate_PP_Fn(S<T>::config.arc_post_migrate_fn, this);
		}

		/**
		 * Builds a node with the specified id and data.
		 *
		 * The specified data is implicitly wrapped in a LocalData instance,
		 * for synchronization purpose.
		 *
		 * @param id node id
		 * @param data node data (wrapped in a LocalData<T> instance)
		 */
		template<class T, template<typename> class S> Node<SyncDataPtr<T>>* DistributedGraph<T, S>::buildNode(unsigned long id, T data) {
			return Graph<SyncDataPtr<T>>::buildNode(id, SyncDataPtr<T>(new S<T>(id, this->getMpiCommunicator(), this->getProxy(), data)));
		}

		/**
		 * Builds a node with the specified id, weight and data.
		 *
		 * The specified data is implicitly wrapped in a LocalData instance,
		 * for synchronization purpose.
		 *
		 * @param id node id
		 * @param weight node weight
		 * @param data node data (wrapped in a LocalData<T> instance)
		 */
		template<class T, template<typename> class S> Node<SyncDataPtr<T>>* DistributedGraph<T, S>::buildNode(unsigned long id, float weight, T data) {
			return Graph<SyncDataPtr<T>>::buildNode(id, weight, SyncDataPtr<T>(new S<T>(id, this->getMpiCommunicator(), this->getProxy(), data)));
		}

		template<class T, template<typename> class S> std::string DistributedGraph<T, S>::getLocalData(unsigned long id) const {
			FPMAS_LOGV(getMpiCommunicator().getRank(), "GRAPH", "getLocalData %lu", id);
			return json(this->getNodes().at(id)->data()->get()).dump();
		}

		template<class T, template<typename> class S> std::string DistributedGraph<T, S>::getUpdatedData(unsigned long id) const {
			FPMAS_LOGV(getMpiCommunicator().getRank(), "GRAPH", "getUpdatedData %lu", id);
			return json(this->getGhost().getNode(id)->data()->get()).dump();
		}

		template<class T, template<typename> class S> void DistributedGraph<T, S>::updateData(unsigned long id, std::string data) {
			FPMAS_LOGV(getMpiCommunicator().getRank(), "GRAPH", "updateData %lu : %s", id, data.c_str());
			this->getNodes().at(id)->data()->get() = json::parse(data).get<T>();
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

			this->synchronize();
		}

		/**
		 * Synchronizes the DistributedGraph instances, calling the
		 * SyncMpiCommunicator::terminate() method and extra processing
		 * that might be required by the synchronization mode S (eg :
		 * synchronize the ghost graph data for the GhostData mode).
		 */
		template<class T, template<typename> class S> void DistributedGraph<T, S>::synchronize() {
			this->mpiCommunicator.terminate();
			S<T>::termination(this);
		}
	}
}

#endif
