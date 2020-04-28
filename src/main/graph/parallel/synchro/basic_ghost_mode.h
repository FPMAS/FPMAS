#ifndef BASIC_GHOST_MODE_H
#define BASIC_GHOST_MODE_H

#include <vector>
#include "api/graph/parallel/synchro/sync_mode.h"
#include "api/graph/parallel/synchro/mutex.h"
#include "communication/communication.h"

namespace FPMAS::graph::parallel::synchro {

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
			void release(T&&) override {};

			void lock() override {};
			void unlock() override {};
	};

	template<typename NodeType, typename ArcType>
	class GhostDataSync : public FPMAS::api::graph::parallel::synchro::DataSync<NodeType, ArcType> {
		public:
			void synchronize(
				FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>& graph
				) override;

	};
	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	class GhostSyncLinker : public FPMAS::api::graph::parallel::synchro::SyncLinker<NodeType, ArcType> {

		std::vector<const ArcType*> linkBuffer;
		std::vector<const ArcType*> unlinkBuffer;

		FPMAS::api::communication::MpiCommunicator& comm;
		Mpi<ArcType> arcMpi;
		Mpi<DistributedId> distIdMpi;
		public:
			GhostSyncLinker(FPMAS::api::communication::MpiCommunicator& comm)
				: comm(comm), arcMpi(comm), distIdMpi(comm) {}

			void link(const ArcType*) override;
			void unlink(const ArcType*) override;

			void synchronize(
				FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>& graph
				) override;

			const Mpi<ArcType>& getArcMpi() const {return arcMpi;}
			const Mpi<DistributedId>& getDistIdMpi() const {return distIdMpi;}
	};

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostSyncLinker<NodeType, ArcType, Mpi>::link(const ArcType * arc) {
		linkBuffer.push_back(arc);
	}

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostSyncLinker<NodeType, ArcType, Mpi>::unlink(const ArcType * arc) {
		linkBuffer.erase(
			std::remove(linkBuffer.begin(), linkBuffer.end(), arc),
			linkBuffer.end()
			);
		unlinkBuffer.push_back(arc);
	}

	template<typename NodeType, typename ArcType, template<typename> class Mpi>
	void GhostSyncLinker<NodeType, ArcType, Mpi>::synchronize(
			FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>& graph
			) {
		//auto& comm = graph.getMpiCommunicator();
		int currentLocation = comm.getRank();
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
					auto src = arc->getSourceNode();
					auto tgt = arc->getTargetNode();
					graph.erase(graph.getArc(id));
					if(src->state() == LocationState::DISTANT) {
						graph.clear(src);
					}
					if(tgt->state() == LocationState::DISTANT) {
						graph.clear(tgt);
					}
				}
			}
		}
	}

	template<typename DistArc, typename DistNode>
		using DefaultGhostSyncLinker = GhostSyncLinker<DistNode, DistArc, communication::Mpi>;


	template<template<typename> class Mpi>
		using GhostMode = FPMAS::api::graph::parallel::synchro::SyncMode<SingleThreadMutex, DefaultGhostSyncLinker, GhostDataSync>;
}
#endif
