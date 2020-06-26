#ifndef BASIC_GHOST_MODE_H
#define BASIC_GHOST_MODE_H

#include <vector>
#include "fpmas/api/synchro/mutex.h"
#include "fpmas/api/synchro/sync_mode.h"
#include "fpmas/communication/communication.h"
#include "fpmas/graph/parallel/distributed_arc.h"
#include "fpmas/graph/parallel/distributed_node.h"

namespace fpmas::synchro {

	namespace ghost {
		using api::graph::parallel::LocationState;

		template<typename T>
			class SingleThreadMutex : public fpmas::api::synchro::Mutex<T> {
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
			class GhostDataSync : public fpmas::api::synchro::DataSync {
				public:
					typedef graph::parallel::NodePtrWrapper<T> NodePtr;
					typedef api::communication::TypedMpi<NodePtr> NodeMpi;

					typedef api::communication::TypedMpi<DistributedId> IdMpi;

				private:
					NodeMpi& node_mpi;
					IdMpi& id_mpi;

					fpmas::api::graph::parallel::DistributedGraph<T>& graph;

				public:
					GhostDataSync(
							NodeMpi& node_mpi, IdMpi& id_mpi,
							fpmas::api::graph::parallel::DistributedGraph<T>& graph
							)
						: node_mpi(node_mpi), id_mpi(id_mpi), graph(graph) {}

					void synchronize() override;
			};

		template<typename T>
			void GhostDataSync<T>::synchronize() {
				FPMAS_LOGI(graph.getMpiCommunicator().getRank(), "GHOST_MODE", "Synchronizing graph data...");
				std::unordered_map<int, std::vector<DistributedId>> requests;
				for(auto node : graph.getLocationManager().getDistantNodes()) {
					requests[node.second->getLocation()].push_back(node.first);
				}
				requests = id_mpi.migrate(requests);

				std::unordered_map<int, std::vector<NodePtr>> nodes;
				for(auto list : requests) {
					for(auto id : list.second) {
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
			class GhostSyncLinker : public fpmas::api::synchro::SyncLinker<T> {

				public:
					typedef fpmas::api::graph::parallel::DistributedArc<T> ArcApi;
					typedef graph::parallel::ArcPtrWrapper<T> ArcPtr;
					typedef api::communication::TypedMpi<ArcPtr> ArcMpi;
					typedef api::communication::TypedMpi<DistributedId> IdMpi;
				private:
					std::vector<ArcPtr> link_buffer;
					//std::vector<DistributedId> unlink_buffer;
					std::unordered_map<int, std::vector<DistributedId>> unlink_migration;

					ArcMpi& arc_mpi;
					IdMpi& id_mpi;
					fpmas::api::graph::parallel::DistributedGraph<T>& graph;

				public:
					GhostSyncLinker(
							ArcMpi& arc_mpi, IdMpi& id_mpi,
							fpmas::api::graph::parallel::DistributedGraph<T>& graph)
						: arc_mpi(arc_mpi), id_mpi(id_mpi), graph(graph) {}

					void link(const ArcApi*) override;
					void unlink(const ArcApi*) override;

					void synchronize() override;
			};

		template<typename T>
			void GhostSyncLinker<T>::link(const ArcApi * arc) {
				if(arc->state() == LocationState::DISTANT) {
					link_buffer.push_back(const_cast<ArcApi*>(arc));
				}
			}

		template<typename T>
			void GhostSyncLinker<T>::unlink(const ArcApi * arc) {
				if(arc->state() == LocationState::DISTANT) {
					link_buffer.erase(
							std::remove(link_buffer.begin(), link_buffer.end(), arc),
							link_buffer.end()
							);
					auto src = arc->getSourceNode();
					if(src->state() == LocationState::DISTANT) {
						unlink_migration[src->getLocation()].push_back(arc->getId());
					}
					auto tgt = arc->getTargetNode();
					if(tgt->state() == LocationState::DISTANT) {
						unlink_migration[tgt->getLocation()].push_back(arc->getId());
					}
				}
			}

		template<typename T>
			void GhostSyncLinker<T>::synchronize() {
				/*
				 * Migrate links
				 */
				std::vector<ArcPtr> arcs_to_clear;
				std::unordered_map<int, std::vector<ArcPtr>> link_migration;
				for(auto arc : link_buffer) {
					auto src = arc->getSourceNode();
					if(src->state() == LocationState::DISTANT) {
						link_migration[src->getLocation()].push_back(arc);
					}
					auto tgt = arc->getTargetNode();
					if(tgt->state() == LocationState::DISTANT) {
						link_migration[tgt->getLocation()].push_back(arc);
					}
					if(src->state() == LocationState::DISTANT
						&& tgt->state() == LocationState::DISTANT) {
						arcs_to_clear.push_back(arc);
					}
				}
				link_migration = arc_mpi.migrate(link_migration);

				for(auto import_list : link_migration) {
					for (auto arc : import_list.second) {
						graph.importArc(arc);
					}
				}
				link_buffer.clear();

				/*
				 * Migrate unlinks
				 */
   /*             for(auto arc : unlink_buffer) {*/
					//auto src = arc->getSourceNode();
					//if(src->state() == LocationState::DISTANT) {
						//unlink_migration[src->getLocation()].push_back(arc->getId());
					//}
					//auto tgt = arc->getTargetNode();
					//if(tgt->state() == LocationState::DISTANT) {
						//unlink_migration[tgt->getLocation()].push_back(arc->getId());
					//}
				/*}*/

				unlink_migration = id_mpi.migrate(unlink_migration);
				for(auto import_list : unlink_migration) {
					for(DistributedId id : import_list.second) {
						if(graph.getArcs().count(id) > 0) {
							auto arc = graph.getArc(id);
							graph.erase(arc);
						}
					}
				}
				unlink_migration.clear();
				for(auto arc : arcs_to_clear)
					graph.erase(arc);
			}

		template<typename T>
			class GhostModeRuntime : fpmas::api::synchro::SyncModeRuntime<T> {
				communication::TypedMpi<DistributedId> id_mpi;
				communication::TypedMpi<graph::parallel::NodePtrWrapper<T>> node_mpi;
				communication::TypedMpi<graph::parallel::ArcPtrWrapper<T>> arc_mpi;

				GhostDataSync<T> data_sync;
				GhostSyncLinker<T> sync_linker;

				public:
				GhostModeRuntime(
						fpmas::api::graph::parallel::DistributedGraph<T>& graph,
						api::communication::MpiCommunicator& comm)
					: id_mpi(comm), node_mpi(comm), arc_mpi(comm),
					data_sync(node_mpi, id_mpi, graph), sync_linker(arc_mpi, id_mpi, graph) {}

				SingleThreadMutex<T>* buildMutex(api::graph::parallel::DistributedNode<T>* node) override {
					return new SingleThreadMutex<T>(node->data());
				};
				GhostDataSync<T>& getDataSync() override {return data_sync;}
				GhostSyncLinker<T>& getSyncLinker() override {return sync_linker;}
			};
	}
	typedef fpmas::api::synchro::SyncMode<
		synchro::ghost::SingleThreadMutex,
		synchro::ghost::GhostModeRuntime
			> GhostMode;
}
#endif
