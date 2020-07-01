#ifndef HARD_SYNC_LINKER_H
#define HARD_SYNC_LINKER_H

#include "fpmas/utils/macros.h"

#include "fpmas/api/communication/communication.h"
#include "fpmas/api/synchro/sync_mode.h"
#include "fpmas/api/synchro/hard/client_server.h"
#include "fpmas/graph/parallel/distributed_arc.h"
#include "server_pack.h"

namespace fpmas::synchro::hard {

	using api::synchro::hard::Epoch;
	using api::synchro::hard::Tag;
	using api::graph::parallel::LocationState;

	template<typename T>
	class LinkServer
		: public fpmas::api::synchro::hard::LinkServer {
			public:
				typedef api::graph::parallel::DistributedArc<T> ArcApi;
				typedef api::communication::TypedMpi<graph::parallel::ArcPtrWrapper<T>> ArcMpi;
				typedef api::communication::TypedMpi<DistributedId> IdMpi;
			private:
			Epoch epoch = Epoch::EVEN;

			api::communication::MpiCommunicator& comm;
			api::graph::parallel::DistributedGraph<T>& graph;
			IdMpi& id_mpi;
			ArcMpi& arc_mpi;

			public:
				LinkServer(api::communication::MpiCommunicator& comm, api::graph::parallel::DistributedGraph<T>& graph,
						IdMpi& id_mpi, ArcMpi& arc_mpi)
					:  comm(comm), graph(graph), id_mpi(id_mpi), arc_mpi(arc_mpi) {}

				Epoch getEpoch() const override {return epoch;}
				void setEpoch(Epoch epoch) override {this->epoch = epoch;}

				void handleIncomingRequests() override;
		};

