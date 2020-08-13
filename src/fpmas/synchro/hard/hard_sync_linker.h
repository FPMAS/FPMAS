#ifndef HARD_SYNC_LINKER_H
#define HARD_SYNC_LINKER_H

#include "fpmas/utils/macros.h"

#include "fpmas/api/communication/communication.h"
#include "fpmas/api/synchro/sync_mode.h"
#include "./api/client_server.h"
#include "fpmas/graph/distributed_edge.h"
#include "server_pack.h"

namespace fpmas { namespace synchro { namespace hard {
	using api::Tag;
	using api::Epoch;
	using fpmas::api::graph::LocationState;

	/**
	 * api::LinkServer implemenation.
	 */
	template<typename T>
		class LinkServer : public api::LinkServer {
			public:
				typedef fpmas::api::graph::DistributedEdge<T> EdgeApi;
				typedef fpmas::api::communication::TypedMpi<graph::EdgePtrWrapper<T>> EdgeMpi;
				typedef fpmas::api::communication::TypedMpi<DistributedId> IdMpi;
			private:
				Epoch epoch = Epoch::EVEN;

				fpmas::api::communication::MpiCommunicator& comm;
				fpmas::api::graph::DistributedGraph<T>& graph;
				IdMpi& id_mpi;
				EdgeMpi& edge_mpi;

			public:
				/**
				 * LinkServer constructor.
				 *
				 * @param comm MPI communicator
				 * @param graph associated api::graph::DistributedGraph
				 * @param id_mpi Typed MPI used to transmit DistributedId
				 * @param edge_mpi Typed MPI used to transmit edges
				 */
				LinkServer(fpmas::api::communication::MpiCommunicator& comm, fpmas::api::graph::DistributedGraph<T>& graph,
						IdMpi& id_mpi, EdgeMpi& edge_mpi)
					:  comm(comm), graph(graph), id_mpi(id_mpi), edge_mpi(edge_mpi) {}

				Epoch getEpoch() const override {return epoch;}
				void setEpoch(Epoch epoch) override {this->epoch = epoch;}

				void handleIncomingRequests() override;
		};

