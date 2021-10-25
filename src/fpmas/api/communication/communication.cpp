#include "communication.h"

namespace fpmas { namespace api { namespace communication {
	Status Status::IGNORE {};
	MPI_Datatype MpiCommunicator::IGNORE_TYPE = MPI_INT;

	bool operator==(const DataPack& d1, const DataPack& d2) {
		if(d1.count != d2.count)
			return false;
		if(d1.size != d2.size)
			return false;
		for(int i = 0; i < d1.size; i++) {
			if(d1.buffer[i] != d2.buffer[i])
				return false;
		}
		return true;
	}
}}}
