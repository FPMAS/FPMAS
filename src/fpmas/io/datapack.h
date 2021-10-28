#ifndef FPMAS_IO_DATAPACK_H
#define FPMAS_IO_DATAPACK_H

#include "fpmas/api/communication/communication.h"

namespace fpmas { namespace io { namespace datapack {
	using fpmas::api::communication::DataPack;

	template<typename T>
		struct base_io {
			static std::size_t pack_size() {
				return sizeof(T);
			}

			static std::size_t pack_size(const T& data) {
				return sizeof(T);
			}

			static void write(DataPack& data_pack, const T& data, std::size_t& offset) {
				std::memcpy(&data_pack.buffer[offset], &data, sizeof(T));
				offset += sizeof(T);
			}

			static void read(const DataPack& data_pack, T& data, std::size_t& offset) {
				std::memcpy(&data, &data_pack.buffer[offset], sizeof(T));
				offset += sizeof(T);
			}
		};

	// Helper methods
	template<typename T>
		std::size_t pack_size() {
			return base_io<T>::pack_size();
		}
	template<typename T>
		std::size_t pack_size(const T& data) {
			return base_io<T>::pack_size(data);
		}
	template<typename T>
		std::size_t pack_size(std::size_t n) {
			return base_io<T>::pack_size(n);
		}

	template<>
		struct base_io<DistributedId> {
			static std::size_t pack_size();
			static std::size_t pack_size(const DistributedId&);

			static void write(
					DataPack& data_pack, const DistributedId& id, std::size_t& offset);
			static void read(
					const DataPack& data_pack, DistributedId& id, std::size_t& offset);
		};

	template<typename T>
		void write(DataPack& data_pack, const T& data, std::size_t& offset) {
			base_io<T>::write(data_pack, data, offset);
		}
	template<typename T>
		void read(const DataPack& data_pack, T& data, std::size_t& offset) {
			base_io<T>::read(data_pack, data, offset);
		}

	template<>
		struct base_io<std::string> {
			static std::size_t pack_size(const std::string& str);

			static void write(
					DataPack& data_pack, const std::string& data, std::size_t& offset);
			static void read(
					const DataPack& data_pack, std::string& str, std::size_t& offset);
		};

	template<typename T>
		struct base_io<std::vector<T>> {
			static std::size_t pack_size(const std::vector<T>& vec);
			static std::size_t pack_size(std::size_t n);

			static void write(
					DataPack& data_pack, const std::vector<T>& vec,
					std::size_t& offset);
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
		std::size_t base_io<std::vector<T>>::pack_size(std::size_t n) {
			return datapack::pack_size<std::size_t>()
				+ n * datapack::pack_size<T>();
		}

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

	template<>
		struct base_io<DataPack> {
			static std::size_t pack_size(const DataPack& data);

			static void write(
					DataPack& dest, const DataPack& source, std::size_t& offset);
			static void read(
					const DataPack& source, DataPack& dest, std::size_t& offset);
		};

	template<template<typename> class S>
		class ObjectPack {
			friend base_io<ObjectPack<S>>;

			private:
			DataPack data;

			mutable std::size_t write_offset = 0;
			mutable std::size_t read_offset = 0;

			public:
			ObjectPack() = default;

			template<typename T>
				ObjectPack(const T& item)
				: ObjectPack(S<T>::template to_datapack<ObjectPack<S>>(item)) {
				}

			template<typename T>
				ObjectPack<S>& operator=(const T& item) {
					*this = S<T>::template to_datapack<ObjectPack<S>>(item);
					return *this;
				}

			template<typename T>
				T get() const {
					return S<T>::from_datapack(*this);
				}

			void allocate(std::size_t size) {
				data = {(int) size, 1};
			}

			template<typename T>
				void write(const T& item) {
					datapack::write(this->data, item, write_offset);
				}

			template<typename T>
				T read() const {
					T item;
					read(item);
					return item;
				}

			template<typename T>
				void read(T& item) const {
					datapack::read(this->data, item, read_offset);
				}

			DataPack dump() const {
				write_offset = 0;
				return std::move(data);
			}

			static ObjectPack<S> parse(DataPack&& data_pack) {
				ObjectPack<S> pack;
				pack.data = std::move(data_pack);
				return pack;
			}
		};

	template<template<typename> class S>
		struct base_io<ObjectPack<S>> {
			static std::size_t pack_size(const ObjectPack<S>& pack) {
				return datapack::pack_size(pack.data);
			}

			static void write(
					DataPack& data_pack, const ObjectPack<S>& pack, std::size_t& offset) {
				datapack::write(data_pack, pack.data, offset);
			}

			static void read(
					const DataPack& data_pack, ObjectPack<S>& pack, std::size_t& offset) {
				datapack::read(data_pack, pack.data, offset);
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

}}}
#endif