	template<typename T>
		void LinkServer<T>::handleIncomingRequests() {
			MPI_Status req_status;
			// Check read request
			if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::LINK, &req_status)) {
				EdgeApi* edge = edge_mpi.recv(req_status.MPI_SOURCE, req_status.MPI_TAG);
				FPMAS_LOGD(this->comm.getRank(), "LINK_SERVER", "receive link request from %i", req_status.MPI_SOURCE);
				graph.importEdge(edge);
			}
			if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLINK, &req_status)) {
				DistributedId unlink_id = id_mpi.recv(req_status.MPI_SOURCE, req_status.MPI_TAG);
				FPMAS_LOGD(this->comm.getRank(), "LINK_SERVER", "receive unlink request %s from %i", ID_C_STR(unlink_id), req_status.MPI_SOURCE);
				//graph.clearEdge(graph.getEdge(unlinkId));
				graph.erase(graph.getEdge(unlink_id));
			}
		}

	/**
	 * api::LinkClient implementation.
	 */
	template<typename T>
		class LinkClient : public api::LinkClient<T> {
			public:
			typedef fpmas::api::graph::DistributedEdge<T> EdgeApi;
			typedef fpmas::api::communication::TypedMpi<graph::EdgePtrWrapper<T>> EdgeMpi;
			typedef fpmas::api::communication::TypedMpi<DistributedId> IdMpi;

			private:
			fpmas::api::communication::MpiCommunicator& comm;
			IdMpi& id_mpi;
			EdgeMpi& edge_mpi;
			ServerPack<T>& server_pack;

			public:
			/**
			 * LinkClient constructor.
			 *
			 * @param comm MPI communicator
			 * @param id_mpi Typed MPI used to transmit DistributedId
			 * @param edge_mpi Typed MPI used to transmit edges
			 * @param server_pack associated ServerPack. The ServerPack is used
			 * to handle incoming requests (see ServerPack::waitSendRequest())
			 * while the client is waiting for requests to be sent, in order to
			 * avoid deadlocks.
			 */
			LinkClient(fpmas::api::communication::MpiCommunicator& comm, IdMpi& id_mpi, EdgeMpi& edge_mpi,
					ServerPack<T>& server_pack)
				: comm(comm), id_mpi(id_mpi), edge_mpi(edge_mpi), server_pack(server_pack) {}

			/**
			 * \copydoc api::LinkClient::link
			 */
			void link(const EdgeApi*) override;

			/** 
			 * \copydoc api::LinkClient::unlink
			 */
			void unlink(const EdgeApi*) override;
		};

	template<typename T>
		void LinkClient<T>::link(const EdgeApi* edge) {
			bool distantSrc = edge->getSourceNode()->state() == LocationState::DISTANT;
			bool distantTgt = edge->getTargetNode()->state() == LocationState::DISTANT;

			if(edge->getSourceNode()->getLocation() != edge->getTargetNode()->getLocation()) {
				// The edge is DISTANT, so at least of the two nodes is
				// DISTANT. In this case, if the two nodes are
				// DISTANT, they don't have the same location, so two
				// requests must be performed.
				MPI_Request reqSrc;
				MPI_Request reqTgt;
				// Simultaneously initiate the two requests, that are potentially
				// made to different procs
				if(distantSrc) {
					edge_mpi.Issend(
							const_cast<EdgeApi*>(edge), edge->getSourceNode()->getLocation(),
							server_pack.getEpoch() | Tag::LINK, &reqSrc
							); 
				}
				if(distantTgt) {
					edge_mpi.Issend(
							const_cast<EdgeApi*>(edge), edge->getTargetNode()->getLocation(),
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
				// The edge is DISTANT, and its source and target nodes
				// locations are the same, so the two nodes are necessarily
				// located on the same DISTANT proc : only need to perform
				// one request
				MPI_Request req;
				edge_mpi.Issend(
						const_cast<EdgeApi*>(edge), edge->getSourceNode()->getLocation(),
						server_pack.getEpoch() | Tag::LINK, &req
						); 
				server_pack.waitSendRequest(&req);
			}
		}

	template<typename T>
		void LinkClient<T>::unlink(const EdgeApi* edge) {
			bool distantSrc = edge->getSourceNode()->state() == LocationState::DISTANT;
			bool distantTgt = edge->getTargetNode()->state() == LocationState::DISTANT;

			MPI_Request reqSrc;
			MPI_Request reqTgt;
			// Simultaneously initiate the two requests, that are potentially
			// made to different procs
			if(distantSrc) {
				this->id_mpi.Issend(
						edge->getId(), edge->getSourceNode()->getLocation(),
						server_pack.getEpoch() | Tag::UNLINK, &reqSrc
						); 
			}
			if(distantTgt) {
				this->id_mpi.Issend(
						edge->getId(), edge->getTargetNode()->getLocation(),
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

	/**
	 * HardSyncMode fpmas::api::synchro::SyncLinker implementation.
	 */
	template<typename T>
		class HardSyncLinker : public fpmas::api::synchro::SyncLinker<T> {
			public:
				typedef api::TerminationAlgorithm
					TerminationAlgorithm;
				typedef fpmas::api::graph::DistributedEdge<T> EdgeApi;

			private:
				std::vector<EdgeApi*> ghost_edges;
				fpmas::api::graph::DistributedGraph<T>& graph;
				api::LinkClient<T>& link_client;
				ServerPack<T>& server_pack;

			public:
				/**
				 * HardSyncLinker constructor.
				 *
				 * @param graph associated graph
				 * @param link_client client used to transmit requests
				 * @param server_pack associated server_pack used for
				 * synchronization
				 */
				HardSyncLinker(fpmas::api::graph::DistributedGraph<T>& graph,
						api::LinkClient<T>& link_client, ServerPack<T>& server_pack)
					: graph(graph), link_client(link_client), server_pack(server_pack) {}

				/**
				 * Performs a link request.
				 *
				 * If the specified edge is DISTANT, the request is transmitted
				 * the link request to the api::LinkClient. Nothing needs to be
				 * done otherwise.
				 *
				 * If source and target nodes are both DISTANT, the edge is planned
				 * to be deleted at the next synchronize() call.
				 *
				 * @param edge edge to link
				 */
				void link(const EdgeApi* edge) override {
					if(edge->state() == LocationState::DISTANT) {
						link_client.link(edge);

						if(edge->getSourceNode()->state() == LocationState::DISTANT
								&& edge->getTargetNode()->state() == LocationState::DISTANT) {
							ghost_edges.push_back(const_cast<EdgeApi*>(edge));
						}
					}
				};

				/**
				 * Performs an unlink request.
				 *
				 * If the specified edge is DISTANT, the request is transmitted
				 * the link request to the api::LinkClient. Nothing needs to be
				 * done otherwise.
				 *
				 * @param edge edge to unlink
				 */
				void unlink(const EdgeApi* edge) override {
					if(edge->state() == LocationState::DISTANT) {
						link_client.unlink(edge);
					}
				};
				/**
				 * HardSyncLinker synchronization.
				 *
				 * First, edges linked between a DISTANT source and a DISTANT
				 * target since the last synchronization are erased from the
				 * graph.
				 *
				 * All processes are then synchronized using
				 * ServerPack::terminate().
				 *
				 * Notice that, since ServerPack is used, not only SyncLinker
				 * requests but also HardSyncMutex requests (i.e. data related
				 * requests) will be handled until termination.
				 *
				 * This is required to avoid the following deadlock situation :
				 * | Process 0                        | Process 1
				 * |----------------------------------|---------------------------
				 * | ...                              | ...
				 * | init HardSyncLinker::terminate() | send data request to P0...
				 * | handles link requests...         | waits for data response...
				 * | handles link requests...         | waits for data response...
				 * | DEADLOCK                         ||
				 */
				void synchronize() override {
					FPMAS_LOGI(graph.getMpiCommunicator().getRank(), "HARD_SYNC_LINKER", "Synchronizing sync linker...");
					for(auto edge : ghost_edges)
						graph.erase(edge);
					ghost_edges.clear();

					server_pack.terminate();
					FPMAS_LOGI(graph.getMpiCommunicator().getRank(), "HARD_SYNC_LINKER", "Synchronized.");
				};
		};
}}}
#endif
