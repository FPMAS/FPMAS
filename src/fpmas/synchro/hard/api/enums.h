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
		enum Color : std::uint8_t {
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
		struct Serializer<synchro::hard::api::Color> {
			/**
			 * Returns the buffer size, in bytes, required to serialize a Color
			 * instance in `p`, i.e. `p.size<int>()`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p) {
					return p.template size<std::uint8_t>();
				}

			/**
			 * Equivalent to size().
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p, const synchro::hard::api::Color&) {
					return p.template size<std::uint8_t>();
				}

			/**
			 * Writes `color` to the `pack` buffer.
			 *
			 * @param pack destination BasicObjectPack
			 * @param color Color to serialize
			 */
			template<typename PackType>
				static void to_datapack(
						PackType& pack, const synchro::hard::api::Color& color) {
					pack.template put((std::uint8_t) color);
				}

			/**
			 * Reads a Color from the `pack` buffer.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized Color
			 */
			template<typename PackType>
			static synchro::hard::api::Color from_datapack(const PackType& pack) {
				return (synchro::hard::api::Color) pack.template get<std::uint8_t>();
			}
		};
}}}
#endif
