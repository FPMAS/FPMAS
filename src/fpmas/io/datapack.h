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
		struct base_io<std::vector<DataPack>> {
			static std::size_t pack_size(const std::vector<DataPack>& vec);

			static void write(
					DataPack& data_pack, const std::vector<DataPack>& data, std::size_t& offset);
			static void read(
					const DataPack& data_pack, std::vector<DataPack>& data, std::size_t& offset);
		};

	template<typename T>
		struct Serializer {
			static DataPack to_datapack(const T& item) {
				DataPack pack (pack_size(item), 1);
				std::size_t offset = 0;
				write(pack, item, offset);
				return pack;
			}

			static T from_datapack(const DataPack& pack) {
				T item;
				std::size_t offset = 0;
				read(pack, item, offset);
				return item;
			}
		};

	template<typename T>
		struct Serializer<std::vector<T>> {
			static DataPack to_datapack(const std::vector<T>& data) {
				std::vector<DataPack> data_pack(data.size());

				for(std::size_t i = 0; i < data.size(); i++) {
					// Serialize
					data_pack[i] = Serializer<T>::to_datapack(data[i]);
				}

				DataPack total_data_pack(io::datapack::pack_size(data_pack), 1);
				std::size_t offset = 0;
				io::datapack::write(total_data_pack, data_pack, offset);

				return {total_data_pack};
			}

			static std::vector<T> from_datapack(const DataPack& pack) {
				std::vector<DataPack> data_packs;
				std::size_t offset = 0;
				io::datapack::read(pack, data_packs, offset);

				std::vector<T> data;
				for(std::size_t i = 0; i < data_packs.size(); i++) {
					data.emplace_back(Serializer<T>::from_datapack(data_packs[i]));
				}
				return data;
			}
		};

}}}
#endif
