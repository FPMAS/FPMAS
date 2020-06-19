#ifndef SYNC_MODE_API_H
#define SYNC_MODE_API_H

#include <string>
#include "api/graph/parallel/distributed_graph.h"

namespace FPMAS::api::synchro {
	class DataSync {
		public:
			virtual void synchronize() = 0;
	};

	template<typename T>
	class SyncLinker {
		public:
			virtual void link(const FPMAS::api::graph::parallel::DistributedArc<T>*) = 0;
			virtual void unlink(const FPMAS::api::graph::parallel::DistributedArc<T>*) = 0;

			virtual void synchronize() = 0;
	};

	template<typename T>
		class SyncModeRuntime {
			public:
				virtual Mutex<T>* buildMutex(api::graph::parallel::DistributedNode<T>*) = 0;
				virtual SyncLinker<T>& getSyncLinker() = 0;
				virtual DataSync& getDataSync() = 0;
		};

	template<
		template<typename> class Mutex,
		template<typename> class Runtime
		>
		class SyncMode {
			public:
				template<typename T>
					using MutexType = Mutex<T>;
				template<typename T>
					using SyncModeRuntimeType = Runtime<T>;

				
		};
}
#endif
