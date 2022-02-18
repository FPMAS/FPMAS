#ifndef BASIC_GHOST_MODE_H
#define BASIC_GHOST_MODE_H

/** \file src/fpmas/synchro/ghost/ghost_mode.h
 * GhostMode implementation.
 */

#include "fpmas/communication/communication.h"
#include "fpmas/graph/distributed_graph.h"
#include "../data_update_pack.h"

namespace fpmas { namespace synchro {
	namespace ghost {

		using api::graph::LocationState;

		/**
		 * GhostMode Mutex implementation.
		 *
		 * Currently, the GhostMode is defined such that a unique thread is
		 * allowed to access to each node. In consequence, there is literally
		 * no need for concurrency management.
		 *
		 * In the future, this might be improved to handle multi-threading.
		 */
		template<typename T>
			class SingleThreadMutex : public api::synchro::Mutex<T> {
				private:
					T& _data;
					void _lock() override {};
					void _lockShared() override {};
					void _unlock() override {};
					void _unlockShared() override {};

				public:
					/**
					 * SingleThreadMutex constructor.
					 *
					 * @param data reference to node data
					 */
					SingleThreadMutex(T& data) : _data(data) {}

					T& data() override {return _data;}
					const T& data() const override {return _data;}

					const T& read() override {return _data;};
					void releaseRead() override {};
					T& acquire() override {return _data;};
					void releaseAcquire() override {};

					void lock() override {};
					void unlock() override {};
					bool locked() const override {return false;}

					void lockShared() override {};
					void unlockShared() override {};
					int sharedLockCount() const override {return 0;};
			};

		/**
		 * GhostMode DataSync implementation.
		 *
		 * Data of each \DISTANT node is fetched from host processes at each
		 * graph synchronization operation.
		 */
		template<typename T>
			class GhostDataSync : public api::synchro::DataSync<T> {
				public:
					/**
					 * TypedMpi used to transmit NodeUpdatePacks with MPI.
					 */
					typedef api::communication::TypedMpi<NodeUpdatePack<T>> DataMpi;
					/**
					 * TypedMpi used to transmit DistributedIds with MPI.
					 */
					typedef api::communication::TypedMpi<DistributedId> IdMpi;

				private:
					DataMpi& data_mpi;
					IdMpi& id_mpi;

					api::graph::DistributedGraph<T>& graph;
					std::unordered_map<int, std::vector<DistributedId>> buildRequests();
					std::unordered_map<int, std::vector<DistributedId>> buildRequests(
							std::unordered_set<api::graph::DistributedNode<T>*> nodes
							);
					void _synchronize(
							std::unordered_map<int, std::vector<DistributedId>> requests
							);

				public:
					/**
					 * GhostDataSync constructor.
					 *
					 * @param data_mpi DataMpi instance
					 * @param id_mpi IdMpi instance
					 * @param graph reference to the associated
					 * DistributedGraph
					 */
					GhostDataSync(
							DataMpi& data_mpi, IdMpi& id_mpi,
							api::graph::DistributedGraph<T>& graph
							)
						: data_mpi(data_mpi), id_mpi(id_mpi), graph(graph) {}

					/**
					 * Fetches updated data for all the \DISTANT nodes of the
					 * DistributedGraph from the corresponding host processes.
					 */
					void synchronize() override;

					void synchronize(
							std::unordered_set<api::graph::DistributedNode<T>*> nodes
							) override;
			};

		template<typename T>
			void GhostDataSync<T>::_synchronize(
					std::unordered_map<int, std::vector<DistributedId>> requests) {
				FPMAS_LOGI(
						graph.getMpiCommunicator().getRank(), "GHOST_MODE",
						"Synchronizing graph data...", ""
						);
				requests = id_mpi.migrate(requests);

				std::unordered_map<int, std::vector<NodeUpdatePack<T>>> updated_data;
				for(auto list : requests) {
					for(auto id : list.second) {
						FPMAS_LOGV(
								graph.getMpiCommunicator().getRank(), "GHOST_MODE",
								"Export %s to %i", FPMAS_C_STR(id), list.first
								);
						auto node = graph.getNode(id);
						updated_data[list.first].push_back({
								id, node->data(), node->getWeight()
								});
					}
				}
				// TODO : Should use data update packs.
				updated_data = data_mpi.migrate(updated_data);
				for(auto list : updated_data) {
					for(auto& data : list.second) {
						auto local_node = graph.getNode(data.id);
						local_node->data() = std::move(data.updated_data);
						local_node->setWeight(data.updated_weight);
					}
				}

				FPMAS_LOGI(
						graph.getMpiCommunicator().getRank(), "GHOST_MODE",
						"Graph data synchronized.", ""
						);
			}

