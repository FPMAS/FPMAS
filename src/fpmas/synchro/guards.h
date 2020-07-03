#ifndef FPMAS_SYNCHRO_GUARDS
#define FPMAS_SYNCHRO_GUARDS

#include "fpmas/api/graph/parallel/distributed_node.h"

namespace fpmas {
	namespace synchro {
		template<typename T>
			class Guard {
				protected:
				fpmas::api::synchro::Mutex<T>& mutex;

				Guard(fpmas::api::graph::parallel::DistributedNode<T>* node)
					: mutex(node->mutex()) {
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
			ReadGuard(fpmas::api::graph::parallel::DistributedNode<T>* node)
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
			AcquireGuard(fpmas::api::graph::parallel::DistributedNode<T>* node)
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