	template<typename T>
		void LinkServer<T>::handleIncomingRequests() {
			MPI_Status req_status;
			// Check read request
			if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::LINK, &req_status)) {
				ArcApi* arc = arc_mpi.recv(&req_status);
				FPMAS_LOGD(this->comm.getRank(), "LINK_SERVER", "receive link request from %i", req_status.MPI_SOURCE);
				graph.importArc(arc);
			}
			if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLINK, &req_status)) {
				DistributedId unlink_id = id_mpi.recv(&req_status);
				FPMAS_LOGD(this->comm.getRank(), "LINK_SERVER", "receive unlink request %s from %i", ID_C_STR(unlink_id), req_status.MPI_SOURCE);
				//graph.clearArc(graph.getArc(unlinkId));
				graph.erase(graph.getArc(unlink_id));
			}
		}

	template<typename T>
		class LinkClient : public fpmas::api::synchro::hard::LinkClient<T> {
			typedef fpmas::api::synchro::hard::LinkServer LinkServer;
			public:
				typedef api::graph::parallel::DistributedArc<T> ArcApi;
				typedef api::communication::TypedMpi<graph::parallel::ArcPtrWrapper<T>> ArcMpi;
				typedef api::communication::TypedMpi<DistributedId> IdMpi;

			private:
				api::communication::MpiCommunicator& comm;
				IdMpi& id_mpi;
				ArcMpi& arc_mpi;
				ServerPack<T>& server_pack;

			public:
				LinkClient(api::communication::MpiCommunicator& comm, IdMpi& id_mpi, ArcMpi& arc_mpi,
						ServerPack<T>& server_pack)
					: comm(comm), id_mpi(id_mpi), arc_mpi(arc_mpi), server_pack(server_pack) {}

				void link(const ArcApi*) override;
				void unlink(const ArcApi*) override;
		};

	template<typename T>
		void LinkClient<T>::link(const ArcApi* arc) {
			if(arc->state() == LocationState::DISTANT) {
				bool distantSrc = arc->getSourceNode()->state() == LocationState::DISTANT;
				bool distantTgt = arc->getTargetNode()->state() == LocationState::DISTANT;

				if(arc->getSourceNode()->getLocation() != arc->getTargetNode()->getLocation()) {
					// The arc is DISTANT, so at least of the two nodes is
					// DISTANT. In this case, if the two nodes are
					// DISTANT, they don't have the same location, so two
					// requests must be performed.
					MPI_Request reqSrc;
					MPI_Request reqTgt;
					// Simultaneously initiate the two requests, that are potentially
					// made to different procs
					if(distantSrc) {
						arc_mpi.Issend(
								const_cast<ArcApi*>(arc), arc->getSourceNode()->getLocation(),
								server_pack.getEpoch() | Tag::LINK, &reqSrc
								); 
					}
					if(distantTgt) {
						arc_mpi.Issend(
								const_cast<ArcApi*>(arc), arc->getTargetNode()->getLocation(),
								server_pack.getEpoch() | Tag::LINK, &reqTgt
								); 
					}
					// Sequentially waits for each request : if reqTgt completes
					// before reqSrc, the second waitSendRequest will immediatly return
					// so there is no performance issue.
					if(distantSrc) {
						server_pack.waitSendRequest(&reqSrc);
					}
					if(distantTgt) {
						server_pack.waitSendRequest(&reqTgt);
					}
				} else {
					// The arc is DISTANT, and its source and target nodes
					// locations are the same, so the two nodes are necessarily
					// located on the same DISTANT proc : only need to perform
					// one request
					MPI_Request req;
					arc_mpi.Issend(
							const_cast<ArcApi*>(arc), arc->getSourceNode()->getLocation(),
							server_pack.getEpoch() | Tag::LINK, &req
							); 
					server_pack.waitSendRequest(&req);
				}
			}
		}

	template<typename T>
		void LinkClient<T>::unlink(const ArcApi* arc) {
			if(arc->state() == LocationState::DISTANT) {
				bool distantSrc = arc->getSourceNode()->state() == LocationState::DISTANT;
				bool distantTgt = arc->getTargetNode()->state() == LocationState::DISTANT;

				MPI_Request reqSrc;
				MPI_Request reqTgt;
				// Simultaneously initiate the two requests, that are potentially
				// made to different procs
				if(distantSrc) {
					this->id_mpi.Issend(
							arc->getId(), arc->getSourceNode()->getLocation(),
							server_pack.getEpoch() | Tag::UNLINK, &reqSrc
							); 
				}
				if(distantTgt) {
					this->id_mpi.Issend(
							arc->getId(), arc->getTargetNode()->getLocation(),
							server_pack.getEpoch() | Tag::UNLINK, &reqTgt
							); 
				}
				// Sequentially waits for each request : if reqTgt completes
				// before reqSrc, the second waitSendRequest will immediatly return
				// so there is no performance issue.
				if(distantSrc) {
					server_pack.waitSendRequest(&reqSrc);
				}
				if(distantTgt) {
					server_pack.waitSendRequest(&reqTgt);
				}
			}
		}

	template<typename T>
	class HardSyncLinker : public fpmas::api::synchro::SyncLinker<T> {
		public:
			typedef api::synchro::hard::TerminationAlgorithm
				TerminationAlgorithm;
			typedef api::graph::parallel::DistributedArc<T> ArcApi;

		private:
			typedef api::synchro::hard::LinkClient<T> LinkClient;
			typedef api::synchro::hard::LinkServer LinkServer;

			std::vector<ArcApi*> ghost_arcs;
			api::graph::parallel::DistributedGraph<T>& graph;
			LinkClient& link_client;
			ServerPack<T>& server_pack;

		public:
			HardSyncLinker(api::graph::parallel::DistributedGraph<T>& graph,
					LinkClient& link_client, ServerPack<T>& server_pack)
				: graph(graph), link_client(link_client), server_pack(server_pack) {}

			void link(const ArcApi* arc) override {
				link_client.link(arc);
				
				if(arc->getSourceNode()->state() == LocationState::DISTANT
						&& arc->getTargetNode()->state() == LocationState::DISTANT) {
					ghost_arcs.push_back(const_cast<ArcApi*>(arc));
				}
			};

			void unlink(const ArcApi* arc) override {
				link_client.unlink(arc);
			};

			void synchronize() override {
				FPMAS_LOGI(graph.getMpiCommunicator().getRank(), "HARD_SYNC_LINKER", "Synchronizing sync linker...");
				for(auto arc : ghost_arcs)
					graph.erase(arc);
				ghost_arcs.clear();

				server_pack.terminate();
				FPMAS_LOGI(graph.getMpiCommunicator().getRank(), "HARD_SYNC_LINKER", "Synchronized.");
			};
	};
}

#endif
