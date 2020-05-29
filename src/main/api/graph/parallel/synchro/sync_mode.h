#ifndef SYNC_MODE_API_H
#define SYNC_MODE_API_H

#include <string>
#include "api/graph/parallel/distributed_graph.h"

namespace FPMAS::api::graph::parallel::synchro {
/*
 *    template <typename NodeType, typename ArcType>
 *        class DataSync {
 *            public:
 *                virtual void synchronize(
 *                        FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>&
 *                        ) = 0;
 *        };
 *
 *    template<typename NodeType, typename ArcType>
 *        class SyncLinker {
 *            public:
 *                virtual void link(const ArcType*) = 0;
 *                virtual void unlink(const ArcType*) = 0;
 *
 *                virtual void synchronize(
 *                        FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>&
 *                        ) = 0;
 *        };
 */
	class DataSync {
		public:
			virtual void synchronize() = 0;
	};

	template<typename ArcType>
		class SyncLinker {
			public:
				virtual void link(const ArcType*) = 0;
				virtual void unlink(const ArcType*) = 0;

				virtual void synchronize() = 0;
		};

	template<typename Node, typename Arc, typename Mutex>
		class SyncModeRuntime {
			public:
				virtual void setUp(DistributedId id, Mutex&) = 0;
				virtual SyncLinker<Arc>& getSyncLinker() = 0;
				virtual DataSync& getDataSync() = 0;
		};

	template<
		template<typename> class Mutex,
		template<typename, typename, typename> class Runtime
		>
		class SyncMode {
			public:
				template<typename T>
					using MutexType = Mutex<T>;
				template<typename T, typename Node, typename Arc>
					using SyncModeRuntimeType = Runtime<Node, Arc, Mutex<T>>;

				
		};
}
#endif
