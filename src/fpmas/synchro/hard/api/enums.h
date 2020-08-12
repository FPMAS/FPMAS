#ifndef HARD_SYNC_ENUMS_H
#define HARD_SYNC_ENUMS_H

namespace fpmas { namespace synchro {
	namespace hard { namespace api {
		enum Epoch : int {
			EVEN = 0x00,
			ODD = 0x10
		};

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
			TOKEN = 0x0D,
			END = 0x0E
		};

		/**
		 * Used by the termination algorithm.
		 */
		enum Color : int {
			WHITE = 0,
			BLACK = 1
		};

		enum class MutexRequestType {
			READ, LOCK, ACQUIRE, LOCK_SHARED
		};
	}
}}}
#endif
