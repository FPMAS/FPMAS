#ifndef BASIC_GHOST_MODE_H
#define BASIC_GHOST_MODE_H

#include <vector>
#include "fpmas/api/synchro/mutex.h"
#include "fpmas/api/synchro/sync_mode.h"
#include "fpmas/communication/communication.h"
#include "fpmas/graph/parallel/distributed_edge.h"
#include "fpmas/graph/parallel/distributed_node.h"

namespace fpmas { namespace synchro { namespace ghost {

	using api::graph::LocationState;

	template<typename T>
		class SingleThreadMutex : public api::synchro::Mutex<T> {
			private:
				T& _data;
				void _lock() override {};
				void _lockShared() override {};
				void _unlock() override {};
				void _unlockShared() override {};

			public:
				SingleThreadMutex(T& data) : _data(data) {}

				T& data() override {return _data;}
				const T& data() const override {return _data;}

				// TODO: Exceptions when multiple lock
				const T& read() override {return _data;};
				void releaseRead() override {};
				T& acquire() override {return _data;};
				void releaseAcquire() override {};

				void lock() override {};
				void unlock() override {};
				bool locked() const override {return false;}

				void lockShared() override {};
				void unlockShared() override {};
				int lockedShared() const override {return 0;};
		};

	template<typename T>
		class GhostDataSync : public api::synchro::DataSync {
			public:
				typedef graph::NodePtrWrapper<T> NodePtr;
				typedef api::communication::TypedMpi<NodePtr> NodeMpi;

				typedef api::communication::TypedMpi<DistributedId> IdMpi;

			private:
				NodeMpi& node_mpi;
				IdMpi& id_mpi;

				api::graph::DistributedGraph<T>& graph;

			public:
				GhostDataSync(
						NodeMpi& node_mpi, IdMpi& id_mpi,
						api::graph::DistributedGraph<T>& graph
						)
					: node_mpi(node_mpi), id_mpi(id_mpi), graph(graph) {}

				void synchronize() override;
		};

	template<typename T>
		void GhostDataSync<T>::synchronize() {
			FPMAS_LOGI(graph.getMpiCommunicator().getRank(), "GHOST_MODE", "Synchronizing graph data...");
			std::unordered_map<int, std::vector<DistributedId>> requests;
			for(auto node : graph.getLocationManager().getDistantNodes()) {
				FPMAS_LOGV(graph.getMpiCommunicator().getRank(), "GHOST_MODE", "Request %s from %i",
						ID_C_STR(node.first), node.second->getLocation());
				requests[node.second->getLocation()].push_back(node.first);
			}
			requests = id_mpi.migrate(requests);

			std::unordered_map<int, std::vector<NodePtr>> nodes;
			for(auto list : requests) {
				for(auto id : list.second) {
					FPMAS_LOGV(graph.getMpiCommunicator().getRank(), "GHOST_MODE", "Export %s to %i",
							ID_C_STR(id), list.first);
					nodes[list.first].push_back(graph.getNode(id));
				}
			}
			// TODO : Should use data update packs.
			nodes = node_mpi.migrate(nodes);
			for(auto list : nodes) {
				for(auto& node : list.second) {
					auto local_node = graph.getNode(node->getId());
					local_node->data() = std::move(node->data());
					local_node->setWeight(node->getWeight());
				}
			}
			for(auto node_list : nodes)
				for(auto node : node_list.second)
					delete node.get();

			FPMAS_LOGI(graph.getMpiCommunicator().getRank(), "GHOST_MODE", "Graph data synchronized...");
		}


	template<typename T>
		class GhostSyncLinker : public api::synchro::SyncLinker<T> {

			public:
				typedef api::graph::DistributedEdge<T> EdgeApi;
				typedef graph::EdgePtrWrapper<T> EdgePtr;
				typedef api::communication::TypedMpi<EdgePtr> EdgeMpi;
				typedef api::communication::TypedMpi<DistributedId> IdMpi;
			private:
				std::vector<EdgePtr> link_buffer;
				//std::vector<DistributedId> unlink_buffer;
				std::unordered_map<int, std::vector<DistributedId>> unlink_migration;

				EdgeMpi& edge_mpi;
				IdMpi& id_mpi;
				api::graph::DistributedGraph<T>& graph;

			public:
				GhostSyncLinker(
						EdgeMpi& edge_mpi, IdMpi& id_mpi,
						api::graph::DistributedGraph<T>& graph)
					: edge_mpi(edge_mpi), id_mpi(id_mpi), graph(graph) {}

				void link(const EdgeApi*) override;
				void unlink(const EdgeApi*) override;

				void synchronize() override;
		};

	template<typename T>
		void GhostSyncLinker<T>::link(const EdgeApi * edge) {
			if(edge->state() == LocationState::DISTANT) {
				link_buffer.push_back(const_cast<EdgeApi*>(edge));
			}
		}

	template<typename T>
		void GhostSyncLinker<T>::unlink(const EdgeApi * edge) {
			if(edge->state() == LocationState::DISTANT) {
				link_buffer.erase(
						std::remove(link_buffer.begin(), link_buffer.end(), edge),
						link_buffer.end()
						);
				auto src = edge->getSourceNode();
				if(src->state() == LocationState::DISTANT) {
					unlink_migration[src->getLocation()].push_back(edge->getId());
				}
				auto tgt = edge->getTargetNode();
				if(tgt->state() == LocationState::DISTANT) {
					unlink_migration[tgt->getLocation()].push_back(edge->getId());
				}
			}
		}

	template<typename T>
		void GhostSyncLinker<T>::synchronize() {
			/*
			 * Migrate links
			 */
			std::vector<EdgePtr> edges_to_clear;
			std::unordered_map<int, std::vector<EdgePtr>> link_migration;
			for(auto edge : link_buffer) {
				auto src = edge->getSourceNode();
				if(src->state() == LocationState::DISTANT) {
					link_migration[src->getLocation()].push_back(edge);
				}
				auto tgt = edge->getTargetNode();
				if(tgt->state() == LocationState::DISTANT) {
					link_migration[tgt->getLocation()].push_back(edge);
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
		}

	template<typename T>
		class GhostModeRuntime : api::synchro::SyncModeRuntime<T> {
			communication::TypedMpi<DistributedId> id_mpi;
			communication::TypedMpi<graph::NodePtrWrapper<T>> node_mpi;
			communication::TypedMpi<graph::EdgePtrWrapper<T>> edge_mpi;

			GhostDataSync<T> data_sync;
			GhostSyncLinker<T> sync_linker;

			public:
			GhostModeRuntime(
					api::graph::DistributedGraph<T>& graph,
					api::communication::MpiCommunicator& comm)
				: id_mpi(comm), node_mpi(comm), edge_mpi(comm),
				data_sync(node_mpi, id_mpi, graph), sync_linker(edge_mpi, id_mpi, graph) {}

			SingleThreadMutex<T>* buildMutex(api::graph::DistributedNode<T>* node) override {
				return new SingleThreadMutex<T>(node->data());
			};
			GhostDataSync<T>& getDataSync() override {return data_sync;}
			GhostSyncLinker<T>& getSyncLinker() override {return sync_linker;}
		};
}

typedef api::synchro::SyncMode<
synchro::ghost::SingleThreadMutex,
	synchro::ghost::GhostModeRuntime
	> GhostMode;
	}}
#endif
