#ifndef FPMAS_IO_DATAPACK_H
#define FPMAS_IO_DATAPACK_H

/** \file src/fpmas/io/datapack.h
 * Binary DataPack based serialization features.
 */

#include <string>
#include <set>
#include <list>
#include <deque>

#include "fpmas/api/communication/communication.h"
#include "../api/utils/ptr_wrapper.h"

namespace fpmas { namespace io { namespace datapack {
	using fpmas::api::communication::DataPack;
	using api::utils::PtrWrapper;

	/**
	 * Void Serializer definition. Specializations should implement the
	 * following methods:
	 *
	 * - `static std::size_t size(const ObjectPack& p, const T& data)`
	 *
	 *   Specifies the buffer size required to serialize `data` into `p`.
	 *
	 * - `static void to_datapack(ObjectPack& p, const T& data)`
	 *
	 *   Serializes `data` into `p`, typically using the put() method.
	 *
	 * - `static T to_datapack(const ObjectPack& p)`
	 *
	 *   Unserializes `data` from `p`, typically using the get() method.
	 *
	 * The following method can be optionally implemented, if the buffer size
	 * required to serialize an instance of T does not depend on the instance
	 * of T itself (e.g. `sizeof(int) + sizeof(char)`):
	 *
	 * - `static std::size_t size(const ObjectPack& p)`
	 *
	 *   Specifies the buffer size required to serialize any instance of T into
	 *   `p`. This enables the BasicObjectPack::size<T>() method usage for the
	 *   type T.
	 *
	 * Custom serializers can also be implemented. In this case, `ObjectPack`
	 * arguments must be replaced by `BasicObjectPack<S>` (or any equivalent
	 * typedef), where S is the currently implemented serializer (e.g.
	 * LightSerializer).
	 *
	 * @tparam T data type
	 * @tparam Enable SFINAE condition
	 */
	template<typename T, typename Enable = void>
		struct Serializer {
		};

