#ifndef FPMAS_HARD_SYNC_LINKER_H
#define FPMAS_HARD_SYNC_LINKER_H

/** \file src/fpmas/synchro/hard/hard_sync_linker.h
 * HardSyncMode SyncLinker implementation.
 */

#include <set>

#include "fpmas/utils/macros.h"

#include "fpmas/graph/distributed_graph.h"
#include "fpmas/synchro/hard/api/hard_sync_mode.h"
#include "server_pack.h"

namespace fpmas { namespace synchro { namespace hard {
	using api::Tag;
	using api::Epoch;
	using fpmas::api::graph::LocationState;

	/**
	 * api::LinkServer implementation.
	 */
	template<typename T>
		class LinkServer : public api::LinkServer {
			public:
				/**
				 * DistributedEdge API.
				 */
				typedef fpmas::api::graph::DistributedEdge<T> EdgeApi;
				/**
				 * TypedMpi used to transmit Edges with MPI.
				 */
				typedef fpmas::api::communication::TypedMpi<graph::EdgePtrWrapper<T>> EdgeMpi;
				/**
				 * TypedMpi used to transmit DistributedIds with MPI.
				 */
				typedef fpmas::api::communication::TypedMpi<DistributedId> IdMpi;
			private:
				Epoch epoch = Epoch::EVEN;

				fpmas::api::communication::MpiCommunicator& comm;
				fpmas::api::graph::DistributedGraph<T>& graph;
				api::HardSyncLinker<T>& sync_linker;
				IdMpi& id_mpi;
				EdgeMpi& edge_mpi;
				std::set<DistributedId> locked_unlink_edges;
				std::set<DistributedId> locked_remove_nodes;

			public:
				/**
				 * LinkServer constructor.
				 *
				 * @param comm MPI communicator
				 * @param graph associated api::graph::DistributedGraph
				 * @param sync_linker associated HardSyncLinker
				 * @param id_mpi Typed MPI used to transmit DistributedId
				 * @param edge_mpi Typed MPI used to transmit edges
				 */
				LinkServer(
						fpmas::api::communication::MpiCommunicator& comm,
						fpmas::api::graph::DistributedGraph<T>& graph,
						api::HardSyncLinker<T>& sync_linker,
						IdMpi& id_mpi, EdgeMpi& edge_mpi)
					:  comm(comm), graph(graph), sync_linker(sync_linker), id_mpi(id_mpi), edge_mpi(edge_mpi) {}

				Epoch getEpoch() const override {return epoch;}
				void setEpoch(api::Epoch epoch) override {this->epoch = epoch;}

				void handleIncomingRequests() override;

				
				void lockUnlink(DistributedId edge_id) override {
					locked_unlink_edges.insert(edge_id);
				}
				
				bool isLockedUnlink(DistributedId edge_id) override {
					return locked_unlink_edges.count(edge_id) > 0;
				}
				
				void unlockUnlink(DistributedId edge_id) override {
					locked_unlink_edges.erase(edge_id);
				}

				void lockRemoveNode(DistributedId node_id) override {
					locked_remove_nodes.insert(node_id);
				}

				bool isLockedRemoveNode(DistributedId node_id) override {
					return locked_remove_nodes.count(node_id);
				}

				void unlockRemoveNode(DistributedId node_id) override {
					locked_remove_nodes.erase(node_id);
				}
		};

