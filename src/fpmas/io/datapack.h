#ifndef FPMAS_IO_DATAPACK_H
#define FPMAS_IO_DATAPACK_H

/** \file src/fpmas/io/datapack.h
 * Binary DataPack based serialization features.
 */

#include "fpmas/api/communication/communication.h"
#include "fpmas/io/json.h"

namespace fpmas { namespace io {
	/**
	 * Namespace containing binary DataPack based serialization features.
	 */
	namespace datapack {
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
		 *
		 * The following helper methods can be used directly:
		 * - pack_size<T>()
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
				 */
				static std::size_t pack_size() {
					return sizeof(T);
				}

				/**
				 * Returns the buffer size, in bytes, required to serialize the
				 * specified instance of T in a DataPack.
				 *
				 * This fundamental type implementation returns `sizeof(T)`.
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
				 */
				static std::size_t pack_size();
				/**
				 * Equivalent to pack_size().
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
				 * `datapack::pack_size<std::size_t>()+str.size()*datapack::pack_size<char>`.
				 *
				 * @param str std::string instance to serialize
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
		 * api::communication::DataPack base_io specialization.
		 *
		 * The purpose of this specialization is to read / write a DataPack
		 * from / to a larger DataPack, efficiently copying raw binary data.
		 */
		template<>
			struct base_io<DataPack> {
				 /**
				 * Returns the buffer size, in bytes, required to serialize the
				 * specified DataPack instance, i.e.
				 * `datapack::pack_size<std::size_t>()+data.size`.
				 *
				 * @param str std::string instance to serialize
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

		/**
		 * std::vector base_io specialization.
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
				 * corresponding write() operation.  `offset` is incremented
				 * accordingly.
				 *
				 * @param data_pack source DataPack
				 * @param destination destination std::vector
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
				base_io<std::size_t>::write(data_pack, vec.size(), offset);
				for(auto item : vec)
					base_io<T>::write(data_pack, item, offset);
			}

		template<typename T>
			void base_io<std::vector<T>>::read(
					const DataPack& data_pack, std::vector<T>& vec,
					std::size_t& offset) {
				std::size_t size;
				base_io<std::size_t>::read(data_pack, size, offset);
				vec.resize(size);
				for(std::size_t i = 0; i < size; i++)
					base_io<T>::read(data_pack, vec[i], offset);
			}

		template<template<typename> class S>
			class BasicObjectPack {
				friend base_io<BasicObjectPack<S>>;

				private:
				DataPack _data;

				std::size_t write_offset = 0;
				mutable std::size_t read_offset = 0;

				public:
				BasicObjectPack() = default;

				template<typename T>
					BasicObjectPack(const T& item)
					: BasicObjectPack(S<T>::template to_datapack<BasicObjectPack<S>>(item)) {
					}

				template<typename T>
					BasicObjectPack<S>& operator=(const T& item) {
						*this = S<T>::template to_datapack<BasicObjectPack<S>>(item);
						return *this;
					}

				const DataPack& data() const {
					return _data;
				}

				template<typename T>
					T get() const {
						return S<T>::from_datapack(*this);
					}

				void allocate(std::size_t size) {
					_data = {size, 1};
				}

				template<typename T>
					void write(const T& item) {
						datapack::write(this->_data, item, write_offset);
					}

				template<typename T>
					T read() const {
						T item;
						read(item);
						return item;
					}

				template<typename T>
					void read(T& item) const {
						datapack::read(this->_data, item, read_offset);
					}

				DataPack dump() {
					write_offset = 0;
					read_offset = 0;
					return std::move(_data);
				}

				static BasicObjectPack<S> parse(DataPack&& data_pack) {
					BasicObjectPack<S> pack;
					pack._data = std::move(data_pack);
					return pack;
				}
			};

		/**
		 * BasicObjectPack base_io specialization.
		 */
		template<template<typename> class S>
			struct base_io<BasicObjectPack<S>> {
				/**
				 * Returns the buffer size, in bytes, required to serialize
				 * `pack`, i.e. `datapack::pack_size(pack.data())`.
				 *
				 * @param pack BasicObjectPack to serialize
				 */
				static std::size_t pack_size(const BasicObjectPack<S>& pack) {
					return datapack::pack_size(pack._data);
				}