		template<typename T>
			 std::unordered_map<int, std::vector<DistributedId>> GhostDataSync<T>
			 ::buildRequests(std::unordered_set<api::graph::DistributedNode<T>*> nodes) {
				 std::unordered_map<int, std::vector<DistributedId>> requests;
				 for(auto node : nodes) {
					 if(node->state() == fpmas::api::graph::DISTANT) {
						 FPMAS_LOGV(
								 graph.getMpiCommunicator().getRank(), "GHOST_MODE",
								 "Request %s from %i",
								 FPMAS_C_STR(node->getId()), node->location());

						 requests[node->location()].push_back(node->getId());
					 }
				 }
				 return requests;
			 }

		template<typename T>
			void GhostDataSync<T>::synchronize(
					std::unordered_set<api::graph::DistributedNode<T>*> nodes) {
				FPMAS_LOGI(
						graph.getMpiCommunicator().getRank(), "GHOST_MODE",
						"Synchronizing graph data...", ""
						);

				std::unordered_map<int, std::vector<DistributedId>> requests
					= buildRequests(nodes);
				_synchronize(requests);
			}

		template<typename T>
			 std::unordered_map<int, std::vector<DistributedId>> GhostDataSync<T>
			 ::buildRequests() {
				 std::unordered_map<int, std::vector<DistributedId>> requests;
				 for(auto node : graph.getLocationManager().getDistantNodes()) {
					 FPMAS_LOGV(
							 graph.getMpiCommunicator().getRank(), "GHOST_MODE",
							 "Request %s from %i",
							 FPMAS_C_STR(node.first), node.second->location()
							 );
					 requests[node.second->location()].push_back(node.first);
				 }
				 return requests;
			 }

		template<typename T>
			void GhostDataSync<T>::synchronize() {
				FPMAS_LOGI(
						graph.getMpiCommunicator().getRank(), "GHOST_MODE",
						"Synchronizing graph data...", "");

				std::unordered_map<int, std::vector<DistributedId>> requests
					= buildRequests();
				_synchronize(requests);
			}


		/**
		 * Base GhostMode SyncLinker implementation.
		 *
		 * Link, unlink and node removal requests involving \DISTANT nodes are
		 * buffered until synchronize_links() is called.
		 *
		 * This class defines everything required to implement an
		 * api::synchro::SyncLinker that only commits link, unlink and node
		 * removal requests using collective communications. The commit
		 * operations can notably be performed using the simple
		 * synchronize_links() method. The only method not implemented is
		 * synchronize(), so that custom api::synchro::SyncLinker
		 * implementations can be defined extending this class.
		 * Notice that the synchronize() implementation can be as simple as a
		 * call to synchronize_links(), as performed by the GhostSyncLinker.
		 */
		template<typename T>
			class GhostSyncLinkerBase : public api::synchro::SyncLinker<T> {
				public:
					/**
					 * DistributedEdge API
					 */
					typedef api::graph::DistributedEdge<T> EdgeApi;
					/**
					 * DistributedNode API
					 */
					typedef api::graph::DistributedNode<T> NodeApi;
					/**
					 * DistributedEdge pointer wrapper
					 */
					typedef graph::EdgePtrWrapper<T> EdgePtr;
					/**
					 * TypedMpi used to transmit Edges with MPI.
					 */
					typedef api::communication::TypedMpi<EdgePtr> EdgeMpi;
					/**
					 * TypedMpi used to transmit DistributedIds with MPI.
					 */
					typedef api::communication::TypedMpi<DistributedId> IdMpi;
				private:
					std::vector<EdgePtr> link_buffer;
					std::unordered_map<int, std::vector<DistributedId>> unlink_migration;
					std::unordered_map<int, std::vector<DistributedId>> remove_node_buffer;
					std::vector<NodeApi*> local_nodes_to_remove;