	template<typename T>
		void LinkServer<T>::handleIncomingRequests() {
			fpmas::api::communication::Status status;
			// Check read request
			if(edge_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::LINK, status)) {
				EdgeApi* edge = edge_mpi.recv(status.source, status.tag);
				FPMAS_LOGD(this->comm.getRank(), "LINK_SERVER", "receive link request from %i", status.source);
				graph.importEdge(edge);
			}
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLINK, status)) {
				DistributedId unlink_id = id_mpi.recv(status.source, status.tag);
				FPMAS_LOGD(this->comm.getRank(), "LINK_SERVER", "receive unlink request %s from %i", FPMAS_C_STR(unlink_id), status.source);
				if(
						// The edge is not being unlinking by the local process
						!isLockedUnlink(unlink_id)
						// The edge has not been unlinked by an other UNLINK
						// operation
						&& graph.getEdges().count(unlink_id) > 0) {
					auto* edge = graph.getEdge(unlink_id);
					// Source or target node is not being removed by the local
					// process. In this case, the local process is responsible
					// for all the required unlink operations, so the incoming
					// request is ignored.
					if(!(isLockedRemoveNode(edge->getSourceNode()->getId()) || isLockedRemoveNode(edge->getTargetNode()->getId()))) {
						graph.erase(edge);
					}
				}
			}
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::REMOVE_NODE, status)) {
				DistributedId node_id = id_mpi.recv(status.source, status.tag);
				FPMAS_LOGD(this->comm.getRank(), "LINK_SERVER", "receive remove node request %s from %i", FPMAS_C_STR(node_id), status.source);

				// Initiates a removeNode operation from the local process,
				// that will trigger all required UNLINK operations
				graph.removeNode(graph.getNode(node_id));
			}
		}

	/**
	 * api::LinkClient implementation.
	 */
	template<typename T>
		class LinkClient : public api::LinkClient<T> {
			public:
				/**
				 * DistributedEdge API.
				 */
				typedef fpmas::api::graph::DistributedEdge<T> EdgeApi;
				/**
				 * DistributedNode API.
				 */
				typedef fpmas::api::graph::DistributedNode<T> NodeApi;
				/**
				 * TypedMpi used to transmit Edges with MPI.
				 */
				typedef fpmas::api::communication::TypedMpi<graph::EdgePtrWrapper<T>> EdgeMpi;
				/**
				 * TypedMpi used to transmit DistributedIds with MPI.
				 */
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

				void link(const EdgeApi*) override;
				void unlink(const EdgeApi*) override;
				void removeNode(const NodeApi*) override;
		};

	template<typename T>
		void LinkClient<T>::link(const EdgeApi* edge) {
			bool distant_src = edge->getSourceNode()->state() == LocationState::DISTANT;
			bool distant_tgt = edge->getTargetNode()->state() == LocationState::DISTANT;

			if(edge->getSourceNode()->location() != edge->getTargetNode()->location()) {
				// The edge is DISTANT, so at least of the two nodes is
				// DISTANT. In this case, if the two nodes are
				// DISTANT, they don't have the same location, so two
				// requests must be performed.
				fpmas::api::communication::Request req_src;
				fpmas::api::communication::Request req_tgt;
				// Simultaneously initiate the two requests, that are potentially
				// made to different procs
				if(distant_src) {
					edge_mpi.Issend(
							const_cast<EdgeApi*>(edge), edge->getSourceNode()->location(),
							server_pack.getEpoch() | Tag::LINK, req_src
							); 
				}
				if(distant_tgt) {
					edge_mpi.Issend(
							const_cast<EdgeApi*>(edge), edge->getTargetNode()->location(),
							server_pack.getEpoch() | Tag::LINK, req_tgt
							); 
				}
				// Sequentially waits for each request : if req_tgt completes
				// before req_src, the second waitSendRequest will immediatly return
				// so there is no performance issue.
				if(distant_src) {
					server_pack.waitSendRequest(req_src);
				}
				if(distant_tgt) {
					server_pack.waitSendRequest(req_tgt);
				}
			} else {
				// The edge is DISTANT, and its source and target nodes
				// locations are the same, so the two nodes are necessarily
				// located on the same DISTANT proc : only need to perform
				// one request
				fpmas::api::communication::Request req;
				edge_mpi.Issend(
						const_cast<EdgeApi*>(edge), edge->getSourceNode()->location(),
						server_pack.getEpoch() | Tag::LINK, req
						); 
				server_pack.waitSendRequest(req);
			}
		}

	template<typename T>
		void LinkClient<T>::unlink(const EdgeApi* edge) {

			bool distant_src = edge->getSourceNode()->state() == LocationState::DISTANT;
			bool distant_tgt = edge->getTargetNode()->state() == LocationState::DISTANT;

			fpmas::api::communication::Request req_src;
			fpmas::api::communication::Request req_tgt;
			// Simultaneously initiate the two requests, that are potentially
			// made to different procs
			if(distant_src) {
				this->id_mpi.Issend(
						edge->getId(), edge->getSourceNode()->location(),
						server_pack.getEpoch() | Tag::UNLINK, req_src
						); 
			}
			if(distant_tgt) {
				this->id_mpi.Issend(
						edge->getId(), edge->getTargetNode()->location(),
						server_pack.getEpoch() | Tag::UNLINK, req_tgt
						); 
			}
			// Sequentially waits for each request : if req_tgt completes
			// before req_src, the second waitSendRequest will immediatly return
			// so there is no performance issue.
			if(distant_src) {
				server_pack.waitSendRequest(req_src);
			}
			if(distant_tgt) {
				server_pack.waitSendRequest(req_tgt);
			}
		}

	template<typename T>
		void LinkClient<T>::removeNode(const NodeApi* node) {
			fpmas::api::communication::Request req;
			this->id_mpi.Issend(
				node->getId(), node->location(),
				server_pack.getEpoch() | Tag::REMOVE_NODE, req
				);

			server_pack.waitSendRequest(req);
		}


	/**
	 * HardSyncMode fpmas::api::synchro::SyncLinker implementation.
	 */
	template<typename T>
		class HardSyncLinker : public api::HardSyncLinker<T> {
			public:
				/**
				 * DistributedEdge API.
				 */
				typedef fpmas::api::graph::DistributedEdge<T> EdgeApi;
				/**
				 * DistributedNode API.
				 */
				typedef fpmas::api::graph::DistributedNode<T> NodeApi;

			private:
				std::vector<EdgeApi*> ghost_edges;
				fpmas::api::graph::DistributedGraph<T>& graph;
				api::LinkClient<T>& link_client;
				ServerPack<T>& server_pack;
				std::vector<fpmas::api::graph::DistributedNode<T>*> nodes_to_remove;

			public:
				void registerNodeToRemove(fpmas::api::graph::DistributedNode<T>* node) override {
					nodes_to_remove.push_back(node);
				}

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
				 * If the specified edge is \DISTANT, the request is
				 * transmitted to the api::LinkClient component. Nothing needs
				 * to be done otherwise.
				 *
				 * If source and target nodes are both \DISTANT, the edge is planned
				 * to be deleted at the next synchronize() call.
				 *
				 * @param edge edge to link
				 */
				void link(EdgeApi* edge) override {
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
				 * If the specified edge is \DISTANT, the request is
				 * transmitted to the api::LinkClient component. Nothing needs
				 * to be done otherwise.
				 *
				 * @param edge edge to unlink
				 */
				void unlink(EdgeApi* edge) override {
					// Prevents other processes to unlink the edge while the local
					// process is unlinking it. In this case, incoming unlink requests
					// have no effect. (see LinkServer::handleIncomingRequests())
					server_pack.linkServer().lockUnlink(edge->getId());

					if(edge->state() == LocationState::DISTANT) {
						link_client.unlink(edge);
					}

					// Unlocks the unlink operation. The edge will actually be erased
					// from the graph upon return, in the DistributedGraph::unlink()
					// method.
					server_pack.linkServer().unlockUnlink(edge->getId());
				};

				/**
				 * Performs a remove node request.
				 *
				 * If the specified node is \DISTANT, the request is
				 * transmitted to the api::LinkClient component. Else, the
				 * local process own the node, so unlink operations of the
				 * connected edges are performed using
				 * fpmas::api::graph::DistributedGraph::unlink().
				 *
				 * The node is scheduled to be erased at the next
				 * HardDataSync::synchronize() call (see
				 * api::LinkClient::removeNode() for a detailed explanation).
				 *
				 * @param node node to remove
				 */
				void removeNode(NodeApi* node) override {
					if(node->state() == LocationState::DISTANT) {
						link_client.removeNode(node);
					} else {
						server_pack.linkServer().lockRemoveNode(node->getId());
						for(auto edge : node->getOutgoingEdges())
							graph.unlink(edge);
						for(auto edge : node->getIncomingEdges())
							graph.unlink(edge);
						server_pack.linkServer().unlockRemoveNode(node->getId());
					}
					registerNodeToRemove(node);
				}

				/**
				 * HardSyncLinker synchronization.
				 *
				 * First, edges linked between a \DISTANT source and a \DISTANT
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
					FPMAS_LOGI(graph.getMpiCommunicator().getRank(), "HARD_SYNC_LINKER", "Synchronizing sync linker...", "");
					for(auto edge : ghost_edges)
						graph.erase(edge);
					ghost_edges.clear();

					server_pack.terminate();

					for(auto node : nodes_to_remove) {
						graph.erase(node);
					}
					nodes_to_remove.clear();

					FPMAS_LOGI(graph.getMpiCommunicator().getRank(), "HARD_SYNC_LINKER", "Synchronized.", "");
				};
		};
}}}
#endif
