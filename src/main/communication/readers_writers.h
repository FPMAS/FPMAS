#ifndef READERS_WRITERS_H
#define READERS_WRITERS_H

#include <unordered_map>

namespace FPMAS::communication {


	class TerminableMpiCommunicator;

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
		private:
			std::unordered_map<unsigned long, ReadersWriters> readersWriters;

		public:
			void initRead(unsigned long);
			void releaseRead(unsigned long);
			void initWrite(unsigned long);
			void releaseWrite(unsigned long);
			const ReadersWriters& get(unsigned long) const;

	};

}
#endif
