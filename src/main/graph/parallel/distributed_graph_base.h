#ifndef DISTRIBUTED_GRAPH_BASE_H
#define DISTRIBUTED_GRAPH_BASE_H

#include "../base/graph.h"

#include "zoltan/zoltan_lb.h"
#include "zoltan/zoltan_node_migrate.h"
#include "zoltan/zoltan_arc_migrate.h"

#include "distributed_id.h"
#include "olz_graph.h"

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

		/**
		 * Defines basic items and interfaces of a distributed Graph structure.
		 *
		 * Functions that are only local, such as the buildNode functions, are
		 * implemented by this base.
		 *
		 * Any distributed Graph implementation should then provide specific
		 * functions that might perform distant operations, usually depending
		 * on the synchronization mode used :
		 * - link(DistributedId, DistributedId, DistributedId, LayerId)
		 * - unlink(arc_ptr)
		 * - distribute()
		 * - distribute(std::unordered_map<DistributedId, std::pair<int, int>>)
		 * - synchronize()
		 *
		 */
		template<typename T, SYNC_MODE, int N>
		class DistributedGraphBase : public Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>, communication::ResourceContainer {
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

			// Serialization caches used to pack objects
			std::unordered_map<DistributedId, std::string> node_serialization_cache;
			std::unordered_map<DistributedId, std::string> arc_serialization_cache;

			// When importing nodes, obsolete ghost nodes are stored when
			// the real node has been imported. It is safely deleted in
			// arc::mid_migrate_pp_fn once associated arcs have eventually 
			// been exported
			std::set<DistributedId> obsoleteGhosts;
			void setUpZoltan();
			void removeExportedNode(DistributedId);

			protected:
			SyncMpiCommunicator mpiCommunicator;
			Zoltan zoltan;
			Proxy proxy;
			S<T, N> syncMode;
			GhostGraph<T, N, S> ghost;

			void setZoltanNodeMigration();
			void setZoltanArcMigration();

			/*
			 * Zoltan structures used to manage nodes and arcs migration
			 */
			// Node export buffer
			int export_node_num;
			ZOLTAN_ID_PTR export_node_global_ids;
			int* export_node_procs;

			// Arc migration buffers
			int export_arcs_num;
			ZOLTAN_ID_PTR export_arcs_global_ids;
			int* export_arcs_procs;

			Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>* _buildNode(DistributedId id, float weight, T&& data) {
				return this->Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>
					::_buildNode(
						id,
						weight,
						std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							id,
							this->getMpiCommunicator(),
							this->getProxy(),
							std::forward<T>(data)
						))
					);
			}

			Arc<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>* _link(DistributedId arcId, DistributedId source, DistributedId target, LayerId layer) {
				return this->Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>
					::_link(arcId, source, target, layer);
			};

			public:
			typedef Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N> node_type;
			typedef Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>* node_ptr;
			typedef Arc<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>* arc_ptr;

			DistributedGraphBase<T, S, N>();
			DistributedGraphBase<T, S, N>(std::initializer_list<int>);

			DistributedGraphBase<T, S, N>(const DistributedGraphBase<T,S,N>&) = delete;
			DistributedGraphBase<T, S, N>& operator=(const DistributedGraphBase<T, S, N>&) = delete;

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

			std::string getLocalData(DistributedId) const override;
			std::string getUpdatedData(DistributedId) const override;
			void updateData(DistributedId, std::string) override;


			node_ptr buildNode();
			node_ptr buildNode(T&& data);
			node_ptr buildNode(T& data);
			node_ptr buildNode(float weight, T&& data);
			node_ptr buildNode(float weight, const T& data);

			//arc_ptr link(DistributedId, DistributedId);
			void unlink(DistributedId);

			using Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>::link;
			/**
			 * Links the specified source and target node within this
			 * DistributedGraph on the given layer.
			 *
			 * @param source source node id
			 * @param target target node id
			 * @param arcId new arc id
			 * @param layerId id of the Layer on which nodes should be linked
			 * @return pointer to the created arc
			 */
			virtual arc_ptr link(
					DistributedId, DistributedId, LayerId
					) override = 0;
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
			 * @param arc pointer to the arc to unlink (might be a local Arc or distant
			 * GhostArc)
			 */
			virtual void unlink(arc_ptr) override = 0;

			/**
			 * Distributes the graph accross the available cores performing a
			 * load balancing.
			 */
			virtual void distribute() = 0;

			/**
			 * Distributes the graph accross the available cores using the
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
			virtual void distribute(std::unordered_map<DistributedId, std::pair<int, int>>) = 0;

			virtual void synchronize();

		};
		/**
		 * Builds a DistributedGraph over all the available procs.
		 */
		template<class T, SYNC_MODE, int N> DistributedGraphBase<T, S, N>::DistributedGraphBase()
			:	
				Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>(),
				syncMode(*this),
				mpiCommunicator(*this),
				ghost(this),
				proxy(mpiCommunicator.getRank()),
				zoltan(mpiCommunicator.getMpiComm())
		{
			this->currentNodeId = DistributedId(mpiCommunicator.getRank());
			this->currentArcId = DistributedId(mpiCommunicator.getRank());
			this->setUpZoltan();
		}

		/**
		 * Builds a DistributedGraph over the specified procs.
		 *
		 * @param ranks ranks of the procs on which the DistributedGraph is
		 * built
		 */
		template<class T, SYNC_MODE, int N> DistributedGraphBase<T, S, N>::DistributedGraphBase(std::initializer_list<int> ranks)
			: 
				Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>(),
				syncMode(*this),
				mpiCommunicator(*this, ranks),
				ghost(this, ranks),
				proxy(mpiCommunicator.getRank(), ranks),
				zoltan(mpiCommunicator.getMpiComm())
		{
			this->currentNodeId = DistributedId(mpiCommunicator.getRank());
			this->currentArcId = DistributedId(mpiCommunicator.getRank());
			this->setUpZoltan();
		}

		/*
		 * Initializes zoltan parameters and zoltan lb query functions.
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraphBase<T, S, N>::setUpZoltan() {
			FPMAS::config::zoltan_config(&this->zoltan);

			// Initializes Zoltan Node Load Balancing functions
			this->zoltan.Set_Num_Obj_Fn(FPMAS::graph::parallel::zoltan::num_obj<T, N, S>, this);
			this->zoltan.Set_Obj_List_Fn(FPMAS::graph::parallel::zoltan::obj_list<T, N, S>, this);
			this->zoltan.Set_Num_Edges_Multi_Fn(FPMAS::graph::parallel::zoltan::num_edges_multi_fn<T, N, S>, this);
			this->zoltan.Set_Edge_List_Multi_Fn(FPMAS::graph::parallel::zoltan::edge_list_multi_fn<T, N, S>, this);
		}

		template<class T, SYNC_MODE, int N> SyncMpiCommunicator& DistributedGraphBase<T, S, N>::getMpiCommunicator() {
			return this->mpiCommunicator;
		}

		template<class T, SYNC_MODE, int N> const SyncMpiCommunicator& DistributedGraphBase<T, S, N>::getMpiCommunicator() const {
			return this->mpiCommunicator;
		}
		/**
		 * Returns a reference to the proxy associated to this DistributedGraphBase.
		 *
		 * @return reference to the current proxy
		 */
		template<class T, SYNC_MODE, int N> Proxy& DistributedGraphBase<T, S, N>::getProxy() {
			return this->proxy;
		}

		template<class T, SYNC_MODE, int N> GhostGraph<T, N, S>& DistributedGraphBase<T, S, N>::getGhost() {
			return this->ghost;
		}

		template<class T, SYNC_MODE, int N> const GhostGraph<T, N, S>& DistributedGraphBase<T, S, N>::getGhost() const {
			return this->ghost;
		}

		/**
		 * Configures Zoltan to migrate nodes.
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraphBase<T, S, N>::setZoltanNodeMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::node::obj_size_multi_fn<T, N, S>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::node::pack_obj_multi_fn<T, N, S>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::node::unpack_obj_multi_fn<T, N, S>, this);
			zoltan.Set_Mid_Migrate_PP_Fn(NULL);
			zoltan.Set_Post_Migrate_PP_Fn(syncMode.config().node_post_migrate_fn, this);
		}

		/**
		 * Configures Zoltan to migrate arcs.
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraphBase<T, S, N>::setZoltanArcMigration() {
			zoltan.Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T, N, S>, this);
			zoltan.Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T, N, S>, this);
			zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T, N, S>, this);

			zoltan.Set_Mid_Migrate_PP_Fn(syncMode.config().arc_mid_migrate_fn, this);
			zoltan.Set_Post_Migrate_PP_Fn(syncMode.config().arc_post_migrate_fn, this);
		}

		template<class T, SYNC_MODE, int N>
		typename DistributedGraphBase<T,S,N>::node_ptr DistributedGraphBase<T, S, N>
		::buildNode() {
			DistributedId currentId = this->currentNodeId;
			return Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>
				::buildNode(
					std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							currentId++,
							this->getMpiCommunicator(),
							this->getProxy(),
							T()
						))
					);
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
		typename DistributedGraphBase<T,S,N>::node_ptr DistributedGraphBase<T, S, N>
		::buildNode(T&& data) {
			DistributedId currentId = this->currentNodeId;
			return Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>
				::buildNode(
					std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							currentId++,
							this->getMpiCommunicator(),
							this->getProxy(),
							std::forward<T>(data)
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
		typename DistributedGraphBase<T,S,N>::node_ptr DistributedGraphBase<T, S, N>
		::buildNode(T& data) {
			DistributedId currentId = this->currentNodeId;
			return Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>
				::buildNode(
					std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							currentId++,
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
		typename DistributedGraphBase<T,S,N>::node_ptr DistributedGraphBase<T, S, N>
		::buildNode(float weight, T&& data) {
			DistributedId currentId = this->currentNodeId;
			return Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>
				::buildNode(
					weight,
					std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							currentId++,
							this->getMpiCommunicator(),
							this->getProxy(),
							std::forward<T>(data)
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
		typename DistributedGraphBase<T,S,N>::node_ptr DistributedGraphBase<T, S, N>
		::buildNode(float weight, const T& data) {
			DistributedId currentId = this->currentNodeId;
			return Graph<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>
				::buildNode(
					weight,
					std::unique_ptr<SyncData<T,N,S>>(S<T,N>::wrap(
							currentId++,
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
		 * @see link(DistributedId, DistributedId, DistributedId, LayerId)
		 */
		/*
		 *template<class T, SYNC_MODE, int N>
		 *typename DistributedGraphBase<T, S, N>::arc_ptr DistributedGraphBase<T, S, N>
		 *::link(DistributedId source, DistributedId target) {
		 *    return this->link(source, target, base::DefaultLayer);
		 *}
		 */

		/**
		 * Unlinks the arc with the specified id.
		 *
		 * If the arc is local, the operation is completely local.  Else, if
		 * the arc is a GhostArc, distant operations might apply.  See
		 * unlink(Arc<std::unique_ptr<SyncData<T,N,S>>,N>*) for more information.
		 *
		 * @param arcId id of the arc to unlink. (might be a local arc, or a
		 * ghost arc)
		 */
		template<typename T, SYNC_MODE, int N> void DistributedGraphBase<T, S, N>::unlink(DistributedId arcId) {
			try {
				this->unlink(this->getArcs().at(arcId));
			} catch (std::out_of_range) {
				try {
					this->unlink(this->getGhost().getArcs().at(arcId));
				} catch (std::out_of_range) {
					FPMAS_LOGE(
							this->mpiCommunicator.getRank(),
							"DIST_GRAPH", "%s",
							FPMAS::graph::base::exceptions::arc_out_of_graph(arcId).what()
							);
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
		template<class T, SYNC_MODE, int N> std::string DistributedGraphBase<T, S, N>::getLocalData(DistributedId id) const {
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
		template<class T, SYNC_MODE, int N> std::string DistributedGraphBase<T, S, N>::getUpdatedData(DistributedId id) const {
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
		template<class T, SYNC_MODE, int N> void DistributedGraphBase<T, S, N>::updateData(DistributedId id, std::string data) {
			FPMAS_LOGV(getMpiCommunicator().getRank(), "GRAPH", "updateData %lu : %s", id, data.c_str());
			this->getNodes().at(id)->data()->update(json::parse(data).get<T>());
		}

		template<class T, SYNC_MODE, int N> void DistributedGraphBase<T, S, N>
			::removeExportedNode(DistributedId id) {
				auto nodeToRemove = this->getNodes().at(id);
				for(auto& layer : nodeToRemove->getLayers()) {
					for(auto arc : layer.getIncomingArcs()) {
						try {
							this->Graph<std::unique_ptr<SyncData<T, N, S>>, DistributedId, N>
								::unlink(arc);
						} catch(base::exceptions::arc_out_of_graph<DistributedId>) {
							this->getGhost().unlink((GhostArc<T,N,S>*) arc);
						}
					}
					for(auto arc : layer.getOutgoingArcs()) {
						try {
							this->Graph<std::unique_ptr<SyncData<T, N, S>>, DistributedId, N>
								::unlink(arc);
						} catch(base::exceptions::arc_out_of_graph<DistributedId>) {
							this->getGhost().unlink((GhostArc<T,N,S>*) arc);
						}
					}
				}
				this->removeNode(id);
			}
		/**
		 * Synchronizes the DistributedGraph instances, calling the
		 * SyncMpiCommunicator::terminate() method.
		 *
		 * Can be overriden to perform extra processing.
		 */
		template<class T, SYNC_MODE, int N> void DistributedGraphBase<T, S, N>::synchronize() {
			this->mpiCommunicator.terminate();
		}


	}
}
#endif
