#ifndef FPMAS_HARD_SYNC_ENUMS_H
#define FPMAS_HARD_SYNC_ENUMS_H

/** \file src/fpmas/synchro/hard/api/enums.h
 * HardSyncMode related enums.
 */

#include "fpmas/io/datapack.h"

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

namespace fpmas { namespace io { namespace datapack {
	/**
	 * Color base_io specialization.
	 */
	template<>
		struct base_io<synchro::hard::api::Color> {
			/**
			 * Returns the buffer size, in bytes, required to serialize a
			 * Color instance in a DataPack, i.e.
			 * `datapack::pack_size<int>()`.
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size() {
				return datapack::pack_size<int>();
			}
			/**
			 * Equivalent to pack_size().
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const synchro::hard::api::Color&) {
				return pack_size();
			}

			/**
			 * Writes `color` to the `data_pack` buffer at the given `offset`.
			 * pack_size() bytes are written, and `offset` is incremented
			 * accordingly.
			 *
			 * @param data_pack destination DataPack
			 * @param color source color
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(DataPack& data_pack, const synchro::hard::api::Color& color,
					std::size_t& offset) {
				int i = color;
				datapack::write(data_pack, i, offset);
			}

			/**
			 * Reads a Color from the `data_pack` buffer at the
			 * given `offset`. pack_size() bytes are read, and `offset` is
			 * incremented accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param color destination color
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(const DataPack& data_pack, synchro::hard::api::Color& color,
					std::size_t offset) {
				int i;
				datapack::read(data_pack, i, offset);
				color = (synchro::hard::api::Color) i;
			}
		};
}}}
#endif