					EdgeMpi& edge_mpi;
					IdMpi& id_mpi;
					api::graph::DistributedGraph<T>& graph;

				protected:
					/**
					 * Commits currently buffered link, unlink and node removal
					 * operations.
					 */
					void synchronize_links();

				public:
					/**
					 * GhostSyncLinker constructor.
					 *
					 * @param edge_mpi EdgePtr MPI communicator
					 * @param id_mpi DistributedId MPI communicator
					 * @param graph reference to the associated
					 * DistributedGraph
					 */
					GhostSyncLinkerBase(
							EdgeMpi& edge_mpi, IdMpi& id_mpi,
							api::graph::DistributedGraph<T>& graph)
						: edge_mpi(edge_mpi), id_mpi(id_mpi), graph(graph) {}

					/**
					 * If source or target node is \DISTANT, edge is buffered to
					 * be committed at the next synchronize() call.
					 *
					 * @param edge linked edge
					 */
					void link(EdgeApi* edge) override;

					/**
					 * If source or target node is \DISTANT, edge unlinking is buffered to
					 * be committed at the next synchronize() call.
					 *
					 * For efficiency purpose, the edge is removed from the
					 * link buffer if it was not committed yet.
					 *
					 * @param edge unlinked edge
					 */
					void unlink(EdgeApi* edge) override;

					void removeNode(NodeApi* node) override;
			};

		template<typename T>
			void GhostSyncLinkerBase<T>::synchronize_links() {
				FPMAS_LOGI(graph.getMpiCommunicator().getRank(),
						"GHOST_MODE", "Synchronizing graph links...", "");
				/*
				 * Migrate links
				 */
				std::vector<EdgePtr> edges_to_clear;
				std::unordered_map<int, std::vector<EdgePtr>> link_migration;
				for(auto edge : link_buffer) {
					auto src = edge->getSourceNode();
					if(src->state() == LocationState::DISTANT) {
						link_migration[src->location()].push_back(edge);
					}
					auto tgt = edge->getTargetNode();
					if(tgt->state() == LocationState::DISTANT) {
						link_migration[tgt->location()].push_back(edge);
					}
					if(src->state() == LocationState::DISTANT
							&& tgt->state() == LocationState::DISTANT) {
						edges_to_clear.push_back(edge);
					}
				}
				link_migration = edge_mpi.migrate(link_migration);

				for(auto import_list : link_migration) {
					for (auto edge : import_list.second) {
						graph.importEdge(edge);
					}
				}
				link_buffer.clear();

				/*
				 * Migrate node removal
				 */
				remove_node_buffer = id_mpi.migrate(remove_node_buffer);
				for(auto import_list : remove_node_buffer) {
					for(DistributedId node_id : import_list.second) {
						auto* node = graph.getNode(node_id);
						for(auto edge : node->getOutgoingEdges())
							graph.unlink(edge);
						for(auto edge : node->getIncomingEdges())
							graph.unlink(edge);
					}
				}

				/*
				 * Migrate unlinks
				 */
				unlink_migration = id_mpi.migrate(unlink_migration);
				for(auto import_list : unlink_migration) {
					for(DistributedId id : import_list.second) {
						if(graph.getEdges().count(id) > 0) {
							auto edge = graph.getEdge(id);
							graph.erase(edge);
						}
					}
				}
				unlink_migration.clear();

				for(auto edge : edges_to_clear)
					graph.erase(edge);

				for(auto import_list : remove_node_buffer)
					for(auto node_id : import_list.second)
						local_nodes_to_remove.push_back(graph.getNode(node_id));
				remove_node_buffer.clear();

				for(auto node : local_nodes_to_remove) {
					graph.erase(node);
				}
				local_nodes_to_remove.clear();

				FPMAS_LOGI(graph.getMpiCommunicator().getRank(),
						"GHOST_MODE", "Graph links synchronized.", "");
			}

		template<typename T>
			void GhostSyncLinkerBase<T>::link(EdgeApi * edge) {
				if(edge->state() == LocationState::DISTANT) {
					link_buffer.push_back(const_cast<EdgeApi*>(edge));
				}
			}

