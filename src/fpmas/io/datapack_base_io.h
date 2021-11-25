#ifndef FPMAS_DATAPACK_BASE_IO_H
#define FPMAS_DATAPACK_BASE_IO_H

#include "fpmas/api/communication/communication.h"
#include <set>
#include <list>
#include <map>
#include <deque>

namespace fpmas { namespace io { namespace datapack {
	using fpmas::api::communication::DataPack;

		/**
		 * The purpose of the base_io interface is to provide static methods to
		 * efficiently read and write data to pre-allocated DataPack buffers.
		 *
		 * This void base_io structure **must** be specialized.
		 *
		 * Any specialization for a type `T` must eventually implement the
		 * following methods:
		 * <table>
		 * <tr><th>method</th><th>description</th><th>note</th></tr>
		 * <tr>
		 * <td>`std::size_t pack_size()`</td>
		 * <td>
		 * Returns the buffer size, in bytes, required to serialize an instance of `T`
		 * in a DataPack.
		 * </td>
		 * <td>
		 * Can be implemented only if the same buffer size is required for **any**
		 * instance of type `T`.
		 * </td>
		 * </tr>
		 * <tr>
		 * <td>`std::size_t pack_size(const T& data)`</td>
		 * <td>
		 * Returns the buffer size, in bytes, required to serialize the specified
		 * instance of `T` in a DataPack.
		 * </td>
		 * <td>
		 * Might be equivalent to pack_size() if the required size is not
		 * dependent on the specified instance.
		 *
		 * Examples of instance dependent pack sizes are `std::string` or
		 * `std::vector<T>`.
		 * </td>
		 * </tr>
		 * <tr>
		 * <td>
		 * `void write(DataPack& data_pack, const T& data, std::size_t& offset)`
		 * </td>
		 * <td>
		 * Writes `data` to the `data_pack`, starting at the specified
		 * `offset`. Upon return, `offset` must correspond to the byte
		 * following the last byte written.
		 * </td>
		 * <td>
		 * Given `size=pack_size()` or `size=pack_size(data)` bytes are written
		 * from `&data_pack.buffer[offset]`.
		 * </td>
		 * </tr>
		 * <tr>
		 * <td>
		 * `void read(const DataPack& data_pack, T& data, std::size_t& offset)`
		 * </td>
		 * <td>
		 * Reads `data` from the `data_pack`, starting at the specified `offset`.
		 * </td>
		 * <td>
		 * Given `size=pack_size()` or `size=pack_size(data)` bytes are read
		 * from `&data_pack.buffer[offset]`. Upon return, `offset` must
		 * correspond to the byte following the last byte read.
		 * </td>
		 * </tr>
		 * </table>
		 *
		 * Write operations assume that the specified DataPack has been already
		 * allocated, and that enough space is available to perform the write()
		 * operation at the given offset. The pack_size() methods can be used
		 * to easily allocate a DataPack before writting into it.
		 *
		 * Available specializations can be found in the fpmas::io::datapack
		 * namespace.
		 * The following helper methods can be used directly:
		 * - pack_size<T>()
		 * - pack_size(const T&)
		 * - write(DataPack& data_pack, const T& data, std::size_t& offset)
		 * - read(const DataPack& dat_pack, T& data, std::size_t& offset)
		 *
		 * @tparam T type of data to read / write to the DataPack buffer
		 * @tparam Enable SFINAE condition
		 */
		template<typename T, typename Enable = void>
		struct base_io {
		};

	/**
	 * base_io<T>::pack_size() helper method.
	 *
	 * @see base_io
	 */
	template<typename T>
		std::size_t pack_size() {
			return base_io<T>::pack_size();
		}
	/**
	 * base_io<T>::pack_size(const T&) helper method.
	 *
	 * @see base_io
	 */
	template<typename T>
		std::size_t pack_size(const T& data) {
			return base_io<T>::pack_size(data);
		}

	/**
	 * base_io<T>::write(DataPack&, const T&, std::size_t&) helper method.
	 *
	 * @see base_io
	 */
	template<typename T>
		void write(DataPack& data_pack, const T& data, std::size_t& offset) {
			base_io<T>::write(data_pack, data, offset);
		}

	/**
	 * base_io<T>::read(const DataPack&, T&, std::size_t&) helper method.
	 *
	 * @see base_io
	 */
	template<typename T>
		void read(const DataPack& data_pack, T& data, std::size_t& offset) {
			base_io<T>::read(data_pack, data, offset);
		}

