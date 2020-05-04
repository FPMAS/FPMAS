#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "api/communication/request_handler.h"
#include "api/communication/resource_handler.h"

#include "communication.h"
#include "readers_writers.h"

namespace FPMAS::communication {

	using api::communication::Epoch;
	using api::communication::Tag;
	using api::communication::Color;
	using api::communication::ResourceHandler;

	/**
	 * An MpiCommunicator extension that implements the
	 * Dijkstra-Feijen-Gasteren termination algorithm.
	 *
	 * The user can invoke the terminate() function to synchronize the
	 * communicators.
	 *
	 */
	template<
		typename ReadersWritersManagerType
		= communication::ReadersWritersManager<communication::FirstReadersWriters>
		>
	class RequestHandler
		: public api::communication::RequestHandler {
			typedef api::communication::MpiCommunicator MpiComm;
		
			private:
			MpiComm& comm;
			MpiDistributedId distributedIdBuffer;

			Epoch epoch = Epoch::EVEN;
			Color color = Color::WHITE;

			ResourceHandler& resourceHandler;
			ReadersWritersManagerType readersWritersManager;

			void waitSendRequest(MPI_Request*);

			void handleRead(int, DistributedId);
			void respondToRead(int, DistributedId) override;
			void handleAcquire(int, DistributedId);
			void respondToAcquire(int, DistributedId) override;
			void handleGiveBack(std::string);

			const MpiComm& getMpiComm();

			void waitForReading(DistributedId);
			void waitForAcquire(DistributedId);


			public:
			RequestHandler(MpiComm&, ResourceHandler&);

			void handleIncomingRequests();

			std::string read(DistributedId, int) override;

			std::string acquire(DistributedId, int) override;
			void giveBack(DistributedId, int) override;

			void terminate() override;

			const ReadersWritersManagerType& getReadersWritersManager() const {
				return readersWritersManager;
			}
		};
	/**
	 * Builds a SyncMpiCommunicator containing all the procs available in
	 * MPI_COMM_WORLD.
	 *
	 * @param resourceHandler pointer to the ResourceContainer instance that will be
	 * used to query serialized data.
	 */
	template<typename ReadersWritersManagerType>
	RequestHandler<ReadersWritersManagerType>::RequestHandler(MpiComm& comm, ResourceHandler& resourceHandler)
		: readersWritersManager(*this, comm.getRank()), comm(comm), resourceHandler(resourceHandler) {
		};