	/**
	 * Base object used to implement the FPMAS binary serialization
	 * technique.
	 *
	 * The purpose of the BasicObjectPack is to provide an high level API
	 * to efficiently read/write data from/to a low level
	 * fpmas::api::communication::DataPack.
	 *
	 * The BasicObjectPack is the _container_ that provides access to the
	 * serialized data. Serialization / deserialization is ensured by the
	 * provided serializer `S` specializations.
	 *
	 * The BasicObjectPack provides two serialization techniques.
	 *
	 * 1. The Serializer technique is used by BasicObjectPack(const T&),
	 * operator=(const T&), put() and get(). In this case, the
	 * `S<T>::%to_datapack()` and `S<T>::%from_datapack()` methods are used to
	 * perform those serializations, so the corresponding `S<T>`
	 * specializations must be available.
	 *
	 * 2. The native `std::memcpy` technique is used by read() and write().
	 * This method is only valid for members that can safely be copied as raw
	 * binary data, i.e. scalar types or associated arrays.
	 *
	 * @note
	 * Examples presented here are based on the default ObjectPack
	 * implementation, i.e. `BasicObjectPack<Serializer>`, however the same
	 * considerations are valid for any custom Serializer implementation passed
	 * as the `S` template parameter.
	 *
	 * @section allocate_put_get allocate(), put() and get()
	 *
	 * A BasicObjectPack can be manually allocated using the allocate() and
	 * size() methods. put() and get() can then be used to serialize and
	 * unserialize data using the Serializer specializations. Notice that
	 * specializations are already provided for fundamental types and other C++
	 * standard types and containers.
	 *
	 * ```cpp
	 * #include "fpmas/io/datapack.h"
	 *
	 * using namespace fpmas::io::datapack;
	 *
	 * int main() {
	 * 	std::vector<int> vec {4, -12, 17};
	 * 	std::string str = "Hello";
	 *
	 * 	ObjectPack pack;
	 * 	pack.allocate(pack.size(vec) + pack.size(str));
	 * 	pack.put(vec);
	 * 	pack.put(str);
	 *
	 * 	std::cout << pack << std::endl;
	 * 	std::cout << "vec: ";
	 * 	for(auto& item : pack.get<std::vector<int>>())
	 * 		std:cout << item << " ";
	 * 	std::cout << std::endl;
	 * 	std::cout << "str: " << pack.get<std::string>() << std::endl;
	 * }
	 * ```
	 * Expected output:
	 * ```
	 * pack: 0x3 0 0 0 0 0 0 0 0x4 0 0 0 0xfff4 0xffff 0xffff 0xffff 0x11 0 0 0 0x5 0 0 0 0 0 0 0 0x48 0x65 0x6c 0x6c 0x6f
	 * vec: 4 -12 17
	 * str: Hello
	 * ```
	 *
	 * The interest of the BasicObjectPack serialization technique is that the
	 * buffer can be allocated in a **single std::malloc** operation even for
	 * very complex data schemes, what is very efficient in terms of memory
	 * usage and execution time.
	 *
	 * @section constructor_and_eq_operator Constructor and operator=
	 *
	 * The allocation operator can be performed automatically using the
	 * BasicObjectPack(const T&) constructor or the operator=(const T&).
	 *
	 * ```cpp
	 * #include "fpmas/io/datapack.h"
	 * 
	 * int main() {
	 * 	std::string str = "Hello world";
	 * 	ObjectPack pack = str;
	 *
	 * 	std::cout << "pack: " << pack << std::endl;
	 * 	std::cout << "str: " << pack.get<std::string>() << std::endl;
	 * }
	 * ```
	 * Expected output:
	 * ```
	 * pack: 0xb 0 0 0 0 0 0 0 0x48 0x65 0x6c 0x6c 0x6f 0x20 0x77 0x6f 0x72 0x6c 0x64
	 * str: Hello world
	 * ```
	 * The disadvantage of this method is that nothing can be added _a priori_
	 * to the BasicObjectPack once it as been assigned. But this method can be
	 * efficiently combined with put() and get() to define custom data
	 * serialization rules.
	 *
	 * @section custom_serializer Custom Serializer
	 *
	 * Custom serialization rules can be specified thanks to Serializer
	 * specializations. For any type T, the static methods specified in the
	 * Serializer documentation must be implemented.
	 *
	 * ```cpp
	 * #include "fpmas/io/datapack.h"
	 *
	 * using namespace fpmas::io::datapack;
	 *
	 * struct Data {
	 * 	std::unordered_map<int, std::string> map {{4, "hey"}, {2, "ho"}};
	 * 	double x = 127.4;
	 * };
	 *
	 * namespace fpmas { namespace io { namespace datapack {
	 * 	template<>
	 * 	struct Serializer<Data> {
	 * 		static std::size_t size(const ObjectPack& pack, const Data& item) {
	 * 			// Computes the ObjectPack size required to serialize data
	 * 			return pack.size(item.map) + pack.size(item.x);
	 * 		}
	 *
	 * 		static void to_datapack(ObjectPack& pack, const Data& item) {
	 * 			// The buffer is pre-allocated, so there is no need to call
	 * 			// allocate()
	 * 			pack.put(item.map);
	 * 			pack.put(item.x);
	 * 		}
	 *
	 * 		static Data from_datapack(const ObjectPack& pack) {
	 * 			Data item;
	 * 			item.map = pack.get<std::unordered_map<int, std::string>>();
	 * 			item.x = pack.get<double>();
	 * 			return item;
	 * 		}
	 * 	};
	 * }}}
	 *
	 * int main() {
	 * 	Data item;
	 * 	// The required size is automatically allocated thanks to the custom
	 * 	// Serializer<Data>::size() method.
	 * 	ObjectPack pack = item;
	 * 	std::cout << "pack: " << pack << std::endl;
	 * 	Data unserial_item = pack.get<Data>();
	 * 	std::cout << "item.map: ";
	 * 	for(auto& item : unserial_item.map)
	 * 		std::cout << "(" << item.first << "," << item.second << ") ";
	 * 	std::cout << std::endl;
	 * 	std::cout << "item.x: " << unserial_item.x << std::endl;
	 * }
	 * ```
	 *
	 * Expected output:
	 * ```
	 * pack: 0x2 0 0 0 0 0 0 0 0x2 0 0 0 0x2 0 0 0 0 0 0 0 0x68 0x6f 0x4 0 0 0 0x3 0 0 0 0 0 0 0 0x68 0x65 0x79 0xff9a 0xff99 0xff99 0xff99 0xff99 0xffd9 0x5f 0x40
	 * item.map: (4,hey) (2,ho)
	 * item.x: 127.4
	 * ```
	 * Once a Serializer specialization has been specified for a type T, it can
	 * then be implicitly used to serialize compound types that use T. For
	 * example `std::vector<Data>` can be serialized. But even in this case,
	 * an `ObjectPack pack = std::vector<Data>(...)` operation only
	 * performs a **single** contiguous memory allocation, thanks to
	 * the `size()` method trick.
	 *
	 * @section write_and_read write() and read() operations
	 *
	 * An read() / write() serialization technique can be used for types that
	 * can be directly copied using an
	 * [std::memcpy](https://en.cppreference.com/w/cpp/string/byte/memcpy)
	 * operation. This is a low level feature that is not required in general
	 * use cases.
	 *
	 * ```cpp
	 * #include "fpmas/io/datapack.h"
	 *
	 * using namespace fpmas::io::datapack;
	 *
	 * int main() {
	 * 	std::uint64_t i = 23000;
	 * 	char str[6] = "abcde";
	 *
	 * 	ObjectPack pack;
	 * 	pack.allocate(sizeof(i) + sizeof(str));
	 * 	pack.write(i);
	 * 	pack.write(str, 3);
	 *
	 * 	std::cout << "pack: " << pack << std::endl;
	 *
	 * 	std::uint64_t u_i;
	 * 	char u_str[3];
	 * 	pack.read(u_i);
	 * 	pack.read(u_str, 3);
	 *
	 * 	std::cout << "u_i: " << u_i << std::endl;
	 * 	std::cout << "u_str: ";
	 * 	for(std::size_t i = 0; i < 3; i++)
	 * 		std::cout << u_str[i];
	 * 	std::cout << std::endl;
	 * }
	 * ```
	 * Expected output:
	 * ```
	 * pack: 0xffd8 0x59 0 0 0 0 0 0 0x61 0x62 0x63 0 0 0
	 * u_i: 23000
	 * u_str: abc
	 * ```
	 *
	 * @note 
	 * The BasicObjectPack design is widely insprired by the
	 * `nlohmann::basic_json` class.
	 *
	 * @tparam S serializer implementation (e.g.: Serializer,
	 * LightSerializer, JsonSerializer...)
	 */
	template<template<typename, typename Enable = void> class S>
		class BasicObjectPack {
			friend Serializer<std::string>;
			friend Serializer<BasicObjectPack<S>>;

