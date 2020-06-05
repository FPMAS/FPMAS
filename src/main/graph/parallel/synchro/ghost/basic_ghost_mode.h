#ifndef BASIC_GHOST_MODE_H
#define BASIC_GHOST_MODE_H

#include <vector>
#include "api/graph/parallel/synchro/sync_mode.h"
#include "api/graph/parallel/synchro/mutex.h"
#include "communication/communication.h"

namespace FPMAS::graph::parallel::synchro::ghost {

	using api::graph::parallel::LocationState;

	template<typename T>
	class SingleThreadMutex : public FPMAS::api::graph::parallel::synchro::Mutex<T> {
		private:
			T& data;

		public:
			SingleThreadMutex(T& data) : data(data) {}

			// TODO: Exceptions when multiple lock
			const T& read() override {return data;};
			T& acquire() override {return data;};
			void release() override {};

			void lock() override {};
			void lockShared() override {};
			void unlock() override {};
			int sharedLock() override {return 0;};
	};

	template<typename T>
	class GhostDataSync : public FPMAS::api::graph::parallel::synchro::DataSync {
		public:
			typedef api::graph::parallel::DistributedNode<T> NodeApi;
			typedef api::communication::TypedMpi<NodeApi*> NodeMpi;

			typedef api::communication::TypedMpi<DistributedId> IdMpi;

		private:
			NodeMpi& node_mpi;
			IdMpi& id_mpi;

			FPMAS::api::graph::parallel::DistributedGraph<T>& graph;

		public:
			GhostDataSync(
				NodeMpi& node_mpi, IdMpi& id_mpi,
				FPMAS::api::graph::parallel::DistributedGraph<T>& graph
				)
				: node_mpi(node_mpi), id_mpi(id_mpi), graph(graph) {}

			void synchronize() override;
	};

	template<typename T>
	void GhostDataSync<T>::synchronize() {
		std::unordered_map<int, std::vector<DistributedId>> requests;
		for(auto node : graph.getLocationManager().getDistantNodes()) {
			requests[node.second->getLocation()].push_back(node.first);
		}
		requests = id_mpi.migrate(requests);

		std::unordered_map<int, std::vector<NodeApi*>> nodes;
		for(auto list : requests) {
			for(auto id : list.second) {
				nodes[list.first].push_back(graph.getNode(id));
			}
		}
		nodes = node_mpi.migrate(nodes);
		for(auto list : nodes) {
			for(auto& node : list.second) {
				auto localNode = graph.getNode(node->getId());
				localNode->mutex().data() = node->mutex().data();
				localNode->setWeight(node->getWeight());
			}
		}
	}


	template<typename T>
	class GhostSyncLinker : public FPMAS::api::graph::parallel::synchro::SyncLinker<T> {

		public:
			typedef api::graph::parallel::DistributedArc<T> ArcApi;
			typedef api::communication::TypedMpi<ArcApi*> ArcMpi;
			typedef api::communication::TypedMpi<DistributedId> IdMpi;
		private:
		std::vector<ArcApi*> link_buffer;
		std::vector<ArcApi*> unlink_buffer;

		ArcMpi& arc_mpi;
		IdMpi& id_mpi;
		FPMAS::api::graph::parallel::DistributedGraph<T>& graph;

		public:
			GhostSyncLinker(
					ArcMpi& arc_mpi, IdMpi& id_mpi,
					FPMAS::api::graph::parallel::DistributedGraph<T>& graph)
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
			unlink_buffer.push_back(const_cast<ArcApi*>(arc));
		}
	}

	template<typename T>
	void GhostSyncLinker<T>::synchronize() {
		/*
		 * Migrate links
		 */
		std::unordered_map<int, std::vector<ArcApi*>> link_migration;
		for(auto arc : link_buffer) {
			auto src = arc->getSourceNode();
			if(src->state() == LocationState::DISTANT) {
				link_migration[src->getLocation()].push_back(arc);
			}
			auto tgt = arc->getTargetNode();
			if(tgt->state() == LocationState::DISTANT) {
				link_migration[tgt->getLocation()].push_back(arc);
			}
		}
		link_migration = arc_mpi.migrate(link_migration);

		for(auto importList : link_migration) {
			for (const auto& arc : importList.second) {
				graph.importArc(arc);
				// TODO : remove
				//delete arc.getSourceNode();
				//delete arc.getTargetNode();
			}
		}

		/*
		 * Migrate unlinks
		 */
		std::unordered_map<int, std::vector<DistributedId>> unlink_migration;
		for(auto arc : unlink_buffer) {
			auto src = arc->getSourceNode();
			if(src->state() == LocationState::DISTANT) {
				unlink_migration[src->getLocation()].push_back(arc->getId());
			}
			auto tgt = arc->getTargetNode();
			if(tgt->state() == LocationState::DISTANT) {
				unlink_migration[tgt->getLocation()].push_back(arc->getId());
			}
		}
		unlink_migration = id_mpi.migrate(unlink_migration);
		for(auto importList : unlink_migration) {
			for(DistributedId id : importList.second) {
				if(graph.getArcs().count(id) > 0) {
					auto arc = graph.getArc(id);
					graph.clear(arc);
				}
			}
		}
	}

	template<typename T>
		class GhostModeRuntime : FPMAS::api::graph::parallel::synchro::SyncModeRuntime<T> {
			GhostDataSync<T> data_sync;
			GhostSyncLinker<T> sync_linker;

			public:
			typedef api::graph::parallel::DistributedNode<T> NodeApi;
			typedef api::graph::parallel::DistributedArc<T> ArcApi;
			GhostModeRuntime(
					api::communication::TypedMpi<NodeApi*&> node_mpi,
					api::communication::TypedMpi<ArcApi*>& arc_mpi,
					api::communication::TypedMpi<DistributedId>& id_mpi,
					FPMAS::api::graph::parallel::DistributedGraph<T>& graph)
				: data_sync(node_mpi, id_mpi, graph), sync_linker(arc_mpi, node_mpi, graph) {}

				SingleThreadMutex<T>& build(DistributedId, T&) override {};
				GhostDataSync<T>& getDataSync() override {return data_sync;}
				GhostSyncLinker<T>& getSyncLinker() override {return sync_linker;}
		};

	template<typename T>
		using GhostMode = FPMAS::api::graph::parallel::synchro::SyncMode<SingleThreadMutex, GhostModeRuntime>;
}
#endif