	/**
	 * Default base_io implementation, only available for fundamental types.
	 *
	 * @tparam T fundamental type (std::size_t, std::int32_t, unsigned int...)
	 */
	template<typename T>
		struct base_io<T, typename std::enable_if<std::is_fundamental<T>::value>::type> {
			/**
			 * Returns the buffer size, in bytes, required to serialize an
			 * instance of T in a DataPack.
			 *
			 * This fundamental type implementation returns `sizeof(T)`.
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size() {
				return sizeof(T);
			}

			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified instance of T in a DataPack.
			 *
			 * This fundamental type implementation returns `sizeof(T)`.
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const T&) {
				return sizeof(T);
			}


			/**
			 * Writes `data` to the `data_pack` buffer at the given `offset`
			 * using an `std::memcpy` operation. pack_size() bytes are
			 * written, and `offset` is incremented accordingly.
			 *
			 * @param data_pack destination DataPack
			 * @param data source data
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(DataPack& data_pack, const T& data, std::size_t& offset) {
				std::memcpy(&data_pack.buffer[offset], &data, pack_size());
				offset += pack_size();
			}

			/**
			 * Reads `data` from the `data_pack` buffer at the given `offset`
			 * using an `std::memcpy` operation. pack_size() bytes are
			 * read, and `offset` is incremented accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param data destination data
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(const DataPack& data_pack, T& data, std::size_t& offset) {
				std::memcpy(&data, &data_pack.buffer[offset], pack_size());
				offset += pack_size();
			}
		};

	/**
	 * api::graph::DistributedId base_io specialization.
	 */
	template<>
		struct base_io<DistributedId> {
			/**
			 * Returns the buffer size, in bytes, required to serialize a
			 * DistributedId instance, i.e.
			 * `datapack::pack_size<int>()+datapack::pack_size<FPMAS_ID_TYPE>`.
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size();

			/**
			 * Equivalent to pack_size().
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const DistributedId&);

			/**
			 * Writes `id` to the `data_pack` buffer at the given `offset`.
			 * pack_size() bytes are written, and `offset` is incremented
			 * accordingly.
			 *
			 * @param data_pack destination DataPack
			 * @param id source id
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& data_pack, const DistributedId& id, std::size_t& offset);
			/**
			 * Reads a DistributedId from the `data_pack` buffer at the
			 * given `offset`. pack_size() bytes are read, and `offset` is
			 * incremented accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param id destination id
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& data_pack, DistributedId& id, std::size_t& offset);
		};

	/**
	 * std::string base_io specialization.
	 */
	template<>
		struct base_io<std::string> {
			/**
			 * Returns the buffer size, in bytes, required to serialize a
			 * DistributedId instance, i.e.
			 * `%datapack::pack_size<std::size_t>()+str.size()*%datapack::pack_size<char>`.
			 *
			 * @param str std::string instance to serialize
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const std::string& str);

			/**
			 * Writes `str` to the `data_pack` buffer at the given
			 * `offset`. pack_size(str) bytes are written, and `offset` is
			 * incremented accordingly.
			 *
			 * First, the size of the string is written
			 * (datapack::pack_size<std::size_t> bytes) and then the
			 * `str.data()` char array is written contiguously.
			 *
			 * @param data_pack destination DataPack
			 * @param str source string
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& data_pack, const std::string& str, std::size_t& offset);

			/**
			 * Reads an std::string from the `data_pack` buffer at the
			 * given `offset`. The count of bytes read depends on the
			 * corresponding write() operation. `offset` is incremented
			 * accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param str destination string
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& data_pack, std::string& str, std::size_t& offset);
		};

	/**
	 * std::vector base_io specialization.
	 *
	 * | Serialization scheme |||||
	 * |----------------------|||||
	 * | _scheme_ | N | item1 | item2 | ... |
	 * | _serializer_ | base_io<std::size_t> | base_io<T> |||
	 *
	 * @tparam T type of data contained into the vector. A base_io
	 * specialization of T must be available.
	 */
	template<typename T>
		struct base_io<std::vector<T>> {
			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified vector, i.e.
			 * `datapack::pack_size<std::size_t>()+N` where `N` is the sum
			 * of `datapack::pack_size(item)` for each `item` in `vec`.
			 *
			 * @param vec std::vector instance to serialize
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const std::vector<T>& vec);