	/**
	 * Performs a reception cycle to handle.
	 *
	 * This function should not be used by the user, but is left public because it
	 * might be useful for unit testing.
	 *
	 * A call to this function will receive and handle at most one of each of the
	 * following requests types :
	 * - read request
	 * - acquire request
	 * - given back data
	 */
	template<typename ReadersWritersManagerType>
	void RequestHandler<ReadersWritersManagerType>::handleIncomingRequests() {
		MPI_Status req_status;

		// Check read request
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::READ, &req_status) > 0) {
			DistributedId id;
			comm.recv(&req_status, id);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "read request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleRead(req_status.MPI_SOURCE, id);
		}

		// Check acquire request
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::ACQUIRE, &req_status) > 0) {
			DistributedId id;
			comm.recv(&req_status, id);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "acquire request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleAcquire(req_status.MPI_SOURCE, id);
		}

		// Check acquire give back
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::ACQUIRE_GIVE_BACK, &req_status) > 0) {
			std::string data;
			comm.recv(&req_status, data);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "given back from %i : %s", req_status.MPI_SOURCE, data.c_str());
			this->handleGiveBack(data);
		}
	}

	/*
	 * Allows to respond to other request while a request (sent in a synchronous
	 * message) is sending, in order to avoid deadlock.
	 */
	template<typename ReadersWritersManagerType>
	void RequestHandler<ReadersWritersManagerType>::waitSendRequest(MPI_Request* req) {
		FPMAS_LOGD(this->comm.getRank(), "WAIT", "wait for send");
		MPI_Status send_status;
		int request_sent;
		MPI_Test(req, &request_sent, &send_status);

		while(request_sent == 0) {
			this->handleIncomingRequests();

			MPI_Test(req, &request_sent, &send_status);
		}
	}

	/**
	 * Performs a synchronized read operation of the data corresponding to id on
	 * the proc at the provided location, and returns the read serialized data.
	 *
	 * @param id id of the data to read
	 * @param location rank of the data location
	 * @return read serialized data
	 */
	template<typename ReadersWritersManagerType>
	std::string RequestHandler<ReadersWritersManagerType>::read(DistributedId id, int location) {
		if(location == this->comm.getRank()) {
			FPMAS_LOGD(location, "READ", "reading local data %s", ID_C_STR(id));
			// The local process needs to wait as any other proc to access its own
			// resources.
			//const ReadersWriters& rw = this->resourceManager.get(id);
			const auto& rw = this->readersWritersManager[id];
			int i = 0;
			while(rw.isLocked()) {
				// Responds to request until data is given back
				this->handleIncomingRequests();
			}

			std::string data = this->resourceHandler.getLocalData(id);

			return data;
		}
		FPMAS_LOGD(this->comm.getRank(), "READ", "reading data %s from %i", ID_C_STR(id), location);
		// Starts non-blocking synchronous send
		MPI_Request req;
		this->comm.Issend(id, location, epoch | Tag::READ, &req);

		// Keep responding to other READ / ACQUIRE request to avoid deadlock,
		// until the request has been received
		this->waitSendRequest(&req);

		// The request has been received : it is assumed that the receiving proc is
		// now responding so we can safely wait for response without deadlocking
		MPI_Status read_response_status;
		this->comm.probe(location, epoch | Tag::READ_RESPONSE, &read_response_status);

		std::string response;
		this->comm.recv(&read_response_status, response);
		return response;
	}

	/*
	 * Handles a read request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond immediately if the resource is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename ReadersWritersManagerType>
	void RequestHandler<ReadersWritersManagerType>::handleRead(int destination, DistributedId id) {
		this->color = Color::BLACK;

		// Transmit request to the resource manager
		//this->resourceManager.read(id, destination);
		this->readersWritersManager[id].read(destination);
	}

	/*
	 * Sends a read response to the destination proc, reading data using the
	 * resourceManager.
	 */
	template<typename ReadersWritersManagerType>
	void RequestHandler<ReadersWritersManagerType>::respondToRead(int destination, DistributedId id) {
		// Perform the response
		std::string data = this->resourceHandler.getLocalData(id);
		MPI_Request req;
		// TODO : asynchronous send : is this a pb? => probably not.
		this->comm.Isend(data, destination, epoch | Tag::READ_RESPONSE, &req);

		FPMAS_LOGD(this->comm.getRank(), "READ", "send read data %s to %i : %s", ID_C_STR(id), destination, data.c_str());
	}

	/**
	 * Performs a synchronized acquire operation of the data corresponding to id on
	 * the proc at the provided location, and returns the acquired serialized data.
	 *
	 * @param id id of the data to read
	 * @param location rank of the data location
	 * @return acquired serialized data
	 */
	template<typename ReadersWritersManagerType>
	std::string RequestHandler<ReadersWritersManagerType>::acquire(DistributedId id, int location) {
		if(location == this->comm.getRank()) {
			// TODO : This management is bad, because the local proc is not added
			// to the waiting queue as any other incoming request.
			// It would be better to :
			// - add the local request to the queue as any other
			// - wait() responding to requests, at the resource manager scale
			// - handle the fact that data is local is the respond functions
			FPMAS_LOGD(location, "ACQUIRE", "acquiring local data %s", ID_C_STR(id));
			// The local process needs to wait as any other proc to access its own
			// resources.
			//const ReadersWriters& rw = this->resourceManager.get(id);
			const auto& rw = this->readersWritersManager[id];
			while(rw.isLocked())
				this->handleIncomingRequests();

			//this->resourceManager.lock(id);
			this->readersWritersManager[id].lock();
			//this->resourceManager.locallyAcquired.insert(id);
			return this->resourceHandler.getLocalData(id);
		}
		FPMAS_LOGD(this->comm.getRank(), "ACQUIRE", "acquiring data %s from %i", ID_C_STR(id), location);
		// Starts non-blocking synchronous send
		MPI_Request req;
		this->comm.Issend(id, location, epoch | Tag::ACQUIRE, &req);

		this->waitSendRequest(&req);

		// The request has been received : it is assumed that the receiving proc is
		// now responding so we can safely wait for response without deadlocking
		MPI_Status acquire_response_status;
		this->comm.probe(location, epoch | Tag::ACQUIRE_RESPONSE, &acquire_response_status);

		std::string response;
		this->comm.recv(&acquire_response_status, response);
		FPMAS_LOGD(this->comm.getRank(), "ACQUIRE", "acquired data %s from %i : %s", ID_C_STR(id), location, response.c_str());

		return response;
	}

	/*
	 * Handles an acquire request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond if the resource immediately is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename ReadersWritersManagerType>
	void RequestHandler<ReadersWritersManagerType>::handleAcquire(int destination, DistributedId id) {
		this->color = Color::BLACK;

		//this->resourceManager.write(id, destination);
		this->readersWritersManager[id].acquire(destination);
	}

	/*
	 * Sends an acquire response to the destination proc, reading data using the
	 * resourceManager.
	 */
	template<typename ReadersWritersManagerType>
	void RequestHandler<ReadersWritersManagerType>::respondToAcquire(int destination, DistributedId id) {
		std::string data = this->resourceHandler.getLocalData(id);
		FPMAS_LOGD(this->comm.getRank(), "ACQUIRE", "send acquired data %s to %i : %s", ID_C_STR(id), destination, data.c_str());

		MPI_Request req;
		// TODO : asynchronous send : is this a pb? => probably not.
		this->comm.Isend(data, destination, epoch | Tag::ACQUIRE_RESPONSE, &req);
	}

	/**
	 * Gives back a previously acquired data to location proc, sending potential
	 * updates that occurred while data was locally acquired.
	 *
	 * @param id id of the data to read
	 * @param location rank of the data location
	 */
	template<typename ReadersWritersManagerType>
	void RequestHandler<ReadersWritersManagerType>::giveBack(DistributedId id, int location) {
		if(location == this->comm.getRank()) {
			// No update needed, because modifications are already local.
			this->readersWritersManager[id].release();
			FPMAS_LOGD(this->comm.getRank(), "ACQUIRE", "locally given back %s", ID_C_STR(id));
			return;
		}

		// Perform the response
		nlohmann::json update = {
			{"id", id},
			{"data", this->resourceHandler.getUpdatedData(id)}
		};
		std::string data = update.dump();
		FPMAS_LOGD(this->comm.getRank(), "ACQUIRE", "giving back to %i : %s", location, data.c_str());
		MPI_Request req;
		// TODO : asynchronous send : is this a pb? => maybe.
		this->comm.Issend(data, location, epoch | Tag::ACQUIRE_GIVE_BACK, &req);

		this->waitSendRequest(&req);
	}

	/*
	 * Handles given back data, updating the local resource and releasing it.
	 */
	template<typename ReadersWritersManagerType>
	void RequestHandler<ReadersWritersManagerType>::handleGiveBack(std::string data) {
		nlohmann::json update_json = nlohmann::json::parse(data);
		DistributedId id = 
			update_json.at("id").get<DistributedId>();

		// Updates local data
		this->resourceHandler.updateData(
				id,
				update_json.at("data").get<std::string>()
				);

		// Releases the resource, and handles waiting requests
		//this->resourceManager.release(id);
		this->readersWritersManager[id].release();
	}

	template<typename ReadersWritersManagerType>
	const api::communication::MpiCommunicator& 
		RequestHandler<ReadersWritersManagerType>::getMpiComm() {
		return this->comm;
	}


	/**
	 * Implements the Dijkstra-Feijen-Gasteren termination algorithm.
	 *
	 * This function must be called when the process using this communicator is
	 * done with read and acquire operations to other procs. When terminate() is
	 * called, it will enter a PASSIVE mode where it is only able to respond to
	 * other procs READ and ACQUIRE requests (ie ACTIVE processes that have not
	 * terminated yet).
	 *
	 * Returns only when all the processes have terminated their process.
	 */
	template<typename ReadersWritersManagerType>
	void RequestHandler<ReadersWritersManagerType>::terminate() {
		Color token;

		if(this->comm.getRank() == 0) {
			this->color = Color::WHITE;
			token = Color::WHITE;
			this->comm.send(token, this->comm.getSize() - 1);
		}

		int sup_rank = (this->comm.getRank() + 1) % this->comm.getSize();
		MPI_Status status;
		while(true) {
			// Check for TOKEN
			if(this->comm.Iprobe(sup_rank, Tag::TOKEN, &status) > 0) {
				this->comm.recv(&status, token);
				if(this->comm.getRank() == 0) {
					if(token == Color::WHITE && this->color == Color::WHITE) {
						for (int i = 1; i < this->comm.getSize(); ++i) {
							this->comm.sendEnd(i);
						}
						switch(this->epoch) {
							case Epoch::EVEN:
								this->epoch = Epoch::ODD;
								break;
							default:
								this->epoch = Epoch::EVEN;
						}
						this->readersWritersManager.clear();
						return;
					} else {
						this->color = Color::WHITE;
						token = Color::WHITE;
						this->comm.send(token, this->comm.getSize() - 1);
					}
				}
				else {
					if(this->color == Color::BLACK) {
						token = Color::BLACK;
					}
					this->comm.send(token, this->comm.getRank() - 1);
					this->color = Color::WHITE;
				}
			}

			// Check for END
			if(this->comm.getRank() > 0 && this->comm.Iprobe(MPI_ANY_SOURCE, Tag::END, &status) > 0) {
				int end;
				this->comm.recv(&status);
				switch(this->epoch) {
					case Epoch::EVEN:
						this->epoch = Epoch::ODD;
						break;
					default:
						this->epoch = Epoch::EVEN;
				}
				this->readersWritersManager.clear();
				return;
			}

			// Handles READ or ACQUIRE requests
			this->handleIncomingRequests();
		}
	}

}
#endif
