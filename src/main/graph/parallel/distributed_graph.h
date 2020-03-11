#ifndef DISTRIBUTED_GRAPH_H
#define DISTRIBUTED_GRAPH_H

#include "utils/log.h"

#include "../base/graph.h"
#include "zoltan_cpp.h"
#include "olz.h"
#include "olz_graph.h"
#include "zoltan/zoltan_lb.h"
#include "zoltan/zoltan_node_migrate.h"
#include "zoltan/zoltan_arc_migrate.h"

#include "utils/config.h"
#include "communication/communication.h"
#include "communication/resource_container.h"

#include "synchro/sync_mode.h"


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
		template<typename T, SYNC_MODE = GhostMode, int N = 1>
		class DistributedGraph : public Graph<std::unique_ptr<SyncData<T,N,S>>, N>, communication::ResourceContainer {
			friend void zoltan::node::obj_size_multi_fn<T, N, S>(ZOLTAN_OBJ_SIZE_ARGS);
			friend void zoltan::arc::obj_size_multi_fn<T, N, S>(ZOLTAN_OBJ_SIZE_ARGS);

			friend void zoltan::node::pack_obj_multi_fn<T, N, S>(ZOLTAN_PACK_OBJ_ARGS);
			friend void zoltan::arc::pack_obj_multi_fn<T, N, S>(ZOLTAN_PACK_OBJ_ARGS);

			friend void zoltan::node::unpack_obj_multi_fn<T, N, S>(ZOLTAN_UNPACK_OBJ_ARGS);
			friend void zoltan::arc::unpack_obj_multi_fn<T, N, S>(ZOLTAN_UNPACK_OBJ_ARGS);

			friend void zoltan::node::post_migrate_pp_fn_no_sync<T, N>(ZOLTAN_MID_POST_MIGRATE_ARGS);
			friend void zoltan::node::post_migrate_pp_fn_olz<T, N, S>(ZOLTAN_MID_POST_MIGRATE_ARGS);

			friend void zoltan::arc::mid_migrate_pp_fn<T, N, S>(ZOLTAN_MID_POST_MIGRATE_ARGS);
			friend void zoltan::arc::post_migrate_pp_fn_olz<T, N, S>(ZOLTAN_MID_POST_MIGRATE_ARGS);
			friend void zoltan::arc::post_migrate_pp_fn_no_sync<T, N>(ZOLTAN_MID_POST_MIGRATE_ARGS);

			private:
			SyncMpiCommunicator mpiCommunicator;
			Zoltan zoltan;
			Proxy proxy;
			S<T, N> syncMode;
			GhostGraph<T, N, S> ghost;

			void distribute(int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int*, int*, ZOLTAN_ID_PTR, int*);
			void setZoltanNodeMigration();
			void setZoltanArcMigration();

			// Serialization caches used to pack objects
			std::unordered_map<NodeId, std::string> node_serialization_cache;
			std::unordered_map<ArcId, std::string> arc_serialization_cache;

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
			std::set<NodeId> obsoleteGhosts;

			// Arc migration buffers
			int export_arcs_num;
			ZOLTAN_ID_PTR export_arcs_global_ids;
			int* export_arcs_procs;
			void setUpZoltan();

			public:
			DistributedGraph<T, S, N>();
			DistributedGraph<T, S, N>(std::initializer_list<int>);

			/**
			 * Returns a reference to the SyncMpiCommunicator used by this DistributedGraph.
			 *
			 * @return reference to the SyncMpiCommunicator associated to this graph
			 */
			SyncMpiCommunicator& getMpiCommunicator();

			/**
			 * Returns a const reference to the MpiCommunicator used by this
			 * DistributedGraph.
			 *
			 * @return const reference to the mpiCommunicator associated to this graph
			 */
			const SyncMpiCommunicator& getMpiCommunicator() const;


			/**
			 * Returns a reference to the GhostGraph currently associated to this
			 * DistributedGraph.
			 *
			 * @return reference to the current GhostGraph
			 */
			GhostGraph<T, N, S>& getGhost();

			/**
			 * Returns a const reference to the GhostGraph currently associated to this
			 * DistributedGraph.
			 *
			 * @return const reference to the current GhostGraph
			 */
			const GhostGraph<T, N, S>& getGhost() const;

			Proxy& getProxy();

			Node<std::unique_ptr<SyncData<T,N,S>>, N>* buildNode(NodeId, T&& data);
			Node<std::unique_ptr<SyncData<T,N,S>>, N>* buildNode(NodeId id, T& data);
			Node<std::unique_ptr<SyncData<T,N,S>>, N>* buildNode(NodeId id, float weight, T&& data);
			Node<std::unique_ptr<SyncData<T,N,S>>, N>* buildNode(NodeId id, float weight, T& data);

			Arc<std::unique_ptr<SyncData<T,N,S>>, N>* link(NodeId, NodeId, ArcId);
			Arc<std::unique_ptr<SyncData<T,N,S>>, N>* link(NodeId, NodeId, ArcId, LayerId);

			std::string getLocalData(unsigned long) const override;
			std::string getUpdatedData(unsigned long) const override;
			void updateData(unsigned long, std::string) override;

			void distribute();
			void distribute(std::unordered_map<NodeId, std::pair<int, int>>);
			void synchronize();
		};

		/*
		 * Initializes zoltan parameters and zoltan lb query functions.
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::setUpZoltan() {
			FPMAS::config::zoltan_config(&this->zoltan);

			// Initializes Zoltan Node Load Balancing functions
			this->zoltan.Set_Num_Obj_Fn(FPMAS::graph::parallel::zoltan::num_obj<T, N, S>, this);
			this->zoltan.Set_Obj_List_Fn(FPMAS::graph::parallel::zoltan::obj_list<T, N, S>, this);
			this->zoltan.Set_Num_Edges_Multi_Fn(FPMAS::graph::parallel::zoltan::num_edges_multi_fn<T, N, S>, this);
			this->zoltan.Set_Edge_List_Multi_Fn(FPMAS::graph::parallel::zoltan::edge_list_multi_fn<T, N, S>, this);
		}

		/**
		 * Builds a DistributedGraph over all the available procs.
		 */
		template<class T, SYNC_MODE, int N> DistributedGraph<T, S, N>::DistributedGraph()
			: syncMode(*this), mpiCommunicator(*this), ghost(this), proxy(mpiCommunicator.getRank()), zoltan(mpiCommunicator.getMpiComm()) {
				this->setUpZoltan();
			}

		/**
		 * Builds a DistributedGraph over the specified procs.
		 *
		 * @param ranks ranks of the procs on which the DistributedGraph is
		 * built
		 */
		template<class T, SYNC_MODE, int N> DistributedGraph<T, S, N>::DistributedGraph(std::initializer_list<int> ranks)
			: syncMode(*this), mpiCommunicator(*this, ranks), ghost(this, ranks), proxy(mpiCommunicator.getRank(), ranks), zoltan(mpiCommunicator.getMpiComm()) {
				this->setUpZoltan();
			}


		template<class T, SYNC_MODE, int N> SyncMpiCommunicator& DistributedGraph<T, S, N>::getMpiCommunicator() {
			return this->mpiCommunicator;
		}

		template<class T, SYNC_MODE, int N> const SyncMpiCommunicator& DistributedGraph<T, S, N>::getMpiCommunicator() const {
			return this->mpiCommunicator;
		}
		/**
		 * Returns a reference to the proxy associated to this DistributedGraph.
		 *
		 * @return reference to the current proxy
		 */
		template<class T, SYNC_MODE, int N> Proxy& DistributedGraph<T, S, N>::getProxy() {
			return this->proxy;
		}

		template<class T, SYNC_MODE, int N> GhostGraph<T, N, S>& DistributedGraph<T, S, N>::getGhost() {
			return this->ghost;
		}

		template<class T, SYNC_MODE, int N> const GhostGraph<T, N, S>& DistributedGraph<T, S, N>::getGhost() const {
			return this->ghost;
		}

		/**
		 * Configures Zoltan to migrate nodes.
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::setZoltanNodeMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::node::obj_size_multi_fn<T, N, S>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::node::pack_obj_multi_fn<T, N, S>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::node::unpack_obj_multi_fn<T, N, S>, this);
			zoltan.Set_Mid_Migrate_PP_Fn(NULL);
			zoltan.Set_Post_Migrate_PP_Fn(syncMode.config().node_post_migrate_fn, this);
		}

		/**
		 * Configures Zoltan to migrate arcs.
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::setZoltanArcMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T, N, S>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T, N, S>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T, N, S>, this);

			zoltan.Set_Mid_Migrate_PP_Fn(syncMode.config().arc_mid_migrate_fn, this);
			zoltan.Set_Post_Migrate_PP_Fn(syncMode.config().arc_post_migrate_fn, this);
		}

		/**
		 * Builds a node with the specified id and data.
		 *
		 * The specified data is moved and implicitly wrapped in a SyncData
		 * instance, for synchronization purpose. This functions ensures that
		 * data instances that are MoveConstructible and MoveAssignable but not
		 * either CopyConstructible or CopyAssignable, such as std::unique_ptr
		 * instances, can be used in the DistributedGraph.
		 *
		 * @param id node id
		 * @param data node data
		 */
		template<class T, SYNC_MODE, int N>
		Node<std::unique_ptr<SyncData<T,N,S>>, N>* DistributedGraph<T, S, N>
		::buildNode(NodeId id, T&& data) {
			return Graph<std::unique_ptr<SyncData<T,N,S>>, N>
				::buildNode(
					id,
					std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							id,
							this->getMpiCommunicator(),
							this->getProxy(),
							std::move(data)
						))
					);
		}

		/**
		 * Builds a node with the specified id and data.
		 *
		 * The specified data is copied and implicitly wrapped in a SyncData
		 * instance, for synchronization purpose.
		 *
		 * @param id node id
		 * @param data node data
		 */
		template<class T, SYNC_MODE, int N>
		Node<std::unique_ptr<SyncData<T,N,S>>, N>* DistributedGraph<T, S, N>
		::buildNode(NodeId id, T& data) {
			return Graph<std::unique_ptr<SyncData<T,N,S>>, N>
				::buildNode(
					id,
					std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							id,
							this->getMpiCommunicator(),
							this->getProxy(),
							data
						))
					);
		}

		/**
		 * Builds a node with the specified id, weight and data.
		 *
		 * The specified data is moved and implicitly wrapped in a SyncData
		 * instance, for synchronization purpose. This functions ensures that
		 * data instances that are MoveConstructible and MoveAssignable but not
		 * either CopyConstructible or CopyAssignable, such as std::unique_ptr
		 * instances, can be used in the DistributedGraph.
		 *
		 * @param id node id
		 * @param weight node weight
		 * @param data node data
		 */
		template<class T, SYNC_MODE, int N>
		Node<std::unique_ptr<SyncData<T,N,S>>, N>* DistributedGraph<T, S, N>
		::buildNode(NodeId id, float weight, T&& data) {
			return Graph<std::unique_ptr<SyncData<T,N,S>>, N>
				::buildNode(
					id,
					weight,
					std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							id,
							this->getMpiCommunicator(),
							this->getProxy(),
							std::move(data)
						))
					);
		}

		/**
		 * Builds a node with the specified id, weight and data.
		 *
		 * The specified data is copied and implicitly wrapped in a SyncData
		 * instance, for synchronization purpose.
		 *
		 * @param id node id
		 * @param weight node weight
		 * @param data node data
		 */
		template<class T, SYNC_MODE, int N>
		Node<std::unique_ptr<SyncData<T,N,S>>, N>* DistributedGraph<T, S, N>
		::buildNode(NodeId id, float weight, T& data) {
			return Graph<std::unique_ptr<SyncData<T,N,S>>, N>
				::buildNode(
					id,
					weight,
					std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							id,
							this->getMpiCommunicator(),
							this->getProxy(),
							data
						))
					);
		}

		/**
		 * Links the specified source and target node within this
		 * DistributedGraph on the given layer on the DefaultLayer.
		 *
		 * Same as `link(source, target, arcId, base::DefaultLayer)`.
		 *
		 * @param source source node id
		 * @param target target node id
		 * @param arcId new arc id
		 * @return pointer to the created arc
		 *
		 * @see link(NodeId, NodeId, ArcId, LayerId)
		 */
		template<class T, SYNC_MODE, int N>
		Arc<std::unique_ptr<SyncData<T,N,S>>, N>* DistributedGraph<T, S, N>
		::link(NodeId source, NodeId target, ArcId arcId) {
			return this->link(source, target, arcId, base::DefaultLayer);
		}

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
					syncMode.initLink(source, target, arcId, layerId);
					// link local -> distant
					auto arc = this->getGhost().link(
						sourceNode, targetNode, arcId, layerId
						);
					syncMode.notifyLinked(arc);
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
					syncMode.initLink(source, target, arcId, layerId);
					// link distant -> local
					auto arc = this->getGhost().link(
							sourceNode, targetNode, arcId, layerId
							);
					syncMode.notifyLinked(arc);
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

					syncMode.initLink(source, target, arcId, layerId);
					// link distant -> distant
					auto arc = this->getGhost().link(
							sourceNode, targetNode, arcId, layerId
							);
					syncMode.notifyLinked(arc);
					return arc;
				}
			}
		}


		/**
		 * ResourceContainer implementation.
		 *
		 * Serializes the *local node* corresponding to id.
		 *
		 * @return serialized data contained in the node corresponding to id
		 */
		template<class T, SYNC_MODE, int N> std::string DistributedGraph<T, S, N>::getLocalData(unsigned long id) const {
			FPMAS_LOGV(getMpiCommunicator().getRank(), "GRAPH", "getLocalData %lu", id);
			return json(this->getNodes().at(id)->data()->get()).dump();
		}

		/**
		 * ResourceContainer implementation.
		 *
		 * Serializes the *ghost data* corresponding to id.
		 * If the current proc modifies distant data that it has acquired, such
		 * data will actually be contained in a GhostNode and modifications
		 * will be applied within the GhostNode.
		 *
		 * @return serialized updates to the data corresponding to id
		 */
		template<class T, SYNC_MODE, int N> std::string DistributedGraph<T, S, N>::getUpdatedData(unsigned long id) const {
			FPMAS_LOGV(getMpiCommunicator().getRank(), "GRAPH", "getUpdatedData %lu", id);
			return json(this->getGhost().getNode(id)->data()->get()).dump();
		}

		/**
		 * ResourceContainer implementation.
		 *
		 * Updates local data corresponding to id, unserializing it from the
		 * specified string.
		 *
		 * This is called in HardSync mode when data has been
		 * released from an other proc.
		 *
		 * @param id local data id
		 * @param data serialized updates
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraph<T, S, N>::updateData(unsigned long id, std::string data) {
			FPMAS_LOGV(getMpiCommunicator().getRank(), "GRAPH", "updateData %lu : %s", id, data.c_str());
			this->getNodes().at(id)->data()->update(json::parse(data).get<T>());
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

				FPMAS_LOGV(mpiCommunicator.getRank(), "Dist", "Zoltan Node migration...");
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
				FPMAS_LOGV(mpiCommunicator.getRank(), "Dist", "Zoltan Node migration done.");

				// Prepares Zoltan to migrate arcs associated to the exported
				// nodes.
				this->setZoltanArcMigration();

				// Unused buffer
				ZOLTAN_ID_TYPE export_arcs_local_ids[0];

				FPMAS_LOGV(mpiCommunicator.getRank(), "Dist", "Zoltan Arc migration...");
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
				FPMAS_LOGV(mpiCommunicator.getRank(), "Dist", "Zoltan Arc migration done.");
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

			this->synchronize();

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
			syncMode.termination();
		}
	}
}

#endif