			/**
			 * Writes `vec` to the `data_pack` buffer at the given
			 * `offset`. pack_size(vec) bytes are written, and `offset` is
			 * incremented accordingly.
			 *
			 * First, `vec.size` is written
			 * (datapack::pack_size<std::size_t> bytes) and then each
			 * `item` in `vec` are written contiguously using the
			 * datapack::write() specialization for T.
			 *
			 * @param data_pack destination DataPack
			 * @param vec source std::vector
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& data_pack, const std::vector<T>& vec,
					std::size_t& offset);

			/**
			 * Reads an std::vector from the `data_pack` buffer at the
			 * given `offset`. The count of bytes read depends on the
			 * corresponding write() operation. `offset` is incremented
			 * accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param vec destination std::vector
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& data_pack, std::vector<T>& vec,
					std::size_t& offset);
		};

	template<typename T>
		std::size_t base_io<std::vector<T>>::pack_size(const std::vector<T>& vec) {
			std::size_t size = datapack::pack_size<std::size_t>();
			for(auto item : vec)
				size += datapack::pack_size(item);
			return size;
		};

	template<typename T>
		void base_io<std::vector<T>>::write(
				DataPack& data_pack, const std::vector<T>& vec,
				std::size_t& offset) {
			datapack::write(data_pack, vec.size(), offset);
			for(auto item : vec)
				datapack::write(data_pack, item, offset);
		}

	template<typename T>
		void base_io<std::vector<T>>::read(
				const DataPack& data_pack, std::vector<T>& vec,
				std::size_t& offset) {
			std::size_t size;
			datapack::read(data_pack, size, offset);
			vec.resize(size);
			for(std::size_t i = 0; i < size; i++)
				datapack::read(data_pack, vec[i], offset);
		}

	/**
	 * std::set base_io specialization.
	 *
	 * | Serialization scheme |||||
	 * |----------------------|||||
	 * | _scheme_ | N | item1 | item2 | ... |
	 * | _serializer_ | base_io<std::size_t> | base_io<T> |||
	 *
	 * @tparam T type of data contained into the set. A base_io
	 * specialization of T must be available.
	 */
	template<typename T>
		struct base_io<std::set<T>> {
			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified set, i.e.
			 * `datapack::pack_size<std::size_t>()+N` where `N` is the sum
			 * of `datapack::pack_size(item)` for each `item` in `set`.
			 *
			 * @param set std::set instance to serialize
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const std::set<T>& set);

			/**
			 * Writes `set` to the `data_pack` buffer at the given
			 * `offset`. pack_size(set) bytes are written, and `offset` is
			 * incremented accordingly.
			 *
			 * First, `set.size` is written
			 * (datapack::pack_size<std::size_t> bytes) and then each
			 * `item` in `set` are written contiguously using the
			 * datapack::write() specialization for T, preserving the order of
			 * items in the set.
			 *
			 * @param data_pack destination DataPack
			 * @param set source std::set
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& data_pack, const std::set<T>& set,
					std::size_t& offset);

			/**
			 * Reads an std::set from the `data_pack` buffer at the
			 * given `offset`. The count of bytes read depends on the
			 * corresponding write() operation. `offset` is incremented
			 * accordingly.
			 *
			 * Since items are already ordered, a linear complexity to build
			 * the set is ensured.
			 *
			 * @param data_pack source DataPack
			 * @param set destination std::set
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& data_pack, std::set<T>& set,
					std::size_t& offset);
		};

	template<typename T>
		std::size_t base_io<std::set<T>>::pack_size(const std::set<T>& set) {
			std::size_t size = datapack::pack_size<std::size_t>();
			for(auto item : set)
				size += datapack::pack_size(item);
			return size;
		};

	template<typename T>
		void base_io<std::set<T>>::write(
				DataPack& data_pack, const std::set<T>& set,
				std::size_t& offset) {
			datapack::write(data_pack, set.size(), offset);
			// Items are written in order
			for(auto item : set)
				datapack::write(data_pack, item, offset);
		}

	template<typename T>
		void base_io<std::set<T>>::read(
				const DataPack& data_pack, std::set<T>& set,
				std::size_t& offset) {
			std::size_t size;
			datapack::read(data_pack, size, offset);
			auto current_hint = set.end();
			for(std::size_t i = 0; i < size; i++) {
				T item;
				datapack::read(data_pack, item, offset);
				// Constant time insertion before end, since data is
				// already sorted.
				set.emplace_hint(current_hint, std::move(item));
				// Updates hint
				current_hint = set.end();
			}
		}

	/**
	 * std::list base_io specialization.
	 *
	 * | Serialization scheme |||||
	 * |----------------------|||||
	 * | _scheme_ | N | item1 | item2 | ... |
	 * | _serializer_ | base_io<std::size_t> | base_io<T> |||
	 *
	 * @tparam T type of data contained into the list. A base_io
	 * specialization of T must be available.
	 */
	template<typename T>
		struct base_io<std::list<T>> {
			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified list, i.e.
			 * `datapack::pack_size<std::size_t>()+N` where `N` is the sum
			 * of `datapack::pack_size(item)` for each `item` in `list`.
			 *
			 * @param list std::list instance to serialize
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const std::list<T>& list);

