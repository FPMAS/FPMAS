#ifndef FPMAS_IO_DATAPACK_H
#define FPMAS_IO_DATAPACK_H

/** \file src/fpmas/io/datapack.h
 * Binary DataPack based serialization features.
 */

#include "datapack_base_io.h"
#include "fpmas/io/json.h"

namespace fpmas { namespace io {
	/**
	 * Namespace containing binary DataPack based serialization features.
	 */
	namespace datapack {

		/**
		 * Base object used to implement the FPMAS binary serialization
		 * technique.
		 *
		 * The purpose of the BasicObjectPack is to provide an high level API
		 * to efficiently read/write data from/to a low level DataPack.
		 *
		 * The BasicObjectPack is the _container_ that provides access to the
		 * serialized data. Serialization / deserialization is ensured by the
		 * provided serializer `S` and by `base_io` specializations.
		 *
		 * The BasicObjectPack provides two serialization techniques.
		 * 1. The Serializer technique is used by BasicObjectPack(const T&),
		 * operator=(const T&) and get().
		 * The S<T>::to_datapack() and S<T>::from_datapack() methods are used
		 * to perform those serialization, so the corresponding specializations
		 * must be available for T.
		 * ```cpp
		 * using namespace fpmas::io::datapack;
		 *
		 * std::string data = "Hello World";
		 * ObjectPack pack = data;
		 *
		 * std::string unserial_data = pack.get<std::string>();
		 * ```
		 *
		 * 2. The base_io technique is used by read() and write(). A base_io
		 * specialization must be available for the data to serialize in order
		 * to use this technique.
		 * First, the internal buffer must be allocated using the allocate()
		 * method. The base_io<T>::pack_size() specializations can be used to
		 * query the required size. write() operations can then be performed,
		 * and data can be retrieved using read() methods. Write and read must
		 * be performed in the **same order**.
		 * ```cpp
		 * using namespace fpmas::io::datapack;
		 *
		 * std::uint64_t i = 23000;
		 * std::string str = "hello";
		 *
		 * ObjectPack pack;
		 * pack.allocate(pack_size<std::uint64_t>() + pack_size(str));
		 *
		 * pack.write(i);
		 * pack.write(str);
		 *
		 * std::uint64_t unserial_i = pack.read<std::uint64_t>();
		 * std::string unserial_str;
		 * pack.read(unserial_str);
		 * ```
		 *
		 * Both serialization techniques can be used on all ObjectPacks, but
		 * only one serialization technique must be used on each ObjectPack.
		 *
		 * @note 
		 * The BasicObjectPack design is widely insprired by the
		 * `nlohmann::basic_json` class.
		 *
		 * @tparam S serializer implementation (e.g.: Serializer,
		 * LightSerializer, JsonSerializer...)
		 */
		template<template<typename> class S>
			class BasicObjectPack {
				friend base_io<BasicObjectPack<S>>;

				private:
				DataPack _data;

				std::size_t write_offset = 0;
				mutable std::size_t read_offset = 0;

				public:
				BasicObjectPack() = default;

				/**
				 * Serializes item in a new BasicObjectPack, using the
				 * S<T>::to_datapack(BasicObjectPack<S>&, const T&) specialization.
				 *
				 * @param item item to serialize
				 */
				template<typename T>
					BasicObjectPack(const T& item) {
						S<T>::to_datapack(*this, item);
					}

				/**
				 * Serializes item in the current BasicObjectPack, using the
				 * S<T>::to_datapack(BasicObjectPack<S>&, const T&) specialization.
				 *
				 * Existing data is discarded.
				 *
				 * @param item item to serialize
				 * @return reference to this BasicObjectPack
				 */
				template<typename T>
					BasicObjectPack<S>& operator=(const T& item) {
						S<T>::to_datapack(*this, item);
						return *this;
					}

				/**
				 * Returns a reference to the internal DataPack, where
				 * serialized data is stored.
				 */
				const DataPack& data() const {
					return _data;
				}

				/**
				 * Returns the current read cursor position, i.e. the index of
				 * the internal DataPack at which the next read() operation
				 * will start.
				 *
				 * @return read cursor position
				 */
				std::size_t readPos() const {
					return read_offset;
				}

				/**
				 * Places the current read cursor to the specified position.
				 *
				 * Can be used in conjonction with readPos() to perform several
				 * read() operation at the same offset.
				 *
				 * @param position new read cursor position
				 */
				void seekRead(std::size_t position = 0) const {
					read_offset = position;
				}

				/**
				 * Unserializes data from the current BasicObjectPack, using
				 * the S<T>::from_datapack(const BasicObjectPack<S>&) specialization.
				 *
				 * For this call to be valid, data must have been serialized in
				 * the BasicObjectPack using either the
				 * BasicObjectPack(const T&) constructor or operator=(const T&).
				 *
				 * @tparam T type of data to unserialize
				 * @return unserialized object
				 */
				template<typename T>
					T get() const {
						return S<T>::from_datapack(*this);
					}

