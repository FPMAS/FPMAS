#ifndef READERS_WRITERS_H
#define READERS_WRITERS_H

#include <set>
#include <vector>
#include <unordered_map>
#include <queue>

namespace FPMAS::communication {
	class SyncMpiCommunicator;

	class ReadersWriters {
		private:
			unsigned long id;
			std::queue<int> read_requests;
			std::queue<int> write_requests;
			SyncMpiCommunicator* comm;
			bool locked = false;

		public:
			ReadersWriters(unsigned long id, SyncMpiCommunicator* comm) : id(id), comm(comm) {}
			void read(int destination);
			void write(int destination);
			void release();
			bool isLocked() const;
	};

	class ResourceManager {
		friend SyncMpiCommunicator;
		private:
			SyncMpiCommunicator* comm;
			std::set<unsigned long> locallyAcquired;
			std::unordered_map<unsigned long, std::vector<int>> pendingReads;
			std::unordered_map<unsigned long, std::vector<int>> pendingAcquires;

			bool isLocallyAcquired(unsigned long) const;
			void addPendingRead(unsigned long id, int destination);
			void addPendingAcquire(unsigned long id, int destination);

			std::unordered_map<unsigned long, ReadersWriters> readersWriters;
			void clear();

		public:
			ResourceManager(SyncMpiCommunicator* comm) : comm(comm) {};
			void read(unsigned long, int);
			void write(unsigned long, int);
			void release(unsigned long);
			const ReadersWriters& get(unsigned long);


	};

}
#endif
