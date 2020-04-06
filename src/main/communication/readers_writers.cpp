#include "readers_writers.h"
#include <iostream>
#include "utils/log.h"

namespace FPMAS::communication {

	void ReadersWriters::read(int destination) {
		if(!locked) {
			this->requestHandler.respondToRead(destination, this->id);
		} else {
			this->read_requests.push(destination);
		}
	}

	void ReadersWriters::write(int destination) {
		FPMAS_LOGV(this->requestHandler.getMpiComm().getRank(), "RW", "data %s : incoming write from proc %i (locked : %i)", ID_C_STR(this->id), destination, this->locked);
		if(!this->locked) {
			this->lock();
			FPMAS_LOGV(this->requestHandler.getMpiComm().getRank(), "RW", "data %s : respond to write from proc %i", ID_C_STR(this->id), destination);
			this->requestHandler.respondToAcquire(destination, this->id);
		} else {
			FPMAS_LOGV(this->requestHandler.getMpiComm().getRank(), "RW", "data %s : add write req from proc %i to waiting queue", ID_C_STR(this->id), destination);
			this->write_requests.push(destination);
		}
	}

	void ReadersWriters::release() {
		while(!this->read_requests.empty()) {
			FPMAS_LOGV(this->requestHandler.getMpiComm().getRank(), "RW", "data %s : respond to waiting read from proc %i", ID_C_STR(this->id), this->write_requests.front());
			this->requestHandler.respondToRead(this->read_requests.front(), this->id);
			this->read_requests.pop();
		}
		if(!this->write_requests.empty()) {
			FPMAS_LOGV(this->requestHandler.getMpiComm().getRank(), "RW", "data %s : respond to waiting write from proc %i", ID_C_STR(this->id), this->write_requests.front());
			this->requestHandler.respondToAcquire(this->write_requests.front(), this->id);
			this->write_requests.pop();
		}
		else {
			FPMAS_LOGV(this->requestHandler.getMpiComm().getRank(), "RW", "release data %s", ID_C_STR(this->id));
			this->locked = false;
		}
	}
	
	bool ReadersWriters::isLocked() const {
		return locked;
	}

	void ReadersWriters::lock() {
		this->locked = true;
	}

	void ResourceManager::read(DistributedId id, int destination) {
		if(this->readersWriters.count(id) == 0)
			this->readersWriters.insert(std::make_pair(
					id,
					ReadersWriters(id, this->comm)
					)
					);
		this->readersWriters.at(id).read(destination);
	}

	void ResourceManager::write(DistributedId id, int destination) {
		if(this->readersWriters.count(id) == 0) {
			this->readersWriters.emplace(std::make_pair(
						id,
						ReadersWriters(id, this->comm)
						));
		}
		this->readersWriters.at(id).write(destination);
	}

	void ResourceManager::release(DistributedId id) {
		this->readersWriters.at(id).release();
	}

	const ReadersWriters& ResourceManager::get(DistributedId id) const {
		if(this->readersWriters.count(id) == 0)
			this->readersWriters.emplace(std::make_pair(
						id,
						ReadersWriters(id, this->comm)
						));
		return this->readersWriters.at(id);
	}

	/*
	 * Manually locks the local resource.
	 * Used by the SyncMpiCommunicator to lock local resources without
	 * unwinding all the communication stack.
	 */
	void ResourceManager::lock(DistributedId id) {
		this->readersWriters.at(id).lock();
	}

	/*
	 * Clears the ResourceManager instance, implicitly re-initializing all the
	 * ReadersWriters instances.
	 */
	void ResourceManager::clear() {
		this->readersWriters.clear();
	}
}