				/**
				 * Allocates `size` bytes in the internal DataPack buffer to
				 * perform write() operations.
				 *
				 * @param size count of bytes to allocate
				 */
				void allocate(std::size_t size) {
					_data = {size, 1};
				}

				/**
				 * Writes the specified item to the internal DataPack buffer
				 * using a low level datapack::write() operation.
				 *
				 * A base_io specialization must be available for T.
				 *
				 * @param item item to write to this BasicObjectPack
				 */
				template<typename T>
					void write(const T& item) {
						datapack::write(this->_data, item, write_offset);
					}

				/**
				 * Constructs a new instance of T using the default constructor,
				 * and reads data into the new instance with read(T&).
				 *
				 * @return read data
				 */
				template<typename T>
					T read() const {
						T item;
						read(item);
						return item;
					}

				/**
				 * Reads an object from the internal DataPack buffer using a
				 * low level datapack::read() operation.
				 *
				 * A base_io specialization must be available for T.
				 *
				 * Data must have been serialize using a write(), and the
				 * read/write call order must be respected.
				 *
				 * @param item item into which data is read
				 */
				template<typename T>
					void read(T& item) const {
						datapack::read(this->_data, item, read_offset);
					}
				/**
				 * Constructs a new instance of T using the default constructor,
				 * and reads data into the new instance with check(T&).
				 *
				 * @return read data
				 */
				template<typename T>
					T check() const {
						T item;
						check(item);
						return item;
					}

				/**
				 * Calls read(T&) and replaces the internal read cursor at its
				 * original position.
				 *
				 * @param item item into which data is read
				 */
				template<typename T>
					void check(T& item) const {
						std::size_t pos = readPos();
						read(item);
						seekRead(pos);
					}

				/**
				 * Returns by move the internal DataPack buffer for
				 * memory efficiency.
				 *
				 * The current BasicObjectPack is left empty.
				 *
				 * @return internal DataPack buffer
				 */
				DataPack dump() {
					write_offset = 0;
					read_offset = 0;
					return std::move(_data);
				}

				/**
				 * Constructs a new BasicObjectPack from the input DataPack,
				 * the is moved into the BasicObjectPack instance, for memory
				 * efficiency.
				 *
				 * @param data_pack input datapack
				 * @return BasicObjectPack containing the input datapack
				 */
				static BasicObjectPack<S> parse(DataPack&& data_pack) {
					BasicObjectPack<S> pack;
					pack._data = std::move(data_pack);
					return pack;
				}

				/**
				 * Constructs a new BasicObjectPack from the input DataPack.
				 *
				 * @param data_pack input datapack
				 * @return BasicObjectPack containing the input datapack
				 */
				static BasicObjectPack<S> parse(const DataPack& data_pack) {
					BasicObjectPack<S> pack;
					pack._data = data_pack;
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
				 * @param object_pack destination BasicObjectPack
				 * @param offset `data_pack.buffer` index at which the first
				 * byte is read
				 */
				static void read(
						const DataPack& data_pack, BasicObjectPack<S>& object_pack, std::size_t& offset) {
					datapack::read(data_pack, object_pack._data, offset);
				}

			};

		/**
		 * Default serializer implementation, based on the base_io
		 * specialization for the type T.
		 *
		 * @tparam T data type
		 */
		template<typename T>
			struct Serializer {
				/**
				 * Serializes `item` to `pack` using the base_io serialization
				 * technique of the BasicObjectPack.
				 *
				 * @param pack destination pack
				 * @param item item to serialize
				 */
				template<typename PackType>
					static void to_datapack(PackType& pack, const T& item) {
						pack.allocate(pack_size(item));
						pack.write(item);
					}

				/**
				 * Deserializes data from `pack` using the base_io
				 * serialization technique of the BasicObjectPack.
				 *
				 * @param pack source pack
				 * @return deserialized data
				 */
				template<typename PackType>
					static T from_datapack(const PackType& pack) {
						return pack.template read<T>();
					}
			};

		/**
		 * A serializer implementation based on json serialization.
		 *
		 * Objects of type T are serialized using the nlohmann::json
		 * library. Json strings are then written and read directly to
		 * DataPack buffers.
		 *
		 * More particularly, the type T must be serializable into the provided
		 * `JsonType`, that is itself based on `nlohmann::basic_json`. For
		 * example, if `JsonType` is `nlohmann::json` (as defined by default in
		 * fpmas::communication::TypedMpi), the classical nlohmann::json custom
		 * serialization rules must be provided as explained at
		 * https://github.com/nlohmann/json#arbitrary-types-conversions.
		 *
		 * An fpmas::io::json::light_serializer must be provided for T when
		 * `JsonType` is fpmas::io::json::light_json. See fpmas::io::json for
		 * more information.
		 *
		 *
		 * @tparam T type to serialize
		 * @tparam JsonType json type used for serialization
		 * (nlohmann::json of fpmas::io::json::light_json).
		 */
		template<typename T, typename JsonType>
			struct BasicJsonSerializer {
				/**
				 * Serializes `data` to `pack`.
				 *
				 * An std::string instance is produced using
				 * `JsonType(data).dump()` and written to `pack`.
				 *
				 * @param pack destination pack
				 * @param data data to serialize
				 */
				template<typename PackType>
					static void to_datapack(PackType& pack, const T& data) {
						std::string json_str = JsonType(data).dump();
						pack.allocate(pack_size(json_str));
						pack.write(json_str);
					}