				/**
				 * Writes `object_pack` to the `data_pack` buffer at the given
				 * `offset`. pack_size(object_pack) bytes are written, and `offset` is
				 * incremented accordingly.
				 *
				 * This is equivalent to a call to
				 * `datapack::write(data_pack, object_pack.data(), offset)`.
				 *
				 * @param data_pack destination DataPack
				 * @param object_pack source BasicObjectPack 
				 * @param offset `data_pack.buffer` index at which the first
				 * byte is written
				 */
				static void write(
						DataPack& data_pack, const BasicObjectPack<S>& object_pack,
						std::size_t& offset) {
					datapack::write(data_pack, object_pack._data, offset);
				}

				/**
				 * Reads a BasicObjectPack from the `data_pack` buffer at the
				 * given `offset`. The count of bytes read depends on the
				 * corresponding write() operation.  `offset` is incremented
				 * accordingly.
				 *
				 * Data is read directly into pack.data().
				 *
				 * @param data_pack source DataPack
				 * @param destination destination BasicObjectPack
				 * @param offset `data_pack.buffer` index at which the first
				 * byte is read
				 */
				static void read(
						const DataPack& data_pack, BasicObjectPack<S>& pack, std::size_t& offset) {
					datapack::read(data_pack, pack._data, offset);
				}

			};

		/*
		 *    template<typename T>
		 *        struct Serializer {
		 *            template<typename PackType>
		 *            static PackType to_datapack(const T& item) {
		 *                PackType pack;
		 *                pack.allocate(pack_size(item));
		 *                pack.write(item);
		 *                return pack;
		 *            }
		 *
		 *            template<typename PackType>
		 *            static T from_datapack(const PackType& pack) {
		 *                return pack.template read<T>();
		 *            }
		 *        };
		 */

		template<typename T, typename JsonType>
			struct JsonSerializer {
				template<typename PackType>
					static PackType to_datapack(const T& data) {
						std::string str = JsonType(data).dump();
						PackType pack;
						pack.allocate(pack_size(str));
						pack.write(str);
						return pack;
					}

				template<typename PackType>
					static T from_datapack(const PackType& pack) {
						return JsonType::parse(
								pack.template read<std::string>()
								).template get<T>();
					}
			};

		template<typename T>
			struct Serializer : public JsonSerializer<T, nlohmann::json> {
			};

		template<typename T>
			struct Serializer<std::vector<T>> {
				template<typename PackType>
					static PackType to_datapack(const std::vector<T>& data) {
						std::vector<PackType> packs(data.size());

						for(std::size_t i = 0; i < data.size(); i++) {
							// Serialize
							packs[i] = PackType(data[i]);
						}
						PackType total_pack;
						total_pack.allocate(datapack::pack_size(packs));
						total_pack.write(packs);
						return total_pack;
					}

				template<typename PackType>
					static std::vector<T> from_datapack(const PackType& pack) {
						std::vector<PackType> packs
							= pack.template read<std::vector<PackType>>();
						// Cannot preallocate data vector if T is not default
						// constructible
						std::vector<T> data;
						for(auto& item : packs) {
							PackType pack = std::move(item);
							data.emplace_back(pack.template get<T>());
							// Pack is freed
						}
						return data;
					}
			};

		typedef BasicObjectPack<Serializer> ObjectPack;

		template<typename T>
			struct LightSerializer : public JsonSerializer<T, fpmas::io::json::light_json> {

			};

		template<typename T>
			struct LightSerializer<std::vector<T>> : public Serializer<std::vector<T>> {
			};

		typedef BasicObjectPack<LightSerializer> LightObjectPack;
	}}
}
#endif
