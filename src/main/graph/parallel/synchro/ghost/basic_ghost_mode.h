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

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	class GhostDataSync : public FPMAS::api::graph::parallel::synchro::DataSync {
		private:
			FPMAS::api::communication::MpiCommunicator& comm;
			Mpi<NodeType> node_mpi;
			Mpi<DistributedId> dist_id_mpi;
			FPMAS::api::graph::parallel::DistributedGraph<typename NodeType::DataType>& graph;

		public:
			const Mpi<NodeType>& getNodeMpi() const {return node_mpi;}
			const Mpi<DistributedId>& getDistIdMpi() const {return dist_id_mpi;}

			GhostDataSync(
				FPMAS::api::communication::MpiCommunicator& comm,
				FPMAS::api::graph::parallel::DistributedGraph<typename NodeType::DataType>& graph
				)
				: comm(comm), node_mpi(comm), dist_id_mpi(comm), graph(graph) {}

			void synchronize() override;
	};

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostDataSync<NodeType, ArcType, Mpi>::synchronize() {
		std::unordered_map<int, std::vector<DistributedId>> requests;
		for(auto node : graph.getLocationManager().getDistantNodes()) {
			requests[node.second->getLocation()].push_back(node.first);
		}
		requests = dist_id_mpi.migrate(requests);

		std::unordered_map<int, std::vector<NodeType>> nodes;
		for(auto list : requests) {
			for(auto id : list.second) {
				nodes[list.first].push_back(*static_cast<NodeType*>(graph.getNode(id)));
			}
		}
		nodes = node_mpi.migrate(nodes);
		for(auto list : nodes) {
			for(auto& node : list.second) {
				auto localNode = graph.getNode(node.getId());
				localNode->data() = node.data();
				localNode->setWeight(node.getWeight());
			}
		}
	}


	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	class GhostSyncLinker : public FPMAS::api::graph::parallel::synchro::SyncLinker<ArcType> {

		std::vector<const ArcType*> link_buffer;
		std::vector<const ArcType*> unlink_buffer;

		FPMAS::api::communication::MpiCommunicator& comm;
		Mpi<ArcType> arc_mpi;
		Mpi<DistributedId> dist_id_mpi;
		FPMAS::api::graph::parallel::DistributedGraph<typename NodeType::DataType>& graph;

		public:
			GhostSyncLinker(
					FPMAS::api::communication::MpiCommunicator& comm,
					FPMAS::api::graph::parallel::DistributedGraph<typename NodeType::DataType>& graph)
				: comm(comm), arc_mpi(comm), dist_id_mpi(comm), graph(graph) {}

			void link(const ArcType*) override;
			void unlink(const ArcType*) override;

			void synchronize() override;

			const Mpi<ArcType>& getArcMpi() const {return arc_mpi;}
			const Mpi<DistributedId>& getDistIdMpi() const {return dist_id_mpi;}
	};

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostSyncLinker<NodeType, ArcType, Mpi>::link(const ArcType * arc) {
		if(arc->state() == LocationState::DISTANT) {
			link_buffer.push_back(arc);
		}
	}

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostSyncLinker<NodeType, ArcType, Mpi>::unlink(const ArcType * arc) {
		if(arc->state() == LocationState::DISTANT) {
			link_buffer.erase(
					std::remove(link_buffer.begin(), link_buffer.end(), arc),
					link_buffer.end()
					);
			unlink_buffer.push_back(arc);
		}
	}

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostSyncLinker<NodeType, ArcType, Mpi>::synchronize() {
		/*
		 * Migrate links
		 */
		std::unordered_map<int, std::vector<ArcType>> link_migration;
		for(auto arc : link_buffer) {
			auto src = arc->getSourceNode();
			if(src->state() == LocationState::DISTANT) {
				link_migration[src->getLocation()].push_back(*arc);
			}
			auto tgt = arc->getTargetNode();
			if(tgt->state() == LocationState::DISTANT) {
				link_migration[tgt->getLocation()].push_back(*arc);
			}
		}
		link_migration = arc_mpi.migrate(link_migration);

		for(auto importList : link_migration) {
			for (const auto& arc : importList.second) {
				graph.importArc(arc);
				delete arc.getSourceNode();
				delete arc.getTargetNode();
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
		unlink_migration = dist_id_mpi.migrate(unlink_migration);
		for(auto importList : unlink_migration) {
			for(DistributedId id : importList.second) {
				if(graph.getArcs().count(id) > 0) {
					auto arc = graph.getArc(id);
					graph.clear(arc);
				}
			}
		}
	}

	template<typename Node, typename Arc, typename Mutex, template<typename> class Mpi>
		class GhostModeRuntime : FPMAS::api::graph::parallel::synchro::SyncModeRuntime<Node, Arc, Mutex> {
			GhostDataSync<Node, Arc, Mpi> data_sync;
			GhostSyncLinker<Node, Arc, Mpi> sync_linker;

			public:
			GhostModeRuntime(
					FPMAS::api::communication::MpiCommunicator& comm,
					FPMAS::api::graph::parallel::DistributedGraph<typename Node::DataType>& graph)
				: data_sync(comm, graph), sync_linker(comm, graph) {}

				void setUp(Mutex&) override {};
				GhostDataSync<Node, Arc, Mpi>& getDataSync() override {return data_sync;}
				GhostSyncLinker<Node, Arc, Mpi>& getSyncLinker() override {return sync_linker;}
		};

	template<typename DistArc, typename DistNode, typename Mutex>
		using DefaultGhostModeRuntime = GhostModeRuntime<DistNode, DistArc, Mutex, communication::TypedMpi>;

	template<typename T>
		using GhostMode = FPMAS::api::graph::parallel::synchro::SyncMode<SingleThreadMutex, DefaultGhostModeRuntime>;
}
#endif
