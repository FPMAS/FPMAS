#ifndef SYNC_MODE_API_H
#define SYNC_MODE_API_H

#include <string>
#include "api/communication/communication.h"
#include "api/graph/parallel/distributed_graph.h"

namespace FPMAS::api::graph::parallel::synchro {
	template <typename NodeType, typename ArcType>
		class DataSync {
			public:
				virtual void synchronize(
						FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>&
						) = 0;
		};

	template<typename NodeType, typename ArcType>
		class SyncLinker {
			public:
				virtual void link(const ArcType*) = 0;
				virtual void unlink(const ArcType*) = 0;

				virtual void synchronize(
						FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>&
						) = 0;
		};

	template<
		template<typename> class Mutex,
		template<typename, typename> class SyncLinker,
		template<typename, typename> class DataSync>
		class SyncMode {
			public:
				template<typename T>
					using mutex_type = Mutex<T>;
				template<typename NodeType, typename ArcType>
					using sync_linker = SyncLinker<NodeType, ArcType>;
				template<typename NodeType, typename ArcType>
					using data_sync = DataSync<NodeType, ArcType>;
				/*
				 *
				 *            virtual void synchronize(
				 *                    FPMAS::api::communication::MpiCommunicator&,
				 *                    FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>&
				 *                    ) = 0;
				 */
		};
}
#endif
