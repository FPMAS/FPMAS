#ifndef READERS_WRITERS_H
#define READERS_WRITERS_H

#include <set>
#include <vector>
#include <unordered_map>

namespace FPMAS::communication {


	class SyncMpiCommunicator;

	class ReadersWriters {
		private:
			int readCount = 0;
			bool reading = false;
			bool writing = false;

		public:
			void initRead();
			void initWrite();
			void releaseRead();
			void releaseWrite();

			bool isAvailableForReading() const;
			bool isAvailableForWriting() const;
			bool isFree() const;

	};

	class ResourceManager {
		friend SyncMpiCommunicator;
		private:
			std::set<unsigned long> locallyAcquired;
			std::unordered_map<unsigned long, std::vector<int>> pendingReads;
			std::unordered_map<unsigned long, std::vector<int>> pendingAcquires;

			bool isLocallyAcquired(unsigned long) const;
			void addPendingRead(unsigned long id, int destination);
			void addPendingAcquire(unsigned long id, int destination);

			std::unordered_map<unsigned long, ReadersWriters> readersWriters;

		public:
			void initRead(unsigned long);
			void releaseRead(unsigned long);
			void initWrite(unsigned long);
			void releaseWrite(unsigned long);
			const ReadersWriters& get(unsigned long);

	};

}
#endif
