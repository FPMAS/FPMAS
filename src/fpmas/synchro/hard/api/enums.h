#ifndef FPMAS_HARD_SYNC_ENUMS_H
#define FPMAS_HARD_SYNC_ENUMS_H

/** \file src/fpmas/synchro/hard/api/enums.h
 * HardSyncMode related enums.
 */

namespace fpmas { namespace synchro {
	namespace hard { namespace api {
		/**
		 * Constants used to build messages tags.
		 */
		enum Epoch : int {
			EVEN = 0x00,
			ODD = 0x10
		};

		/**
		 * Constants used to describe fpmas::synchro::hard::HardSyncMode message types.
		 */
		enum Tag : int {
			READ = 0x00,
			READ_RESPONSE = 0x01,
			ACQUIRE = 0x02,
			ACQUIRE_RESPONSE = 0x03,
			RELEASE_ACQUIRE = 0x04,
			LOCK = 0x05,
			LOCK_RESPONSE = 0x06,
			UNLOCK = 0x07,
			LOCK_SHARED = 0x08,
			LOCK_SHARED_RESPONSE = 0x09,
			UNLOCK_SHARED = 0x0A,
			LINK = 0x0B,
			UNLINK = 0x0C,
			REMOVE_NODE = 0x0D,
			TOKEN = 0x0E,
			END = 0x0F
		};

		/**
		 * Process colors used by the termination algorithm.
		 */
		enum Color : int {
			WHITE = 0,
			BLACK = 1
		};

		/**
		 * Constants used to describe fpmas::synchro::hard::HardSyncMutex
		 * requests.
		 */
		enum class MutexRequestType {
			READ, LOCK, ACQUIRE, LOCK_SHARED
		};
	}
}}}
#endif
