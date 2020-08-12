#ifndef FPMAS_SERVER_PACK_H
#define FPMAS_SERVER_PACK_H

#include "fpmas/api/communication/communication.h"
#include "./api/client_server.h"
#include "fpmas/utils/log.h"

namespace fpmas { namespace synchro { namespace hard {

	template<typename T>
		class ServerPack : public api::Server {
			typedef api::Epoch Epoch;
			typedef api::TerminationAlgorithm TerminationAlgorithm;

			private:
			fpmas::api::communication::MpiCommunicator& comm;
			TerminationAlgorithm& termination;
			api::MutexServer<T>& mutex_server;
			api::LinkServer& link_server;
			Epoch epoch;

			public:
			ServerPack(
					fpmas::api::communication::MpiCommunicator& comm,
					TerminationAlgorithm& termination,
					api::MutexServer<T>& mutex_server,
					api::LinkServer& link_server)
				: comm(comm), termination(termination), mutex_server(mutex_server), link_server(link_server) {
					setEpoch(Epoch::EVEN);
				}

			api::LinkServer& linkServer() {return link_server;}
			api::MutexServer<T>& mutexServer() {return mutex_server;}

			void handleIncomingRequests() override {
				mutex_server.handleIncomingRequests();
				link_server.handleIncomingRequests();
			}

			void setEpoch(Epoch epoch) override {
				this->epoch = epoch;
				mutex_server.setEpoch(epoch);
				link_server.setEpoch(epoch);
			}

			Epoch getEpoch() const override {
				return epoch;
			}

			void terminate() {
				termination.terminate(*this);
			}

			void waitSendRequest(MPI_Request* req) {
				FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "wait for send...");
				bool sent = comm.test(req);

				while(!sent) {
					mutex_server.handleIncomingRequests();
					link_server.handleIncomingRequests();
					sent = comm.test(req);
				}
				FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "Request sent.");
			}
		};
}}}
#endif
