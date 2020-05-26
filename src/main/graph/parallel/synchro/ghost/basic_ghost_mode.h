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
			Mpi<NodeType> nodeMpi;
			Mpi<DistributedId> distIdMpi;
			FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>& graph;

		public:
			const Mpi<NodeType>& getNodeMpi() const {return nodeMpi;}
			const Mpi<DistributedId>& getDistIdMpi() const {return distIdMpi;}

			GhostDataSync(
				FPMAS::api::communication::MpiCommunicator& comm,
				FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>& graph
				)
				: comm(comm), nodeMpi(comm), distIdMpi(comm), graph(graph) {}

			void synchronize() override;
	};

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostDataSync<NodeType, ArcType, Mpi>::synchronize() {
		std::unordered_map<int, std::vector<DistributedId>> requests;
		for(auto node : graph.getLocationManager().getDistantNodes()) {
			requests[node.second->getLocation()].push_back(node.first);
		}
		requests = distIdMpi.migrate(requests);

		std::unordered_map<int, std::vector<NodeType>> nodes;
		for(auto list : requests) {
			for(auto id : list.second) {
				nodes[list.first].push_back(*graph.getNode(id));
			}
		}
		nodes = nodeMpi.migrate(nodes);
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

		std::vector<const ArcType*> linkBuffer;
		std::vector<const ArcType*> unlinkBuffer;

		FPMAS::api::communication::MpiCommunicator& comm;
		Mpi<ArcType> arcMpi;
		Mpi<DistributedId> distIdMpi;
		FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>& graph;

		public:
			GhostSyncLinker(
					FPMAS::api::communication::MpiCommunicator& comm,
					FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>& graph)
				: comm(comm), arcMpi(comm), distIdMpi(comm), graph(graph) {}

			void link(const ArcType*) override;
			void unlink(const ArcType*) override;

			void synchronize() override;

			const Mpi<ArcType>& getArcMpi() const {return arcMpi;}
			const Mpi<DistributedId>& getDistIdMpi() const {return distIdMpi;}
	};

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostSyncLinker<NodeType, ArcType, Mpi>::link(const ArcType * arc) {
		if(arc->state() == LocationState::DISTANT) {
			linkBuffer.push_back(arc);
		}
	}

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostSyncLinker<NodeType, ArcType, Mpi>::unlink(const ArcType * arc) {
		if(arc->state() == LocationState::DISTANT) {
			linkBuffer.erase(
					std::remove(linkBuffer.begin(), linkBuffer.end(), arc),
					linkBuffer.end()
					);
			unlinkBuffer.push_back(arc);
		}
	}

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostSyncLinker<NodeType, ArcType, Mpi>::synchronize() {
		/*
		 * Migrate links
		 */
		std::unordered_map<int, std::vector<ArcType>> linkMigration;
		for(auto arc : linkBuffer) {
			auto src = arc->getSourceNode();
			if(src->state() == LocationState::DISTANT) {
				linkMigration[src->getLocation()].push_back(*arc);
			}
			auto tgt = arc->getTargetNode();
			if(tgt->state() == LocationState::DISTANT) {
				linkMigration[tgt->getLocation()].push_back(*arc);
			}
		}
		linkMigration = arcMpi.migrate(linkMigration);

		for(auto importList : linkMigration) {
			for (const auto& arc : importList.second) {
				graph.importArc(arc);
				delete arc.getSourceNode();
				delete arc.getTargetNode();
			}
		}

		/*
		 * Migrate unlinks
		 */
		std::unordered_map<int, std::vector<DistributedId>> unlinkMigration;
		for(auto arc : unlinkBuffer) {
			auto src = arc->getSourceNode();
			if(src->state() == LocationState::DISTANT) {
				unlinkMigration[src->getLocation()].push_back(arc->getId());
			}
			auto tgt = arc->getTargetNode();
			if(tgt->state() == LocationState::DISTANT) {
				unlinkMigration[tgt->getLocation()].push_back(arc->getId());
			}
		}
		unlinkMigration = distIdMpi.migrate(unlinkMigration);
		for(auto importList : unlinkMigration) {
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
			GhostDataSync<Node, Arc, Mpi> dataSync;
			GhostSyncLinker<Node, Arc, Mpi> syncLinker;

			public:
			GhostModeRuntime(
					FPMAS::api::communication::MpiCommunicator& comm,
					FPMAS::api::graph::parallel::DistributedGraph<Node, Arc>& graph)
				: dataSync(comm, graph), syncLinker(comm, graph) {}

				void setUp(Mutex&) override {};
				GhostDataSync<Node, Arc, Mpi>& getDataSync() override {return dataSync;}
				GhostSyncLinker<Node, Arc, Mpi>& getSyncLinker() override {return syncLinker;}
		};

	/*
	 *template<typename DistArc, typename DistNode>
	 *    using DefaultGhostDataSync = GhostDataSync<DistNode, DistArc, communication::TypedMpi>;
	 *template<typename DistArc, typename DistNode>
	 *    using DefaultGhostSyncLinker = GhostSyncLinker<DistNode, DistArc, communication::TypedMpi>;
	 */

	template<typename DistArc, typename DistNode, typename Mutex>
		using DefaultGhostModeRuntime = GhostModeRuntime<DistNode, DistArc, Mutex, communication::TypedMpi>;

	template<typename T>
		using GhostMode = FPMAS::api::graph::parallel::synchro::SyncMode<SingleThreadMutex, DefaultGhostModeRuntime>;
}
#endif
