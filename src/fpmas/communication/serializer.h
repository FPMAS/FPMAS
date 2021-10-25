#ifndef FPMAS_COMM_SERIALIZER_H
#define FPMAS_COMM_SERIALIZER_H

#include "fpmas/api/communication/communication.h"

namespace fpmas { namespace communication {
	using api::communication::DataPack;

	template<typename T>
		struct BaseSerializer {
			static std::size_t pack_size() {
				return sizeof(T);
			}

			static std::size_t pack_size(const T& data) {
				return sizeof(T);
			}

			static void serialize(DataPack& data_pack, const T& data, std::size_t& offset) {
				std::memcpy(&data_pack.buffer[offset], &data, sizeof(T));
				offset += sizeof(T);
			}

			static void deserialize(const DataPack& data_pack, T& data, std::size_t& offset) {
				std::memcpy(&data, &data_pack.buffer[offset], sizeof(T));
				offset += sizeof(T);
			}
		};

	// Helper methods
	template<typename T>
		std::size_t pack_size() {
			return BaseSerializer<T>::pack_size();
		}
	template<typename T>
		std::size_t pack_size(const T& data) {
			return BaseSerializer<T>::pack_size(data);
		}
	template<typename T>
		std::size_t pack_size(std::size_t n) {
			return BaseSerializer<T>::pack_size(n);
		}

	template<typename T>
		void serialize(DataPack& data_pack, const T& data, std::size_t& offset) {
			BaseSerializer<T>::serialize(data_pack, data, offset);
		}
	template<typename T>
		void deserialize(const DataPack& data_pack, T& data, std::size_t& offset) {
			BaseSerializer<T>::deserialize(data_pack, data, offset);
		}

	template<>
		struct BaseSerializer<DistributedId> {
			static std::size_t pack_size();
			static std::size_t pack_size(const DistributedId&);

			static void serialize(
					DataPack& data_pack, const DistributedId& id, std::size_t& offset);
			static void deserialize(
					const DataPack& data_pack, DistributedId& id, std::size_t& offset);
		};

	template<>
		struct BaseSerializer<std::string> {
			static std::size_t pack_size(const std::string& str);

			static void serialize(
					DataPack& data_pack, const std::string& data, std::size_t& offset);
			static void deserialize(
					const DataPack& data_pack, std::string& str, std::size_t& offset);
		};

	template<typename T>
		struct BaseSerializer<std::vector<T>> {
			static std::size_t pack_size(const std::vector<T>& vec);
			static std::size_t pack_size(std::size_t n);

			static void serialize(
					DataPack& data_pack, const std::vector<T>& vec,
					std::size_t& offset);
			static void deserialize(
					const DataPack& data_pack, std::vector<T>& vec,
					std::size_t& offset);
		};

	template<typename T>
		std::size_t BaseSerializer<std::vector<T>>::pack_size(const std::vector<T>& vec) {
			std::size_t size = communication::pack_size<std::size_t>();
			for(auto item : vec)
				size += communication::pack_size(item);
			return size;
		};

	template<typename T>
		std::size_t BaseSerializer<std::vector<T>>::pack_size(std::size_t n) {
			return communication::pack_size<std::size_t>()
				+ n * communication::pack_size<T>();
		}

	template<typename T>
		void BaseSerializer<std::vector<T>>::serialize(
				DataPack& data_pack, const std::vector<T>& vec,
				std::size_t& offset) {
			BaseSerializer<std::size_t>::serialize(data_pack, vec.size(), offset);
			for(auto item : vec)
				BaseSerializer<T>::serialize(data_pack, item, offset);
		}

	template<typename T>
		 void BaseSerializer<std::vector<T>>::deserialize(
				const DataPack& data_pack, std::vector<T>& vec,
				std::size_t& offset) {
			 std::size_t size;
			 BaseSerializer<std::size_t>::deserialize(data_pack, size, offset);
			 vec.resize(size);
			 for(std::size_t i = 0; i < size; i++)
				 BaseSerializer<T>::deserialize(data_pack, vec[i], offset);
		 }

	template<>
		struct BaseSerializer<std::vector<DataPack>> {
			static std::size_t pack_size(const std::vector<DataPack>& vec);

			static void serialize(
					DataPack& data_pack, const std::vector<DataPack>& data, std::size_t& offset);
			static void deserialize(
					const DataPack& data_pack, std::vector<DataPack>& data, std::size_t& offset);
		};

	template<typename T, typename JsonType>
		struct JsonSerializer {
			static DataPack serialize(const T& data) {
				std::string str = JsonType(data).dump();
				DataPack pack(str.size(), sizeof(char));
				std::memcpy(pack.buffer, str.data(), str.size() * sizeof(char));
				return pack;
			}

			static T deserialize(const DataPack& pack) {
				return JsonType::parse(
						std::string(pack.buffer, pack.count)
						).template get<T>();
			}
		};

	template<typename T>
		struct Serializer : public JsonSerializer<T, nlohmann::json> {
		};

	template<typename T>
		struct Serializer<std::vector<T>> {
			static DataPack serialize(const std::vector<T>& data) {
				std::vector<DataPack> data_pack(data.size());

				for(std::size_t i = 0; i < data.size(); i++) {
					data_pack[i] = Serializer<T>::serialize(data[i]);
				}

				DataPack total_data_pack(pack_size(data_pack), 1);
				std::size_t offset = 0;
				communication::serialize(total_data_pack, data_pack, offset);

				return total_data_pack;
			}

			static std::vector<T> deserialize(const DataPack& pack) {
				std::vector<DataPack> data_packs;
				std::size_t offset = 0;
				communication::deserialize(pack, data_packs, offset);

				std::vector<T> data;
				for(std::size_t i = 0; i < data_packs.size(); i++) {
					data.emplace_back(Serializer<T>::deserialize(data_packs[i]));
				}
				return data;
			}
		};

	template<typename T>
			 void serialize(
					 std::unordered_map<int, communication::DataPack>& data_pack,
					 const std::unordered_map<int, T>& data) {
				for(auto& item : data) {
					data_pack.emplace(item.first,
							communication::Serializer<T>::serialize(
							item.second
							));
				}
			}

		template<typename T>
			void deserialize(
					std::unordered_map<int, communication::DataPack> data_pack,
					std::unordered_map<int, T>& data
					) {
				for(auto& item : data_pack) {
					data.emplace(item.first, communication::Serializer<T>::deserialize(
							item.second
							));
					item.second.free();
				}
			}

}}
#endif
