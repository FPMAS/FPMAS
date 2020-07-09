#ifndef SYNC_MODE_API_H
#define SYNC_MODE_API_H

#include <string>
#include "fpmas/api/graph/distributed_graph.h"

namespace fpmas { namespace api { namespace synchro {
	class DataSync {
		public:
			virtual void synchronize() = 0;
	};

	template<typename T>
	class SyncLinker {
		public:
			virtual void link(const api::graph::DistributedEdge<T>*) = 0;
			virtual void unlink(const api::graph::DistributedEdge<T>*) = 0;

			virtual void synchronize() = 0;
	};

	template<typename T>
		class SyncModeRuntime {
			public:
				virtual Mutex<T>* buildMutex(api::graph::DistributedNode<T>*) = 0;
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
}}}
#endif
