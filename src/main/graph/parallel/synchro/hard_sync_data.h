#ifndef HARD_SYNC_DATA
#define HARD_SYNC_DATA

#include "ghost_data.h"
#include "communication/communication.h"

using FPMAS::communication::TerminableMpiCommunicator;

namespace FPMAS::graph::parallel::synchro {

	template<class T> class HardSyncData : public SyncData<T> {
		protected:
			unsigned long id;
			TerminableMpiCommunicator& mpiComm;
			const Proxy& proxy;

		public:
			HardSyncData(unsigned long, TerminableMpiCommunicator&, const Proxy&);
			HardSyncData(unsigned long, TerminableMpiCommunicator&, const Proxy&, T);

			const T& read() override;

			const static zoltan::utils::zoltan_query_functions config;

			static void termination(DistributedGraph<T, HardSyncData>* dg) {}
	};
	template<class T> const zoltan::utils::zoltan_query_functions HardSyncData<T>::config
		(
		 &FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_olz<T, HardSyncData>,
		 &FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_olz<T, HardSyncData>,
		 &FPMAS::graph::parallel::zoltan::arc::mid_migrate_pp_fn<T, HardSyncData>
		);

	template<class T> HardSyncData<T>::HardSyncData(
			unsigned long id,
			TerminableMpiCommunicator& mpiComm,
			const Proxy& proxy)
		: id(id), mpiComm(mpiComm), proxy(proxy) {
		}

	template<class T> HardSyncData<T>::HardSyncData(
			unsigned long id,
			TerminableMpiCommunicator& mpiComm,
			const Proxy& proxy, T data)
		: SyncData<T>(data), id(id), mpiComm(mpiComm), proxy(proxy) {
		}

	template<class T> const T& HardSyncData<T>::read() {
		this->data = ((nlohmann::json) nlohmann::json::parse(
				this->mpiComm.read(
					this->id,
					this->proxy.getCurrentLocation(this->id)
					)
				)).get<T>();
		return this->data;
	}
}
#endif
