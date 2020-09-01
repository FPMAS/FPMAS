#include "communication.h"

namespace fpmas { namespace api { namespace communication {
	Status Status::IGNORE {};
	MPI_Datatype MpiCommunicator::IGNORE_TYPE = MPI_INT;
}}}
