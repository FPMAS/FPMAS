#ifndef FPMAS_SYNCHRO_GUARDS
#define FPMAS_SYNCHRO_GUARDS

#include "fpmas/api/graph/distributed_node.h"

namespace fpmas {
	namespace synchro {
		template<typename T>
			class Guard {
				protected:
				api::synchro::Mutex<T>& mutex;

				Guard(api::graph::DistributedNode<T>* node)
					: mutex(*node->mutex()) {
					}

				public:
				Guard(const Guard&) = delete;
				Guard(Guard&&) = delete;
				Guard& operator=(const Guard&) = delete;
				Guard& operator=(Guard&&) = delete;

				virtual ~Guard() {}
			};

		template<typename T>
		class ReadGuard : Guard<T> {
			public:
			ReadGuard(api::graph::DistributedNode<T>* node)
				: Guard<T>(node) {
					// updates node.data() according to the SyncMode
					this->mutex.read();
				}

			virtual ~ReadGuard() {
				this->mutex.releaseRead();
			}
		};

		template<typename T>
		class AcquireGuard : Guard<T> {
			public:
			AcquireGuard(api::graph::DistributedNode<T>* node)
				: Guard<T>(node) {
					// updates node.data() according to the SyncMode
					this->mutex.acquire();
				}

			virtual ~AcquireGuard() {
				this->mutex.releaseAcquire();
			}
		};

	}
}
#endif
