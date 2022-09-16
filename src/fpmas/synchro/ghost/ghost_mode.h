#ifndef FPMAS_GHOST_MODE_H
#define FPMAS_GHOST_MODE_H

/** \file src/fpmas/synchro/ghost/ghost_mode.h
 * GhostMode implementation.
 */

#include "fpmas/communication/communication.h"
#include "fpmas/api/graph/distributed_graph.h"
#include "fpmas/utils/macros.h"
#include "../data_update_pack.h"
#include "../synchro.h"
#include "single_thread_mutex.h"

namespace fpmas { namespace synchro {
	namespace ghost {

		using api::graph::LocationState;
		/**
		 * GhostMode Mutex implementation.
		 *
		 * read() and acquire() methods return a reference to the local data.
		 *
		 * @see fpmas::synchro::GhostMode
		 */
		template<typename T>
			class GhostMutex : public SingleThreadMutex<T> {
				public:
					using SingleThreadMutex<T>::SingleThreadMutex;

					const T& read() override {return this->data();};
					void releaseRead() override {};
					T& acquire() override {return this->data();};
					void releaseAcquire() override {};
					void synchronize() override {};
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
				requests = id_mpi.migrate(std::move(requests));

				std::unordered_map<int, std::vector<NodeUpdatePack<T>>> updated_data;
				for(auto list : requests) {
					updated_data[list.first].reserve(list.second.size());
					for(auto id : list.second) {
						FPMAS_LOGV(
								graph.getMpiCommunicator().getRank(), "GHOST_MODE",
								"Export %s to %i", FPMAS_C_STR(id), list.first
								);
						auto node = graph.getNode(id);
						updated_data[list.first].emplace_back(
								id, node->data(), node->getWeight()
								);
					}
				}

				// TODO : Should use data update packs.
				updated_data = data_mpi.migrate(std::move(updated_data));
				for(auto list : updated_data) {
					for(auto& data : list.second) {
						auto local_node = graph.getNode(data.id);
						synchro::DataUpdate<T>::update(
								local_node->data(),
								std::move(data.updated_data)
								);
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

						 requests[node->location()].emplace_back(node->getId());
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

				_synchronize(buildRequests(nodes));
				for(auto node : nodes)
					node->mutex()->synchronize();
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
					 requests[node.second->location()].emplace_back(node.first);
				 }
				 return requests;
			 }

		template<typename T>
			void GhostDataSync<T>::synchronize() {
				FPMAS_LOGI(
						graph.getMpiCommunicator().getRank(), "GHOST_MODE",
						"Synchronizing graph data...", "");

				_synchronize(buildRequests());
				for(auto node : graph.getNodes())
					node.second->mutex()->synchronize();
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
					typedef api::utils::PtrWrapper<EdgeApi> EdgePtr;
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
				link_migration = edge_mpi.migrate(std::move(link_migration));

				for(auto import_list : link_migration) {
					for (auto edge : import_list.second) {
						graph.importEdge(edge);
					}
				}
				link_buffer.clear();

				/*
				 * Migrate node removal
				 */
				remove_node_buffer = id_mpi.migrate(std::move(remove_node_buffer));
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
				unlink_migration = id_mpi.migrate(std::move(unlink_migration));
				for(auto import_list : unlink_migration) {
					for(DistributedId id : import_list.second) {
						const auto& edges = graph.getEdges();
						auto it = edges.find(id);
						if(it != edges.end())
							graph.erase(it->second);
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
		template<typename T, template<typename> class Mutex>
			class GhostMode : public api::synchro::SyncMode<T> {
				communication::TypedMpi<DistributedId> id_mpi;
				communication::TypedMpi<NodeUpdatePack<T>> data_mpi;
				communication::TypedMpi<api::utils::PtrWrapper<api::graph::DistributedEdge<T>>> edge_mpi;

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
				Mutex<T>* buildMutex(api::graph::DistributedNode<T>* node) override {
					return new Mutex<T>(node->data());
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

	/**
	 * Regular GhostMode implementation, based on the GhostMutex.
	 *
	 * Data synchronization can be described as follows:
	 * - \LOCAL nodes: read and acquires are performed directly on the local
	 *   data
	 * - \DISTANT nodes: read and acquires are performed on a local copy,
	 *   imported from the origin process at each DataSync::synchronize() call.
	 *   Local modifications are overridden at each synchronization.
	 *
	 * In the \Agent context, this means that modifications on \LOCAL agents are
	 * immediately perceived by \LOCAL agents, even if they are only exported at
	 * the end of the time step to distant processes.
	 *
	 * Since modifications on \DISTANT agents are erased at each
	 * synchronization, modifications between agents is likely to produce
	 * unexpected behaviors in this mode. However, each \LOCAL agent is allowed
	 * to modify its own state.
	 *
	 * In consequence, even in a read-only model, the state of perceived agents
	 * depends on the execution order **and** on the \LOCAL or \DISTANT state
	 * of each agent:
	 * - the state of a \LOCAL agent not executed yet corresponds to its state
	 *   at the previous time step
	 * - the state of an already executed \LOCAL agent corresponds to its state
	 *   at the current time step
	 * - the state of a \DISTANT agent always corresponds to its state at the
	 *   previous time step
	 *
	 * The results of a read-only model is hence reproducible only considering
	 * that the agents execution order and distribution are the same.
	 *
	 * @note
	 * The fpmas::seed() method can be used to enforce those constraints.
	 * 
	 * A more consistent and reproducible behavior can be obtained using the
	 * fpmas::synchro::GlobalGhostMode.
	 *
	 *
	 * This synchronization policy is inspired from
	 * [RepastHPC](https://repast.github.io/hpc_tutorial/RepastHPC_Demo_01_Step_09.html).
	 *
	 * @see fpmas::synchro::GlobalGhostMode
	 * @see fpmas::synchro::HardSyncMode
	 */
	template<typename T>
		using GhostMode = ghost::GhostMode<T, ghost::GhostMutex>;
}}
#endif