			/**
			 * Writes `list` to the `data_pack` buffer at the given
			 * `offset`. pack_size(list) bytes are written, and `offset` is
			 * incremented accordingly.
			 *
			 * First, `list.size` is written
			 * (datapack::pack_size<std::size_t> bytes) and then each
			 * `item` in `list` are written contiguously using the
			 * datapack::write() specialization for T.
			 *
			 * @param data_pack destination DataPack
			 * @param list source std::list
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& data_pack, const std::list<T>& list,
					std::size_t& offset);

			/**
			 * Reads an std::list from the `data_pack` buffer at the
			 * given `offset`. The count of bytes read depends on the
			 * corresponding write() operation. `offset` is incremented
			 * accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param list destination std::list
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& data_pack, std::list<T>& list,
					std::size_t& offset);
		};

	template<typename T>
		std::size_t base_io<std::list<T>>::pack_size(const std::list<T>& list) {
			std::size_t size = datapack::pack_size<std::size_t>();
			for(auto item : list)
				size += datapack::pack_size(item);
			return size;
		};

	template<typename T>
		void base_io<std::list<T>>::write(
				DataPack& data_pack, const std::list<T>& list,
				std::size_t& offset) {
			datapack::write(data_pack, list.size(), offset);
			// Items are written in order
			for(auto item : list)
				datapack::write(data_pack, item, offset);
		}

	template<typename T>
		void base_io<std::list<T>>::read(
				const DataPack& data_pack, std::list<T>& list,
				std::size_t& offset) {
			std::size_t size;
			datapack::read(data_pack, size, offset);
			for(std::size_t i = 0; i < size; i++) {
				T item;
				datapack::read(data_pack, item, offset);
				list.emplace_back(std::move(item));
			}
		}

	/**
	 * std::deque base_io specialization.
	 *
	 * | Serialization scheme |||||
	 * |----------------------|||||
	 * | _scheme_ | N | item1 | item2 | ... |
	 * | _serializer_ | base_io<std::size_t> | base_io<T> |||
	 *
	 * @tparam T type of data contained into the list. A base_io
	 * specialization of T must be available.
	 */
	template<typename T>
		struct base_io<std::deque<T>> {
			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified deque, i.e.
			 * `datapack::pack_size<std::size_t>()+N` where `N` is the sum
			 * of `datapack::pack_size(item)` for each `item` in `deque`.
			 *
			 * @param deque std::deque instance to serialize
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const std::deque<T>& deque);

