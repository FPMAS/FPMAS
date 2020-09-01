#include "termination.h"

namespace fpmas { namespace synchro { namespace hard {

	void TerminationAlgorithm::toggleEpoch(api::Server& server) {
		switch(server.getEpoch()) {
			case Epoch::EVEN:
				server.setEpoch(Epoch::ODD);
				break;
			case Epoch::ODD:
				server.setEpoch(Epoch::EVEN);
		}
	}

	void TerminationAlgorithm::terminate(api::Server& server) {
		FPMAS_LOGD(comm.getRank(), "TERMINATION", "Entering termination algorithm... (epoch : %i)",
				server.getEpoch());
		Color token;

		if(this->comm.getRank() == 0) {
			this->color = Color::WHITE;
			token = Color::WHITE;
			this->color_mpi.send(token, this->comm.getSize() - 1, Tag::TOKEN);
		}

		int sup_rank = (this->comm.getRank() + 1) % this->comm.getSize();
		communication::Status status;
		while(true) {
			// Check for TOKEN
			if(this->color_mpi.Iprobe(sup_rank, Tag::TOKEN, status) > 0) {
				token = this->color_mpi.recv(status.source, status.tag);
				if(this->comm.getRank() == 0) {
					if(token == Color::WHITE && this->color == Color::WHITE) {
						for (int i = 1; i < this->comm.getSize(); ++i) {
							this->comm.send(i, Tag::END);
						}
						toggleEpoch(server);
						FPMAS_LOGD(comm.getRank(), "TERMINATION", "Termination complete.");
						return;
					} else {
						this->color = Color::WHITE;
						token = Color::WHITE;
						this->color_mpi.send(token, this->comm.getSize() - 1, Tag::TOKEN);
					}
				}
				else {
					if(this->color == Color::BLACK) {
						token = Color::BLACK;
					}
					this->color_mpi.send(token, this->comm.getRank() - 1, Tag::TOKEN);
					this->color = Color::WHITE;
				}
			}

			// Check for END
			if(this->comm.getRank() > 0
					&& this->comm.Iprobe(
						fpmas::api::communication::MpiCommunicator::IGNORE_TYPE,0, Tag::END, status
						) > 0) {
				this->comm.recv(status.source, status.tag);
				toggleEpoch(server);
				FPMAS_LOGD(comm.getRank(), "TERMINATION", "End message received, termination complete.");
				return;
			}

			server.handleIncomingRequests();
		}
	}
}}}
