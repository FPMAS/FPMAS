#include "readers_writers.h"

namespace FPMAS::communication {

	void ReadersWriters::initRead() {
		this->readCount++;
		if(readCount == 1)
			this->reading = true;
	}

	void ReadersWriters::releaseRead() {
		this->readCount --;
		if(readCount == 0)
			this->reading = false;
	}

	void ReadersWriters::initWrite() {
		this->writing = true;
	}

	void ReadersWriters::releaseWrite() {
		this->writing = false;
	}

	bool ReadersWriters::isAvailableForReading() const {
		return reading || !writing;
	}

	bool ReadersWriters::isAvailableForWriting() const {
		return !reading && !writing;
	}

	bool ReadersWriters::isFree() const {
		return !reading && !writing;
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

	void ResourceManager::initRead(unsigned long id) {
		this->readersWriters[id].initRead();
	}

	void ResourceManager::releaseRead(unsigned long id) {
		ReadersWriters& rw = this->readersWriters[id];
		rw.releaseRead();
		if(rw.isFree())
			this->readersWriters.erase(id);
	}

	void ResourceManager::initWrite(unsigned long id) {
		this->readersWriters[id].initWrite();
	}

	void ResourceManager::releaseWrite(unsigned long id) {
		ReadersWriters& rw = this->readersWriters[id];
		rw.releaseWrite();
		if(rw.isFree())
			this->readersWriters.erase(id);
	}

	const ReadersWriters& ResourceManager::get(unsigned long id) {
		return this->readersWriters[id];
	}
}