			/**
			 * Writes `deque` to the `data_pack` buffer at the given `offset`.
			 * pack_size(deque) bytes are written, and `offset` is incremented
			 * accordingly.
			 *
			 * First, `deque.size` is written
			 * (datapack::pack_size<std::size_t> bytes) and then each
			 * `item` in `deque` are written contiguously using the
			 * datapack::write() specialization for T.
			 *
			 * @param data_pack destination DataPack
			 * @param deque source std::deque
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& data_pack, const std::deque<T>& deque,
					std::size_t& offset);

			/**
			 * Reads an std::deque from the `data_pack` buffer at the
			 * given `offset`. The count of bytes read depends on the
			 * corresponding write() operation. `offset` is incremented
			 * accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param deque destination std::deque
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& data_pack, std::deque<T>& deque,
					std::size_t& offset);
		};

	template<typename T>
		std::size_t base_io<std::deque<T>>::pack_size(const std::deque<T>& deque) {
			std::size_t size = datapack::pack_size<std::size_t>();
			for(auto item : deque)
				size += datapack::pack_size(item);
			return size;
		};

	template<typename T>
		void base_io<std::deque<T>>::write(
				DataPack& data_pack, const std::deque<T>& deque,
				std::size_t& offset) {
			datapack::write(data_pack, deque.size(), offset);
			// Items are written in order
			for(auto item : deque)
				datapack::write(data_pack, item, offset);
		}

	template<typename T>
		void base_io<std::deque<T>>::read(
				const DataPack& data_pack, std::deque<T>& deque,
				std::size_t& offset) {
			std::size_t size;
			datapack::read(data_pack, size, offset);
			for(std::size_t i = 0; i < size; i++) {
				T item;
				datapack::read(data_pack, item, offset);
				deque.emplace_back(std::move(item));
			}
		}
	/**
	 * std::pair base_io specialization.
	 *
	 * | Serialization scheme |||
	 * |----------------------|||
	 * | _scheme_ | first | second |
	 * | _serializer_ | base_io<T1> | base_io<T2> |
	 *
	 * @tparam T1 pair first type
	 * @tparam T2 pair second type
	 */
	template<typename T1, typename T2>
		struct base_io<std::pair<T1, T2>> {
			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified pair, i.e.
			 * `datapack::pack_size(pair.first)+datapack::pack_size(pair.second)`.
			 *
			 * @param pair std::pair instance to serialize
			 * 
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const std::pair<T1, T2>& pair) {
				return datapack::pack_size(pair.first)
					+ datapack::pack_size(pair.second);
			}

			/**
			 * Writes `pair` to the `data_pack` buffer at the given
			 * `offset`. pack_size(pair) bytes are written, and `offset` is
			 * incremented accordingly.
			 *
			 * `pair.first` and `pair.second` are written contiguously using the
			 * datapack::write() specialization for T1 and T2.
			 *
			 * @param data_pack destination DataPack
			 * @param pair source std::pair
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& data_pack, const std::pair<T1, T2>& pair,
					std::size_t& offset) {
				datapack::write(data_pack, pair.first, offset);
				datapack::write(data_pack, pair.second, offset);
			}

			/**
			 * Reads an std::pair from the `data_pack` buffer at the
			 * given `offset`. The count of bytes read depends on the
			 * corresponding write() operation. `offset` is incremented
			 * accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param pair destination std::pair
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& data_pack, std::pair<T1, T2>& pair,
					std::size_t& offset) {
				datapack::read(data_pack, pair.first, offset);
				datapack::read(data_pack, pair.second, offset);
			}
		};

	/**
	 * std::unordered_map base_io specialization.
	 *
	 * | Serialization scheme |||||||
	 * |----------------------|||||||
	 * | _scheme_ | N | item1.key | item1.value | item2.key | item2.value | ... |
	 * | _serializer_ | base_io<std::size_t> | base_io<K> | base_io<T> | base_io<K> | base_io<T> | ... |
	 *
	 * @tparam K key type
	 * @tparam T value type
	 */
	template<typename K, typename T>
		struct base_io<std::unordered_map<K, T>> {
			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified map, i.e.
			 * `datapack::pack_size<std::size_t>()+N` where `N` is the sum of
			 * `datapack::pack_size(item.first)+datapack::pack_size(item.second)`
			 * for each item in `map`.
			 *
			 * @param map std::map instance to serialize
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const std::unordered_map<K, T>& map);

