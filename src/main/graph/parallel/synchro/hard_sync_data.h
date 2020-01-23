#ifndef HARD_SYNC_DATA
#define HARD_SYNC_DATA

#include "ghost_data.h"
#include "communication/communication.h"

using FPMAS::communication::TerminableMpiCommunicator;

namespace FPMAS::graph::parallel::synchro {

	template<class T> class HardSyncData : public GhostData<T> {
		public:
			HardSyncData(TerminableMpiCommunicator&);
			HardSyncData(TerminableMpiCommunicator&, T);

	};

	template<class T> HardSyncData<T>::HardSyncData(TerminableMpiCommunicator& mpiComm) : GhostData<T>(mpiComm) {}

	template<class T> HardSyncData<T>::HardSyncData(TerminableMpiCommunicator& mpiComm, T data) : GhostData<T>(mpiComm, data) {}
}
#endif
