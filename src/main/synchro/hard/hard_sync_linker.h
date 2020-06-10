#ifndef HARD_SYNC_LINKER_H
#define HARD_SYNC_LINKER_H

#include "utils/macros.h"

#include "api/communication/communication.h"
#include "api/synchro/sync_mode.h"
#include "api/synchro/hard/client_server.h"

namespace FPMAS::synchro::hard {

	using api::synchro::hard::Epoch;
	using api::synchro::hard::Tag;
	using api::graph::parallel::LocationState;

	template<typename T>
	class LinkServer
		: public FPMAS::api::synchro::hard::LinkServer {
			public:
				typedef api::graph::parallel::DistributedArc<T> ArcApi;
				typedef api::communication::TypedMpi<graph::parallel::ArcPtrWrapper<T>> ArcMpi;
				typedef api::communication::TypedMpi<DistributedId> IdMpi;
			private:
			Epoch epoch = Epoch::EVEN;
			api::communication::MpiCommunicator& comm;
			IdMpi& id_mpi;
			ArcMpi& arc_mpi;

			FPMAS::api::graph::parallel::DistributedGraph<T>& graph;

			public:
				LinkServer(
						api::communication::MpiCommunicator& comm,
						IdMpi& id_mpi, ArcMpi& arc_mpi,
						FPMAS::api::graph::parallel::DistributedGraph<T>& graph)
					:  comm(comm), id_mpi(id_mpi), arc_mpi(arc_mpi), graph(graph) {}

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
				FPMAS_LOGD(this->comm.getRank(), "RECV", "link request from %i", req_status.MPI_SOURCE);
				graph.importArc(arc);
			}
			if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLINK, &req_status)) {
				DistributedId unlinkId = id_mpi.recv(&req_status);
				FPMAS_LOGD(this->comm.getRank(), "RECV", "unlink request %s from %i", ID_C_STR(unlinkId), req_status.MPI_SOURCE);
				graph.clear(graph.getArc(unlinkId));
			}
		}

	template<typename T>
		class LinkClient : public FPMAS::api::synchro::hard::LinkClient<T> {
			typedef FPMAS::api::synchro::hard::LinkServer LinkServer;
			public:
				typedef api::graph::parallel::DistributedArc<T> ArcApi;
				typedef api::communication::TypedMpi<graph::parallel::ArcPtrWrapper<T>> ArcMpi;
				typedef api::communication::TypedMpi<DistributedId> IdMpi;

			private:
				api::communication::MpiCommunicator& comm;
				IdMpi& id_mpi;
				ArcMpi& arc_mpi;
				LinkServer& link_server;

				void waitSendRequest(MPI_Request*);

			public:
				LinkClient(api::communication::MpiCommunicator& comm,
						IdMpi& id_mpi, ArcMpi& arc_mpi,
						LinkServer& link_server)
					: comm(comm), id_mpi(id_mpi), arc_mpi(arc_mpi), link_server(link_server) {}

				void link(const ArcApi*) override;
				void unlink(const ArcApi*) override;
		};

	template<typename T>
		void LinkClient<T>::link(const ArcApi* arc) {
			if(arc->state() == LocationState::DISTANT) {
				bool distantSrc = arc->getSourceNode()->state() == LocationState::DISTANT;
				bool distantTgt = arc->getTargetNode()->state() == LocationState::DISTANT;

				MPI_Request reqSrc;
				MPI_Request reqTgt;
				// Simultaneously initiate the two requests, that are potentially
				// made to different procs
				if(distantSrc) {
					arc_mpi.Issend(
							const_cast<ArcApi*>(arc), arc->getSourceNode()->getLocation(),
							link_server.getEpoch() | Tag::LINK, &reqSrc
							); 
				}
				if(distantTgt) {
					arc_mpi.Issend(
							const_cast<ArcApi*>(arc), arc->getTargetNode()->getLocation(),
							link_server.getEpoch() | Tag::LINK, &reqTgt
							); 
				}
				// Sequentially waits for each request : if reqTgt completes
				// before reqSrc, the second waitSendRequest will immediatly return
				// so there is no performance issue.
				if(distantSrc) {
					waitSendRequest(&reqSrc);
				}
				if(distantTgt) {
					waitSendRequest(&reqTgt);
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
							link_server.getEpoch() | Tag::UNLINK, &reqSrc
							); 
				}
				if(distantTgt) {
					this->id_mpi.Issend(
							arc->getId(), arc->getTargetNode()->getLocation(),
							link_server.getEpoch() | Tag::UNLINK, &reqTgt
							); 
				}
				// Sequentially waits for each request : if reqTgt completes
				// before reqSrc, the second waitSendRequest will immediatly return
				// so there is no performance issue.
				if(distantSrc) {
					waitSendRequest(&reqSrc);
				}
				if(distantTgt) {
					waitSendRequest(&reqTgt);
				}
			}
		}

	/*
	 * Allows to respond to other request while a request (sent in a synchronous
	 * message) is sending, in order to avoid deadlock.
	 */
	template<typename T>
	void LinkClient<T>::waitSendRequest(MPI_Request* req) {
		FPMAS_LOGD(this->comm.getRank(), "WAIT", "wait for send");
		bool sent = comm.test(req);

		while(!sent) {
			link_server.handleIncomingRequests();
			sent = comm.test(req);
		}
	}

	template<typename T>
	class HardSyncLinker : public FPMAS::api::synchro::SyncLinker<T> {
		public:
			typedef api::synchro::hard::TerminationAlgorithm
				TerminationAlgorithm;
			typedef api::graph::parallel::DistributedArc<T> ArcApi;

		private:
			typedef api::synchro::hard::LinkClient<T> LinkClient;
			typedef api::synchro::hard::LinkServer LinkServer;

			LinkClient& link_client;
			LinkServer& link_server;

			TerminationAlgorithm& termination;

		public:
			HardSyncLinker(
					LinkClient& link_client, LinkServer& link_server, TerminationAlgorithm& termination)
				: link_client(link_client), link_server(link_server), termination(termination) {}

			const TerminationAlgorithm& getTerminationAlgorithm() const {
				return termination;
			}

			void link(const ArcApi* arc) override {
				link_client.link(arc);
			};

			void unlink(const ArcApi* arc) override {
				link_client.unlink(arc);
			};

			void synchronize() override {
				termination.terminate(link_server);
			};
	};
}

#endif
