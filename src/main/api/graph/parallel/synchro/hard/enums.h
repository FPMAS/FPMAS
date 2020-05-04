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
		TOKEN = 0x08,
		END = 0x09
	};
}
#endif