		template<typename T>
			void GhostSyncLinkerBase<T>::unlink(EdgeApi * edge) {
				if(edge->state() == LocationState::DISTANT) {
					link_buffer.erase(
							std::remove(link_buffer.begin(), link_buffer.end(), edge),
							link_buffer.end()
							);
					auto src = edge->getSourceNode();
					if(src->state() == LocationState::DISTANT) {
						unlink_migration[src->location()].push_back(edge->getId());
					}
					auto tgt = edge->getTargetNode();
					if(tgt->state() == LocationState::DISTANT) {
						unlink_migration[tgt->location()].push_back(edge->getId());
					}
				}
			}

		template<typename T>
			void GhostSyncLinkerBase<T>::removeNode(NodeApi* node) {
				if(node->state() == LocationState::DISTANT) {
					remove_node_buffer[node->location()].push_back(node->getId());
				} else {
					for(auto edge : node->getOutgoingEdges())
						graph.unlink(edge);
					for(auto edge : node->getIncomingEdges())
						graph.unlink(edge);
				}
				local_nodes_to_remove.push_back(node);
			}

		/**
		 * GhostMode SyncLinker implementation.
		 *
		 * Link, unlink and node removal requests are buffered until the next
		 * graph synchronization.
		 *
		 * All operations are then committed across processes when
		 * synchronize() is called.
		 */
		template<typename T>
			class GhostSyncLinker : public GhostSyncLinkerBase<T> {
				public:
					using GhostSyncLinkerBase<T>::GhostSyncLinkerBase;

					void synchronize() override;
			};

		template<typename T>
			void GhostSyncLinker<T>::synchronize() {
				this->synchronize_links();
			}

		/**
		 * Ghost SyncMode implementation.
		 *
		 * The concept of GhostMode is that local data is updated **only** at
		 * each api::graph::DistributedGraph::synchronize() call by the
		 * GhostDataSync instance. Let's call a "time step" the period between
		 * two graph synchronizations. It is not guaranteed that \DISTANT node
		 * data accessed within a time step is synchronized with the process
		 * that owns the node.
		 *
		 * More importantly, **write operations on \DISTANT nodes are not
		 * reported** to the hosts processes.
		 *
		 * In consequence, the corresponding api::synchro::Mutex implementation
		 * is trivial, since (for now) we consider a single-threaded
		 * environment, so no concurrency needs to be managed since the local
		 * process necessarily access sequentially the local nodes' data.
		 *
		 * However, link, unlink and node removal operations are allowed, and
		 * are committed "at the end of each time step" by the GhostSyncLinker
		 * instance, i.e. at each graph synchronization.
		 */
		template<typename T>
			class GhostMode : public api::synchro::SyncMode<T> {
				communication::TypedMpi<DistributedId> id_mpi;
				communication::TypedMpi<NodeUpdatePack<T>> data_mpi;
				communication::TypedMpi<graph::EdgePtrWrapper<T>> edge_mpi;

				GhostDataSync<T> data_sync;
				GhostSyncLinker<T> sync_linker;

				public:
				/**
				 * GhostMode constructor.
				 *
				 * @param graph reference to the associated DistributedGraph
				 * @param comm MPI communicator
				 */
				GhostMode(
						api::graph::DistributedGraph<T>& graph,
						api::communication::MpiCommunicator& comm)
					: id_mpi(comm), data_mpi(comm), edge_mpi(comm),
					data_sync(data_mpi, id_mpi, graph), sync_linker(edge_mpi, id_mpi, graph) {}

				/**
				 * Builds a new SingleThreadMutex from the specified node data.
				 *
				 * @param node node to which the built mutex will be associated
				 */
				SingleThreadMutex<T>* buildMutex(api::graph::DistributedNode<T>* node) override {
					return new SingleThreadMutex<T>(node->data());
				};

				/**
				 * Returns a reference to the internal GhostDataSync instance.
				 *
				 * @return reference to the DataSync instance
				 */
				GhostDataSync<T>& getDataSync() override {return data_sync;}

				/**
				 * Returns a reference to the internal GhostSyncLinker instance.
				 *
				 * @return reference to the SyncLinker instance
				 */
				GhostSyncLinker<T>& getSyncLinker() override {return sync_linker;}
			};
	}
	using ghost::GhostMode;
}}
#endif
