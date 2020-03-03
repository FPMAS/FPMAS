#ifndef HARD_SYNC_DATA
#define HARD_SYNC_DATA

#include "ghost_data.h"
#include "communication/communication.h"

using FPMAS::communication::SyncMpiCommunicator;

namespace FPMAS::graph::parallel::synchro {

	/**
	 * Hard synchronization mode, that can be used thanks to the second
	 * template parameter of the DistributedGraph<T, S> class.
	 *
	 * In this mode, the graph structure is preserved accross procs as a
	 * "ghost" graph, like in the GhostData mode.
	 *
	 * However, "ghost" data in not fetched automatically, but directly queried
	 * to the proc where the wrapped data is currently located, each time
	 * read() or acquire() is called. A Readers-Writers algorithm manages data
	 * access to avoid concurrency issues.
	 */
	template<class T> class HardSyncData : public SyncData<T> {
		private:
			unsigned long id;
			SyncMpiCommunicator& mpiComm;
			const Proxy& proxy;

		public:
			HardSyncData(unsigned long, SyncMpiCommunicator&, const Proxy&);
			HardSyncData(unsigned long, SyncMpiCommunicator&, const Proxy&, const T&);
			HardSyncData(unsigned long, SyncMpiCommunicator&, const Proxy&, T&&);

			const T& read() override;
			T& acquire() override;
			void release() override;

			/**
			 * Defines the Zoltan configuration used manage and migrate
			 * GhostNode s and GhostArc s.
			 */
			template<int N> const static zoltan::utils::zoltan_query_functions config;

			/**
			 * Termination function used at the end of each
			 * DistributedGraph<T,S>::synchronize() call : does not do anything
			 * in this mode.
			 */
			template<int N> static void termination(DistributedGraph<T, HardSyncData, N>* dg) {}
	};
	template<class T> template<int N> const zoltan::utils::zoltan_query_functions HardSyncData<T>::config
		(
		 &FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_olz<T, N, HardSyncData>,
		 &FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_olz<T, N, HardSyncData>,
		 &FPMAS::graph::parallel::zoltan::arc::mid_migrate_pp_fn<T, N, HardSyncData>
		);

	/**
	 * HardSyncData constructor.
	 *
	 * @param id wrapped data id
	 * @param mpiComm MPI communicator used to communicate with data sources
	 * @param proxy proxy used to locate data
	 */
	template<class T> HardSyncData<T>::HardSyncData(
			unsigned long id,
			SyncMpiCommunicator& mpiComm,
			const Proxy& proxy)
		: id(id), mpiComm(mpiComm), proxy(proxy) {
		}

	/**
	 * HardSyncData constructor.
	 *
	 * @param id wrapped data id
	 * @param mpiComm MPI communicator used to communicate with data sources
	 * @param proxy proxy used to locate data
	 * @param data associated data instance
	 */
	template<class T> HardSyncData<T>::HardSyncData(
			unsigned long id,
			SyncMpiCommunicator& mpiComm,
			const Proxy& proxy,
			T&& data)
		: SyncData<T>(std::move(data)), id(id), mpiComm(mpiComm), proxy(proxy) {
		}

	template<class T> HardSyncData<T>::HardSyncData(
			unsigned long id,
			SyncMpiCommunicator& mpiComm,
			const Proxy& proxy,
			const T& data)
		: SyncData<T>(data), id(id), mpiComm(mpiComm), proxy(proxy) {
		}

	/**
	 * Performs a `read` operation on the proc where the wrapped data is
	 * currently located.
	 *
	 * Data is returned as a const reference so that it is not allowed to write
	 * on data returned by the `read` operation.
	 *
	 * @return const reference to the distant data
	 */
	template<class T> const T& HardSyncData<T>::read() {
		this->data = ((nlohmann::json) nlohmann::json::parse(
				this->mpiComm.read(
					this->id,
					this->proxy.getCurrentLocation(this->id)
					)
				)).get<T>();
		return this->data;
	}

	// TODO: This needs to be optimize!! no need to serialize when data is
	// local...
	template<class T> T& HardSyncData<T>::acquire() {
		this->data = ((nlohmann::json) nlohmann::json::parse(
				this->mpiComm.acquire(
					this->id,
					this->proxy.getCurrentLocation(this->id)
					)
				)).get<T>();
		return this->data;
	}

	template<class T> void HardSyncData<T>::release() {
		this->mpiComm.giveBack(
				this->id,
				this->proxy.getCurrentLocation(this->id)
				);
	}
}
#endif
