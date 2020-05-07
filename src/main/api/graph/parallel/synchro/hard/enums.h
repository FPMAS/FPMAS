#ifndef HARD_SYNC_ENUMS_H
#define HARD_SYNC_ENUMS_H

namespace FPMAS::api::graph::parallel::synchro::hard {
	enum Epoch : int {
		EVEN = 0x00,
		ODD = 0x10
	};

	enum Tag : int {
		READ = 0x00,
		READ_RESPONSE = 0x01,
		ACQUIRE = 0x02,
		ACQUIRE_RESPONSE = 0x03,
		RELEASE = 0x04,
		LOCK = 0x05,
		LOCK_RESPONSE = 0x06,
		UNLOCK = 0x07,
		LINK = 0x08,
		UNLINK = 0x09,
		TOKEN = 0x0A,
		END = 0x0B
	};

	/**
	 * Used by the termination algorithm.
	 */
	enum Color : int {
		WHITE = 0,
		BLACK = 1
	};

	enum class MutexRequestType {
		READ, LOCK, ACQUIRE
	};
}
#endif