			/**
			 * Writes `map` to the `data_pack` buffer at the given
			 * `offset`. `pack_size(map)` bytes are written, and `offset` is
			 * incremented accordingly.
			 *
			 * First, `list.size` is written (datapack::pack_size<std::size_t>
			 * bytes) and then `item.first` and `item.second` are written
			 * contiguously for each item in the map using the
			 * datapack::write() specializations for key and value types.
			 *
			 * @param data_pack destination DataPack
			 * @param map source map 
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& data_pack, const std::unordered_map<K, T>& map,
					std::size_t& offset);

			/**
			 * Reads an std::map from the `data_pack` buffer at the
			 * given `offset`. The count of bytes read depends on the
			 * corresponding write() operation. `offset` is incremented
			 * accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param map destination std::map
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& data_pack, std::unordered_map<K, T>& map,
					std::size_t& offset);
		};

	template<typename K, typename T>
		std::size_t base_io<std::unordered_map<K, T>>::pack_size(const std::unordered_map<K, T>& map) {
			std::size_t size = datapack::pack_size<std::size_t>();
			for(auto item : map)
				size += (datapack::pack_size(item.first) + datapack::pack_size(item.second));
			return size;
		};

	template<typename K, typename T>
		void base_io<std::unordered_map<K, T>>::write(
				DataPack& data_pack, const std::unordered_map<K, T>& map,
				std::size_t& offset) {
			datapack::write(data_pack, map.size(), offset);
			// Items are written in order
			for(auto item : map) {
				datapack::write(data_pack, item.first, offset);
				datapack::write(data_pack, item.second, offset);
			}
		}

	template<typename K, typename T>
		void base_io<std::unordered_map<K, T>>::read(
				const DataPack& data_pack, std::unordered_map<K, T>& map,
				std::size_t& offset) {
			std::size_t size;
			datapack::read(data_pack, size, offset);
			for(std::size_t i = 0; i < size; i++) {
				K key;
				T value;
				datapack::read(data_pack, key, offset);
				datapack::read(data_pack, value, offset);
				map.emplace(std::move(key), std::move(value));
			}
		}

	/**
	 * std::array base_io specialization.
	 *
	 * | Serialization scheme ||||
	 * |----------------------||||
	 * | _scheme_ | item1 | item2 | ... |
	 * | _serializer_ | base_io<T> |||
	 *
	 * @tparam T value type
	 * @tparam N array size
	 */
	template<typename T, std::size_t N>
		struct base_io<std::array<T, N>> {
			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified array, i.e.  the sum of `datapack::pack_size(item)`
			 * for each item in `array`.
			 *
			 * @param array std::array instance to serialize
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const std::array<T, N>& array) {
				std::size_t size = 0;
				for(auto item : array)
					size+=datapack::pack_size(item);
				return size;
			}

			/**
			 * Writes `array` to the `data_pack` buffer at the given
			 * `offset`. `pack_size(array)` bytes are written, and `offset` is
			 * incremented accordingly.
			 *
			 * `pair.first` and `pair.second` are written contiguously using the
			 * datapack::write() specializations for T1 and T2.
			 *
			 * Items are written contiguously using the datapack::write()
			 * specialization for T.
			 *
			 * @param data_pack destination DataPack
			 * @param array source std::array
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& data_pack, const std::array<T, N>& array,
					std::size_t& offset) {
				for(auto item : array)
					datapack::write(data_pack, item, offset);
			}

			/**
			 * Reads an std::array from the `data_pack` buffer at the
			 * given `offset`. The count of bytes read depends on the
			 * corresponding write() operation. `offset` is incremented
			 * accordingly.
			 *
			 * @param data_pack source DataPack
			 * @param array destination std::array
			 * @param offset `data_pack.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& data_pack, std::array<T, N>& array,
					std::size_t& offset) {
				for(std::size_t i = 0; i < N; i++)
					datapack::read(data_pack, array[i], offset);
			}
		};

	/**
	 * api::communication::DataPack base_io specialization.
	 *
	 * The purpose of this specialization is to read / write a DataPack
	 * from / to a larger DataPack, efficiently copying raw binary data.
	 *
	 * | Serialization scheme |||
	 * |----------------------|||
	 * | _scheme_ | DataPack.size | DataPack.buffer |
	 * | _serializer_ | base_io<std::size_t> | std::memcpy |
	 */
	template<>
		struct base_io<DataPack> {
			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified DataPack instance, i.e.
			 * `datapack::pack_size<std::size_t>()+data.size`.
			 *
			 * @param data DataPack instance to serialize
			 *
			 * @return pack size in bytes
			 */
			static std::size_t pack_size(const DataPack& data);

			/**
			 * Writes `source` to the `destination` DataPack buffer at the
			 * given `offset`. pack_size() bytes are written, and `offset`
			 * is incremented accordingly.
			 *
			 * First, `source.size` is written
			 * (datapack::pack_size<std::size_t> bytes) and then the
			 * `source.buffer()` char array is written contiguously.
			 *
			 * @param destination destination DataPack
			 * @param source source DataPack
			 * @param offset `destination.buffer` index at which the first
			 * byte is written
			 */
			static void write(
					DataPack& destination, const DataPack& source, std::size_t& offset);

			/**
			 * Reads a DataPack from the `source` buffer at the given
			 * `offset` into the `destination` buffer. The count of bytes
			 * read depends on the corresponding write() operation.
			 * `offset` is incremented accordingly.
			 *
			 * @param source source DataPack
			 * @param destination destination DataPack
			 * @param offset `source.buffer` index at which the first
			 * byte is read
			 */
			static void read(
					const DataPack& source, DataPack& destination, std::size_t& offset);
		};

}}}
#endif