				/**
				 * Unserializes data from `pack`.
				 *
				 * A json std::string is retrieved from the pack and parsed
				 * using `JsonType::parse()`, and an instance of T is
				 * unserialized using the `JsonType::get()` method.
				 *
				 * @param pack source pack
				 * @return unserialized data
				 */
				template<typename PackType>
					static T from_datapack(const PackType& pack) {
						return JsonType::parse(
								pack.template read<std::string>()
								).template get<T>();
					}
			};

		/**
		 * An nlohmann::json based serializer.
		 *
		 * See https://github.com/nlohmann/json#arbitrary-types-conversions to
		 * learn how to define rules to serialize **any custom type** with the
		 * nlohmann::json library.
		 *
		 * @tparam T type to serialize as an nlohmann::json instance
		 */
		template<typename T>
			using JsonSerializer = BasicJsonSerializer<T, nlohmann::json>;
		/**
		 * An nlohmann::json based BasicObjectPack.
		 */
		typedef BasicObjectPack<JsonSerializer> JsonPack;

		/**
		 * An fpmas::io::json::light_json based serialized. Can be
		 * considered as the _light_ version of JsonSerializer.
		 */
		template<typename T>
			using LightJsonSerializer = BasicJsonSerializer<T, fpmas::io::json::light_json>;
		/**
		 * An fpmas::io::json::light_json based BasicObjectPack. Can be
		 * considered as the _light_ version of JsonPack.
		 */
		typedef BasicObjectPack<LightJsonSerializer> LightJsonPack;

		/**
		 * std::vector Serializer specialization.
		 *
		 * @tparam T vector items type
		 */
		template<typename T>
			struct Serializer<std::vector<T>> {
				/**
				 * Serializes `vec` to `pack`. Each item in `vec` is serialized
				 * using PackType(item), i.e. with the Serializer serialization
				 * technique of the BasicObjectPack.
				 *
				 * @param pack destination pack
				 * @param vec vector to serialize
				 */
				template<typename PackType>
					static void to_datapack(PackType& pack, const std::vector<T>& vec) {
						std::vector<PackType> packs(vec.size());

						for(std::size_t i = 0; i < vec.size(); i++) {
							// Serialize
							packs[i] = PackType(vec[i]);
						}
						pack.allocate(datapack::pack_size(packs));
						pack.write(packs);
					}

				/**
				 * Unserializes a vector from `pack`.
				 *
				 * Each item of the vector is unserialized using the
				 * PackType::get() method, i.e. with the Serializer serialization
				 * technique of the BasicObjectPack.
				 *
				 * @param pack source pack
				 * @return unserialized vector
				 */
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

		/**
		 * BasicObjectPack specialization based on the regular binary
		 * fpmas::io::datapack::Serializer.
		 *
		 * An ObjectPack can be seen as a `json` or `YAML` object that could
		 * be defined in other libraries, using other serialization techniques.
		 */
		typedef BasicObjectPack<Serializer> ObjectPack;

		template<typename T> struct LightSerializer;

		/**
		 * LightSerializer based BasicObjectPack specialization.
		 *
		 * Light version of the regular ObjectPack.
		 */
		typedef BasicObjectPack<LightSerializer> LightObjectPack;

		/**
		 * Light version of the fpmas::io::datapack::Serializer.
		 *
		 * By default, falls back to the Serializer<T> specialization, but
		 * custom LightSerializer specializations can be defined.
		 */
		template<typename T>
			struct LightSerializer {
				/**
				 * Serializes `data` into `pack` using the regular
				 * Serializer<T>::to_datapack() specialization.
				 *
				 * @param pack destination pack
				 * @param data data to serialize
				 */
				static void to_datapack(LightObjectPack& pack, const T& data) {
					Serializer<T>::to_datapack(pack, data);
				}

				/**
				 * Unserializes data from `pack`.
				 *
				 * Data is unserialized using the regular
				 * Serializer<T>::from_datapack() specialization.
				 *
				 * @param pack source pack
				 * @return deserialized data
				 */
				static T from_datapack(const LightObjectPack& pack) {
					return Serializer<T>::from_datapack(pack);
				}
			};
	}}
}
#endif
