#ifndef HARD_SYNC_LINKER_H
#define HARD_SYNC_LINKER_H

#include "utils/macros.h"

#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/sync_mode.h"
#include "api/graph/parallel/synchro/hard/client_server.h"

namespace FPMAS::graph::parallel::synchro::hard {

	using api::graph::parallel::synchro::hard::Epoch;
	using api::graph::parallel::synchro::hard::Tag;
	using api::graph::parallel::LocationState;

	template<typename Node, typename Arc, template<typename> class Mpi>
	class LinkServer
		: public FPMAS::api::graph::parallel::synchro::hard::LinkServer {
			private:
			Epoch epoch = Epoch::EVEN;
			FPMAS::api::communication::MpiCommunicator& comm;
			Mpi<DistributedId> id_mpi;
			Mpi<Arc> arc_mpi;

			FPMAS::api::graph::parallel::DistributedGraph<typename Node::DataType>& graph;

			public:
				LinkServer(
					FPMAS::api::communication::MpiCommunicator& comm,
					FPMAS::api::graph::parallel::DistributedGraph<typename Node::DataType>& graph)
					: comm(comm), id_mpi(comm), arc_mpi(comm), graph(graph) {}

				Mpi<DistributedId>& getIdMpi() {return id_mpi;};
				Mpi<Arc>& getArcMpi() {return arc_mpi;};

				Epoch getEpoch() const override {return epoch;}
				void setEpoch(Epoch epoch) override {this->epoch = epoch;}

				void handleIncomingRequests() override;
		};

	template<typename Node, typename Arc, template<typename> class Mpi>
		void LinkServer<Node, Arc, Mpi>::handleIncomingRequests() {
			MPI_Status req_status;
			// Check read request
			if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::LINK, &req_status)) {
				Arc arc = arc_mpi.recv(&req_status);
				FPMAS_LOGD(this->comm.getRank(), "RECV", "link request from %i", req_status.MPI_SOURCE);
				graph.importArc(arc);
			}
			if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLINK, &req_status)) {
				DistributedId unlinkId = id_mpi.recv(&req_status);
				FPMAS_LOGD(this->comm.getRank(), "RECV", "unlink request %s from %i", ID_C_STR(unlinkId), req_status.MPI_SOURCE);
				graph.clear(graph.getArc(unlinkId));
			}
		}

	template<typename Arc, template<typename> class Mpi>
		class LinkClient : public FPMAS::api::graph::parallel::synchro::hard::LinkClient<Arc> {
			typedef FPMAS::api::communication::MpiCommunicator comm_t;
			typedef FPMAS::api::graph::parallel::synchro::hard::LinkServer link_server;
			private:
				comm_t& comm;
				Mpi<DistributedId> id_mpi;
				Mpi<Arc> arc_mpi;
				link_server& linkServer;

				void waitSendRequest(MPI_Request*);

			public:
				LinkClient(comm_t& comm, link_server& linkServer)
					: comm(comm), id_mpi(comm), arc_mpi(comm), linkServer(linkServer) {}

				Mpi<DistributedId>& getIdMpi() {return id_mpi;}
				Mpi<Arc>& getArcMpi() {return arc_mpi;}

				void link(const Arc*) override;
				void unlink(const Arc*) override;
		};

	template<typename Arc, template<typename> class Mpi>
		void LinkClient<Arc, Mpi>::link(const Arc* arc) {
			if(arc->state() == LocationState::DISTANT) {
				bool distantSrc = arc->getSourceNode()->state() == LocationState::DISTANT;
				bool distantTgt = arc->getTargetNode()->state() == LocationState::DISTANT;

				MPI_Request reqSrc;
				MPI_Request reqTgt;
				// Simultaneously initiate the two requests, that are potentially
				// made to different procs
				if(distantSrc) {
					arc_mpi.Issend(
							*arc, arc->getSourceNode()->getLocation(),
							linkServer.getEpoch() | Tag::LINK, &reqSrc
							); 
				}
				if(distantTgt) {
					arc_mpi.Issend(
							*arc, arc->getTargetNode()->getLocation(),
							linkServer.getEpoch() | Tag::LINK, &reqTgt
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

	template<typename Arc, template<typename> class Mpi>
		void LinkClient<Arc, Mpi>::unlink(const Arc* arc) {
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
							linkServer.getEpoch() | Tag::UNLINK, &reqSrc
							); 
				}
				if(distantTgt) {
					this->id_mpi.Issend(
							arc->getId(), arc->getTargetNode()->getLocation(),
							linkServer.getEpoch() | Tag::UNLINK, &reqTgt
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
	template<typename T, template<typename> class Mpi>
	void LinkClient<T, Mpi>::waitSendRequest(MPI_Request* req) {
		FPMAS_LOGD(this->comm.getRank(), "WAIT", "wait for send");
		bool sent = comm.test(req);

		while(!sent) {
			linkServer.handleIncomingRequests();
			sent = comm.test(req);
		}
	}

	template<typename Arc, typename TerminationAlgorithm>
	class HardSyncLinker : public FPMAS::api::graph::parallel::synchro::SyncLinker<Arc> {
		private:
			typedef FPMAS::api::graph::parallel::synchro::hard::LinkClient<Arc> link_client;
			typedef FPMAS::api::graph::parallel::synchro::hard::LinkServer link_server;

			FPMAS::api::communication::MpiCommunicator& comm;
			link_client& linkClient;
			link_server& linkServer;

			TerminationAlgorithm termination {comm};

		public:
			HardSyncLinker(FPMAS::api::communication::MpiCommunicator& comm, link_client& linkClient, link_server& linkServer)
				: comm(comm), linkClient(linkClient), linkServer(linkServer) {}

			const TerminationAlgorithm& getTerminationAlgorithm() const {
				return termination;
			}

			void link(const Arc* arc) override {
				linkClient.link(arc);
			};

			void unlink(const Arc* arc) override {
				linkClient.unlink(arc);
			};

			void synchronize() override {
				termination.terminate(linkServer);
			};

	};
}

#endif
