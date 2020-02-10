#include "readers_writers.h"
#include <iostream>
#include "sync_communication.h"
#include "utils/log.h"

namespace FPMAS::communication {

	void ReadersWriters::read(int destination) {
		if(!locked) {
			this->comm->respondToRead(destination, this->id);
		} else {
			this->read_requests.push(destination);
		}
	}

	void ReadersWriters::write(int destination) {
		FPMAS_LOGV(this->comm->getRank(), "RW", "data %lu : incoming write from proc %i (locked : %i)", this->id, destination, this->locked);
		if(!this->locked) {
			this->locked = true;
			FPMAS_LOGV(this->comm->getRank(), "RW", "data %lu : respond to write from proc %i", this->id, destination);
			this->comm->respondToAcquire(destination, this->id);
		} else {
			FPMAS_LOGV(this->comm->getRank(), "RW", "data %lu : add write req from proc %i to waiting queue", this->id, destination);
			this->write_requests.push(destination);
		}
	}

	void ReadersWriters::release() {
		while(!this->read_requests.empty()) {
			FPMAS_LOGV(this->comm->getRank(), "RW", "data %lu : respond to waiting read from proc %i", this->id, this->write_requests.front());
			this->comm->respondToRead(this->read_requests.front(), this->id);
			this->read_requests.pop();
		}
		if(!this->write_requests.empty()) {
			FPMAS_LOGV(this->comm->getRank(), "RW", "data %lu : respond to waitin write from proc %i", this->id, this->write_requests.front());
			this->comm->respondToAcquire(this->write_requests.front(), this->id);
			this->write_requests.pop();
		} else {
			FPMAS_LOGV(this->comm->getRank(), "RW", "release data %lu", this->id);
			this->locked = false;
		}
	}
	
	bool ReadersWriters::isLocked() const {
		return locked;
	}

	bool ResourceManager::isLocallyAcquired(unsigned long id) const {
		return this->locallyAcquired.count(id) > 0;
	}

	void ResourceManager::addPendingRead(unsigned long id, int destination) {
		this->pendingReads[id].push_back(destination);
	}

	void ResourceManager::addPendingAcquire(unsigned long id, int destination) {
		this->pendingAcquires[id].push_back(destination);
	}

	void ResourceManager::read(unsigned long id, int destination) {
		if(this->readersWriters.count(id) == 0)
			this->readersWriters.insert(std::make_pair(
					id,
					ReadersWriters(id, this->comm)
					)
					);
		this->readersWriters.at(id).read(destination);
	}

	void ResourceManager::write(unsigned long id, int destination) {
		if(this->readersWriters.count(id) == 0) {
			this->readersWriters.emplace(std::make_pair(
						id,
						ReadersWriters(id, this->comm)
						));
		}
		this->readersWriters.at(id).write(destination);
	}

	void ResourceManager::release(unsigned long id) {
		this->readersWriters.at(id).release();
	}

	const ReadersWriters& ResourceManager::get(unsigned long id) {
		if(this->readersWriters.count(id) == 0)
			this->readersWriters.emplace(std::make_pair(
						id,
						ReadersWriters(id, this->comm)
						));
		return this->readersWriters.at(id);
	}

	void ResourceManager::clear() {
		this->readersWriters.clear();
	}
}
