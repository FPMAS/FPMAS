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

		public:
			void link(const ArcType*) override;
			void unlink(const ArcType*) override;

			void synchronize(
				FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>& graph
				) override;
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
		auto& comm = graph.getMpiCommunicator();
		int currentLocation = comm.getRank();
		/*
		 * Migrate links
		 */
		std::unordered_map<int, std::vector<ArcType>> linkMigration;
		for(auto arc : linkBuffer) {
			int srcLocation = arc->getSourceNode()->getLocation();
			if(srcLocation != currentLocation) {
				linkMigration[srcLocation].push_back(*arc);
			}
			int tgtLocation = arc->getTargetNode()->getLocation();
			if(tgtLocation != currentLocation) {
				linkMigration[tgtLocation].push_back(*arc);
			}
		}
		linkMigration = Mpi<ArcType>(comm).migrate(linkMigration);

		for(auto importList : linkMigration) {
			for (const auto& arc : importList.second) {
				graph.importArc(arc);
			}
		}

		/*
		 * Migrate unlinks
		 */
		std::unordered_map<int, std::vector<DistributedId>> unlinkMigration;
		for(auto arc : linkBuffer) {
			int srcLocation = arc->getSourceNode()->getLocation();
			if(srcLocation != currentLocation) {
				unlinkMigration[srcLocation].push_back(arc->getId());
			}
			int tgtLocation = arc->getTargetNode()->getLocation();
			if(tgtLocation != currentLocation) {
				unlinkMigration[tgtLocation].push_back(arc->getId());
			}
		}
		unlinkMigration = Mpi<DistributedId>(comm).migrate(unlinkMigration);
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