			private:
			DataPack _data;
			std::size_t write_offset = 0;
			mutable std::size_t read_offset = 0;

			public:
			BasicObjectPack() = default;

			/**
			 * Serializes item in a new BasicObjectPack.
			 *
			 * The BasicObjectPack pack is allocated in a single operation,
			 * calling `allocate(S<T>::%size(*this, item))`.
			 *
			 * The data is then serialized into the BasicObjectPack using the
			 * S<T>::to_datapack(BasicObjectPack<S>&, const T&) specialization.
			 *
			 * @param item item to serialize
			 */
			template<typename T>
				BasicObjectPack(const T& item) {
					std::size_t count = S<T>::size(*this, item); 
					this->allocate(count);
					S<T>::to_datapack(*this, item);
					write_offset+=count;
				}

			/**
			 * Serializes item in the current BasicObjectPack.
			 *
			 * Existing data is discarded, and the BasicObjectPack pack is
			 * reallocated in a single operation, calling
			 * `allocate(S<T>::%size(*this, item))`.
			 *
			 * The data is then serialized into the BasicObjectPack using the
			 * S<T>::to_datapack(BasicObjectPack<S>&, const T&) specialization.
			 *
			 * @param item item to serialize
			 * @return reference to this BasicObjectPack
			 */
			template<typename T>
				BasicObjectPack<S>& operator=(const T& item) {
					this->allocate(S<T>::size(*this, item));
					S<T>::to_datapack(*this, item);
					return *this;
				}

			/**
			 * Returns the buffer size required to serialize any instance of T.
			 *
			 * This is usable only if the
			 * `S<T>::%size(const BasicObjectPack<S>&)`
			 * specialization is available.
			 *
			 * This method can be used only when the required buffer size does
			 * not depend on the instance of T to serialize. For example,
			 * `size<std::uint64_t>()` might always return 64, independently of
			 * the integer to serialize.
			 *
			 * @return buffer size required to serialize any instance of T
			 */
			template<typename T>
				std::size_t size() const {
					return S<T>::size(*this);
				}

