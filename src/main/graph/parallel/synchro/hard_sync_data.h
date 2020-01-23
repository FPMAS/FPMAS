#ifndef HARD_SYNC_DATA
#define HARD_SYNC_DATA

#include "ghost_data.h"
#include "communication/communication.h"

using FPMAS::communication::TerminableMpiCommunicator;

namespace FPMAS::graph::parallel::synchro {

	template<class T> class HardSyncData : public SyncData<T> {
		private:
			TerminableMpiCommunicator& mpiComm;

		public:
			HardSyncData(TerminableMpiCommunicator&);
			HardSyncData(TerminableMpiCommunicator&, T);

	};

	template<class T> HardSyncData<T>::HardSyncData(TerminableMpiCommunicator& mpiComm) : mpiComm(mpiComm) {}

	template<class T> HardSyncData<T>::HardSyncData(TerminableMpiCommunicator& mpiComm, T data) : SyncData<T>(data), mpiComm(mpiComm) {}
}
#endif