			/**
			 * Returns the buffer size required to serialize an instance of T.
			 *
			 * This method returns the result of the
			 * `S<T>::%size(const BasicObjectPack<S>&, const T&)`
			 * specialization.
			 *
			 * @param item item to serialize
			 * @return buffer size required to serialize an instance of T
			 */
			template<typename T>
				std::size_t size(const T& item) const {
					return S<T>::size(*this, item);
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
			std::size_t readOffset() const {
				return read_offset;
			}

			/**
			 * Places the current read cursor to the specified offset.
			 *
			 * Can be used in conjunction with readOffset() to perform several
			 * read() operation at the same offset.
			 *
			 * @param offset new read cursor position
			 */
			void seekRead(std::size_t offset = 0) const {
				read_offset = offset;
			}

			/**
			 * Returns the current write cursor position, i.e. the index of
			 * the internal DataPack at which the next write() operation
			 * will start.
			 *
			 * @return write cursor position
			 */
			std::size_t writeOffset() const {
				return write_offset;
			}

			/**
			 * Places the current write cursor to the specified offset.
			 *
			 * Can be used in conjunction with writeOffset() to override a
			 * previous write() operation.
			 *
			 * @param offset new read cursor position
			 */
			void seekWrite(std::size_t offset = 0) {
				write_offset = offset;
			}

			/**
			 * Serializes `item` into the current BasicObjectPack using the
			 * S<T>::to_datapack(BasicObjectPack<S>&, const T&) specialization.
			 *
			 * @param item item to serialize
			 */
			template<typename T>
				void put(const T& item) {
					S<T>::to_datapack(*this, item);
				}

			/**
			 * Unserializes data from the current BasicObjectPack, using
			 * the S<T>::from_datapack(const BasicObjectPack<S>&) specialization.
			 *
			 * @tparam T type of data to unserialize
			 * @return unserialized object
			 */
			template<typename T>
				T get() const {
					T item = S<T>::from_datapack(*this);
					return item;
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
			 * Low level write operation of a scalar value or associated array
			 * into the internal buffer.
			 *
			 * A call to this method assumes that the internal buffer as been
			 * allocated so that `sizeof(T)` bytes are available from the
			 * current writeOffset(). `item` is then copied using an
			 * `std::memcpy` operation, and the internal writeOffset() is
			 * incremented by `sizeof(T)`.
			 *
			 * @param item item to write
			 */
			template<typename T>
				void write(const T& item) {
					std::size_t count = sizeof(T);
					std::memcpy(&_data.buffer[write_offset], &item, count);
					write_offset+=count;
				}

			/**
			 * Low level write operation of `count` bytes from `input_data`
			 * into the internal buffer.
			 *
			 * A call to this method assumes that the internal buffer as been
			 * allocated so that `count` bytes are available from the current
			 * writeOffset(). Data in the range `[input_data[0],
			 * input_data[count])` is then copied using an `std::memcpy`
			 * operation, and the internal writeOffset() is incremented by
			 * `count`.
			 *
			 * @param input_data input buffer to copy data from
			 * @param count count of bytes to copy
			 */
			void write(const void* input_data, std::size_t count) {
					std::memcpy(&_data.buffer[write_offset], input_data, count);
					write_offset+=count;
			}

			/**
			 * Low level read operation of a scalar value or associated array
			 * from the internal buffer.
			 *
			 * A call to this method assumes that `sizeof(T)` bytes are
			 * available in the internal buffer from the current readOffset().
			 * Data is directly copied to `&item` using an `std::memcpy`
			 * operation, and the internal readOffset() is incremented by
			 * `sizeof(T)`.
			 *
			 * @param item item to write
			 */
			template<typename T>
				void read(T& item) const {
					std::size_t count = sizeof(T);
					std::memcpy(&item, &_data.buffer[read_offset], count);
					read_offset+=count;
				}

			/**
			 * Low level read operation of `count` bytes into `output_data`
			 * from the internal buffer.
			 *
			 * A call to this method assumes that `count` bytes are available
			 * in the internal buffer from the current readOffset(), and that
			 * `output_data` is properly allocated to receive `count` bytes.
			 * Data is directly copied to `output_data` using an `std::memcpy`
			 * operation, and the internal readOffset() is incremented by
			 * `count`.
			 *
			 * @param output_data input buffer to copy data from
			 * @param count count of bytes to copy
			 */
			void read(void* output_data, std::size_t count) const {
				std::memcpy(output_data, &_data.buffer[read_offset], count);
				read_offset+=count;
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
					std::size_t pos = readOffset();
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

			/**
			 * Extracts `size` bytes from the current BasicObjectPack pack and
			 * return them as a new BasicObjectPack.
			 *
			 * The read cursor is incremented by `size`.
			 *
			 * This can be useful to extract a value from the current
			 * BasicObjectPack for later unserialization.
			 *
			 * @param size byte count to extract
			 * @return extracted BasicObjectPack
			 */
			BasicObjectPack<S> extract(std::size_t size) const {
				BasicObjectPack<S> pack;
				pack.allocate(size);
				std::memcpy(
						pack._data.buffer,
						&this->_data.buffer[this->read_offset],
						size
						);
				this->read_offset+=size;
				return pack;
			}
		};

	 /**
	  * Serializer specialization that allows nested BasicObjectPack
	  * serialization.
	  *
	  * | Serialization Scheme ||
	  * |----------------------||
	  * | pack.data().size | pack.data().buffer |
	  */
	template<template<typename, typename> class S>
		struct Serializer<BasicObjectPack<S>> {

			/**
			 * Returns the buffer size required to serialize the `child` data
			 * pack into any PackType.
			 *
			 * @return `sizeof(std::size_t) + child.data().size`
			 */
			template<typename PackType>
				static std::size_t size(
						const PackType&, const BasicObjectPack<S>& child) {
					return sizeof(std::size_t) + child._data.size;
				}

			/**
			 * Writes `child` pack to the `parent` pack buffer.
			 *
			 * @param parent destination BasicObjectPack
			 * @param child source BasicObjectPack 
			 */
			template<typename PackType>
				static void to_datapack(
						PackType& parent, const BasicObjectPack<S>& child) {
					parent.write(child._data.size);
					parent.write(child._data.buffer, child._data.size);
				}

			/**
			 * Reads a BasicObjectPack from the `parent` pack buffer. The count
			 * of bytes read depends on the corresponding to_datapack()
			 * operation.
			 *
			 * @param parent source DataPack
			 * @return read BasicObjectPack
			 */
			template<typename PackType>
				static BasicObjectPack<S> from_datapack(const PackType& parent) {
					std::size_t size;
					parent.read(size);

					BasicObjectPack<S> child;
					child.allocate(size);
					parent.read(child._data.buffer, size);
					return child;
				}
		};

	/**
	 * Serializer implementation only available for fundamental types.
	 *
	 * | Serialization Scheme |
	 * |----------------------|
	 * | raw binary value     |
	 *
	 * @tparam T fundamental type (std::size_t, std::int32_t, unsigned int...)
	 */
	template<typename T>
		struct Serializer<T, typename std::enable_if<std::is_fundamental<T>::value>::type> {
			/**
			 * Returns the buffer size required to serialize an instance of T.
			 *
			 * This fundamental type implementation returns `sizeof(T)`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType&) {
					return sizeof(T);
				}

			/**
			 * Equivalent to size(const PackType&).
			 */
			template<typename PackType>
				static std::size_t size(const PackType&, const T&) {
					return sizeof(T);
				}


			/**
			 * Writes `item` to the `pack` buffer using a `pack.write()`
			 * operation.
			 *
			 * @param pack destination BasicObjectPack
			 * @param item source data
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const T& item) {
					pack.write(item);
				}

			/**
			 * Reads a instance of T from the `pack` buffer using a
			 * `pack.read()` operation.
			 *
			 * @param pack source BasicObjectPack
			 * @return read T instance
			 */
			template<typename PackType>
				static T from_datapack(const PackType& pack) {
					T item;
					pack.read(item);
					return item;
				}
		};

	/**
	 * std::string Serializer specialization.
	 *
	 * | Serialization Scheme ||
	 * | str.size() | str.data() (char array) ||
	 */
	template<>
		struct Serializer<std::string> {
			/**
			 * Returns the buffer size, in bytes, required to serialize an
			 * std::string instance, i.e. `sizeof(std::size_t)+str.size()`.
			 *
			 * Notice that this does **not** depend upon the current PackType,
			 * since the std::string is directly copied to the internal
			 * PackType DataPack buffer.
			 */
			template<typename PackType>
				static std::size_t size(const PackType&, const std::string& str) {
					return sizeof(std::size_t) + str.size();
				}

			/**
			 * Writes `str` to the `pack` buffer.
			 *
			 * @param pack destination BasicObjectPack
			 * @param str source string
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const std::string& str) {
					pack.write(str.size());
					pack.write(str.data(), str.size());
				}

			/**
			 * Reads an std::string from the `pack` buffer.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized std::string
			 */
			template<typename PackType>
				static std::string from_datapack(const PackType& pack) {
					std::size_t size;
					pack.read(size);

					std::string str;
					str.resize(size);
					pack.read(str.data(), size);
					return str;
				}
		};

	/**
	 * std::vector Serializer specialization.
	 *
	 * | Serialization Scheme |||||
	 * |----------------------|||||
	 * | vec.size() | item_1 | item_2 | ... | item_n |
	 */
	template<typename T>
		struct Serializer<std::vector<T>> {
			/**
			 * Returns the buffer size required to serialize `vec` into `p`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p, const std::vector<T>& vec) {
					std::size_t n = p.template size<std::size_t>();
					for(const T& item : vec)
						n += p.template size(item);
					return n;
				}

			/**
			 * Serializes `vec` to `pack`.
			 * 
			 * Each item is serialized using the PackType::put(const T&)
			 * method.
			 *
			 * @param pack destination BasicObjectPack
			 * @param vec vector to serialize
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const std::vector<T>& vec) {
					pack.template put(vec.size());
					for(const T& item : vec)
						pack.template put(item);
				}

			/**
			 * Unserializes a vector from `pack`.
			 *
			 * Each item of the vector is unserialized using the
			 * PackType::get<T>() method.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized vector
			 */
			template<typename PackType>
				static std::vector<T> from_datapack(const PackType& pack) {
					std::size_t size = pack.template get<std::size_t>();
					std::vector<T> data;
					for(std::size_t i = 0; i < size; i++)
						data.emplace_back(pack.template get<T>());
					return data;
				}
		};


	/**
	 * std::set Serializer specialization.
	 *
	 * | Serialization scheme |||||
	 * |----------------------|||||
	 * | set.size() | item_1 | item_2 | ... | item_n |
	 *
	 * @tparam T type of data contained into the set. A Serializer
	 * specialization of T must be available.
	 */
	template<typename T>
		struct Serializer<std::set<T>> {
			/**
			 * Returns the buffer size required to serialize `set` into `p`.
			 */
			template<typename PackType>
			static std::size_t size(const PackType& p, const std::set<T>& set) {
				std::size_t n = p.template size<std::size_t>();
				for(auto item : set)
					n += p.template size(item);
				return n;
			}

			/**
			 * Serializes `set` to `pack`.
			 * 
			 * Each item is serialized in the set order using the
			 * PackType::put(const T&) method.
			 *
			 * @param pack destination BasicObjectPack
			 * @param set set to serialize
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const std::set<T>& set) {
					pack.template put(set.size());
					// Items are written in order
					for(auto item : set)
						pack.template put(item);
				}

			/**
			 * Unserializes a set from `pack`.
			 *
			 * Each item of the vector is unserialized using the
			 * PackType::get<T>() method.
			 *
			 * Since items are already ordered, a linear complexity to build
			 * the set is ensured.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized vector
			 */
			template<typename PackType>
				static std::set<T> from_datapack(const PackType& pack) {
					std::size_t size = pack.template get<std::size_t>();
					std::set<T> set;
					auto current_hint = set.end();
					for(std::size_t i = 0; i < size; i++) {
						// Constant time insertion before end, since data is
						// already sorted.
						set.emplace_hint(current_hint, pack.template get<T>());
						// Updates hint
						current_hint = set.end();
					}
					return set;
				}
		};

	/**
	 * std::list Serializer specialization.
	 *
	 * | Serialization scheme |||||
	 * |----------------------|||||
	 * | list.size() | item_1 | item_2 | ... | item_n |
	 */
	template<typename T>
		struct Serializer<std::list<T>> {

			/**
			 * Returns the buffer size required to serialize `list` into `p`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p, const std::list<T>& list) {
					std::size_t n = p.template size<std::size_t>();
					for(auto item : list)
						n += p.template size(item);
					return n;
				}

			/**
			 * Serializes `list` to `pack`.
			 * 
			 * Each item is serialized using the PackType::put(const T&)
			 * method.
			 *
			 * @param pack destination BasicObjectPack
			 * @param list list to serialize
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const std::list<T>& list) {
					pack.template put(list.size());
					// Items are written in order
					for(auto item : list)
						pack. template put(item);
				}

			/**
			 * Unserializes a list from `pack`.
			 *
			 * Each item of the list is unserialized using the
			 * PackType::get<T>() method.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized list
			 */
			template<typename PackType>
				static std::list<T> from_datapack(const PackType& pack) {
					std::size_t size = pack.template get<std::size_t>();
					std::list<T> data;
					for(std::size_t i = 0; i < size; i++) {
						data.emplace_back(pack.template get<T>());
					}
					return data;
				}
		};

	/**
	 * std::deque Serializer specialization.
	 *
	 * | Serialization scheme |||||
	 * |----------------------|||||
	 * | deque.size() | item_1 | item_2 | ... | item_n |
	 */
	template<typename T>
		struct Serializer<std::deque<T>> {
			/**
			 * Returns the buffer size required to serialize `deque` into `p`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p, const std::deque<T>& deque) {
					std::size_t n = p.template size<std::size_t>();
					for(auto item : deque)
						n += p.template size(item);
					return n;
				}

			/**
			 * Serializes `deque` to `pack`.
			 * 
			 * Each item is serialized using the PackType::put(const T&)
			 * method.
			 *
			 * @param pack destination BasicObjectPack
			 * @param deque deque to serialize
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const std::deque<T>& deque) {
					pack.template put(deque.size());
					// Items are written in order
					for(auto item : deque)
						pack.template put(item);
				}

			/**
			 * Unserializes a deque from `pack`.
			 *
			 * Each item of the deque is unserialized using the
			 * PackType::get<T>() method.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized deque
			 */
			template<typename PackType>
				static std::deque<T> from_datapack(const PackType& pack) {
					std::size_t size = pack.template get<std::size_t>();
					std::deque<T> deque;
					for(std::size_t i = 0; i < size; i++) {
						deque.emplace_back(pack.template get<std::size_t>());
					}
					return deque;
				}
		};

	/**
	 * std::pair Serializer specialization.
	 *
	 * | Serialization scheme ||
	 * |----------------------||
	 * | pair.first | pair.second |
	 *
	 * @tparam T1 pair first type
	 * @tparam T2 pair second type
	 */
	template<typename T1, typename T2>
		struct Serializer<std::pair<T1, T2>> {
			/**
			 * Returns the buffer size, in bytes, required to serialize the
			 * specified pair, i.e.
			 * `p.size(pair.first)+p.size(pair.second)`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p, const std::pair<T1, T2>& pair) {
					return p.template size(pair.first)
						+ p.template size(pair.second);
				}

			/**
			 * Serializes `pair` to `pack`.
			 * 
			 * `pair.first` and `pair.second` are serialized using the
			 * PackType::put(const T1&) and PackType::put(const T2&) methods.
			 *
			 * @param pack destination BasicObjectPack
			 * @param pair pair to serialize
			 */
			template<typename PackType>
				static void to_datapack(
						PackType& pack, const std::pair<T1, T2>& pair) {
					pack.template put(pair.first);
					pack.template put(pair.second);
				}

			/**
			 * Unserializes a pair from `pack`.
			 *
			 * `pair.first` and `pair.second` are serialized using the
			 * PackType::get<T1>() and PackType::get<T2>() methods.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized pair
			 */
			template<typename PackType>
				static std::pair<T1, T2> from_datapack(const PackType& pack) {
					return {
						pack.template get<T1>(),
						pack.template get<T2>()
					};
				}
		};

	/**
	 * std::unordered_map Serializer specialization.
	 *
	 * | Serialization scheme ||||||||
	 * |----------------------||||||||
	 * | map.size() | item_1.key | item_1.value | item_2.key | item_2.value | ... | item_n.key | item_n.value |
	 *
	 * @tparam K key type
	 * @tparam T value type
	 */
	template<typename K, typename T>
		struct Serializer<std::unordered_map<K, T>> {
			/**
			 * Returns the buffer size required to serialize the specified map,
			 * i.e. `p.size<std::size_t>()+N` where `N` is the sum of
			 * `p.size(item)` for each item in `map`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p, const std::unordered_map<K, T>& map) {
					std::size_t n = p.template size<std::size_t>();
					for(auto& item : map)
						n += p.template size(item);
					return n;
				}

			/**
			 * Serializes `map` to `pack`.
			 * 
			 * Each item is serialized using the
			 * PackType::put(const std::pair<K, T>&) method.
			 *
			 * @param pack destination BasicObjectPack
			 * @param map map to serialize
			 */
			template<typename PackType>
				static void to_datapack(
						PackType& pack, const std::unordered_map<K, T>& map) {
					pack.template put(map.size());
					// Items are written in order
					for(auto& item : map)
						pack.template put(item);
				}

			/**
			 * Unserializes a map from `pack`.
			 *
			 * Each item of the map is unserialized using the
			 * PackType::get<std::pair<K, T>>() method.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized map
			 */
			template<typename PackType>
				static std::unordered_map<K, T> from_datapack(const PackType& pack) {
					std::size_t size = pack.template get<std::size_t>();
					std::unordered_map<K, T> map;
					for(std::size_t i = 0; i < size; i++)
						map.emplace(pack.template get<std::pair<K, T>>());
					return map;
				}
		};

	/**
	 * std::array Serializer specialization.
	 *
	 * | Serialization scheme ||||
	 * |----------------------||||
	 * | item_1 | item_2 | ... | item_n |
	 *
	 * @tparam T value type
	 * @tparam N array size
	 */
	template<typename T, std::size_t N>
		struct Serializer<std::array<T, N>> {
			/**
			 * Returns the buffer size required to serialize the specified
			 * array, i.e. the sum of `p.size(item)` for each item in `array`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p, const std::array<T, N>& array) {
					std::size_t n = 0;
					for(auto item : array)
						n += p.template size(item);
					return n;
				}

			/**
			 * Serializes `array` to `pack`.
			 * 
			 * Each item is serialized using the PackType::put(const T&)
			 * method.
			 *
			 * @param pack destination BasicObjectPack
			 * @param array array to serialize
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const std::array<T, N>& array) {
					for(auto item : array)
						pack.template put(item);
				}

			/**
			 * Unserializes an array from `pack`.
			 *
			 * Each item of the array is unserialized using the
			 * PackType::get<T>() method.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized array
			 */
			template<typename PackType>
				static std::array<T, N> from_datapack(const PackType& pack) {
					std::array<T, N> array;
					for(std::size_t i = 0; i < N; i++)
						array[i] = pack.template get<T>();
					return array;
				}
		};

	/**
	 * api::graph::DistributedId Serializer specialization.
	 *
	 * | Serialization Scheme ||
	 * | distributed_id.rank() | distributed_id.id() |
	 */
	template<>
		struct Serializer<DistributedId> {
			/**
			 * Returns the buffer size required to serialize a DistributedId
			 * instance, i.e. `p.size<int>()+p.size<FPMAS_ID_TYPE>`.
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p) {
					return
						p.template size<int>()
						+ p.template size<FPMAS_ID_TYPE>();
				}

			/**
			 * Equivalent to size().
			 */
			template<typename PackType>
				static std::size_t size(const PackType& p, const DistributedId&) {
					return size(p);
				}
			/**
			 * Serializes `id` to `pack`.
			 * 
			 * @param pack destination BasicObjectPack
			 * @param id DistributedId to serialize
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const DistributedId& id) {
					pack.template put(id.rank());
					pack.template put(id.id());
				}
			/**
			 * Unserializes a DistributedId from `pack`.
			 *
			 * @param pack source BasicObjectPack
			 * @return unserialized DistributedId
			 */
			template<typename PackType>
				static DistributedId from_datapack(const PackType& pack) {
					return {
						pack.template get<int>(), pack.template get<FPMAS_ID_TYPE>()
					};
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

	template<typename T, typename> struct LightSerializer;

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
	template<typename T, typename Enable = void>
		struct LightSerializer {
			/**
			 * Returns the buffer size required to serialize any instance of T
			 * into `p` using the regular
			 * Serializer<T>::size(const LightObjectPack&) method if it is
			 * available.
			 */
			static std::size_t size(const LightObjectPack& p) {
				return Serializer<T>::size(p);
			}

			/**
			 * Returns the buffer size required to serialize `item`
			 * into `p` using the regular
			 * Serializer<T>::size(const LightObjectPack&, const T&) method.
			 */
			static std::size_t size(const LightObjectPack& p, const T& item) {
				return Serializer<T>::size(p, item);
			}

			/**
			 * Serializes `item` into `pack` using the regular
			 * Serializer<T>::to_datapack() specialization.
			 *
			 * @param pack destination pack
			 * @param item item to serialize
			 */
			static void to_datapack(LightObjectPack& pack, const T& item) {
				Serializer<T>::to_datapack(pack, item);
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

	/**
	 * BasicObjectPack output stream operator specialization.
	 *
	 * The internal DataPack is written has hexadecimal values.
	 */
	template<template<typename, typename> class S>
		std::ostream& operator<<(std::ostream& o, const BasicObjectPack<S>& pack) {
			o.setf(std::ios_base::hex, std::ios_base::basefield);
			o.setf(std::ios_base::showbase);
			for(std::size_t i = 0; i < pack.data().size; i++)
				o << (unsigned short) pack.data().buffer[i] << " ";
			o.unsetf(std::ios_base::showbase);
			o.unsetf(std::ios_base::hex);
			return o;
		}
}}}
#endif
