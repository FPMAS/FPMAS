#ifndef FPMAS_COMM_SERIALIZER_H
#define FPMAS_COMM_SERIALIZER_H

#include "fpmas/api/communication/communication.h"

namespace fpmas { namespace communication {
	using api::communication::DataPack;

	template<typename T>
		void serialize(DataPack& data_pack, const T& data, std::size_t& offset) {
			std::memcpy(&data_pack.buffer[offset], &data, sizeof(T));
			offset += sizeof(T);
		}

	template<>
		void serialize<DistributedId>(
				DataPack& data_pack, const DistributedId& id, std::size_t& offset);

	template<>
		void serialize<std::string>(
				DataPack& data_pack, const std::string& data, std::size_t& offset);
	template<>
		void serialize<std::vector<std::size_t>>(
				DataPack& data_pack, const std::vector<std::size_t>& vec, std::size_t& offset);

	template<typename T>
		void deserialize(const DataPack& data_pack, T& data, std::size_t& offset) {
			std::memcpy(&data, &data_pack.buffer[offset], sizeof(T));
			offset += sizeof(T);
		}

	template<>
		void deserialize<DistributedId>(
				const DataPack& data_pack, DistributedId& id, std::size_t& offset);

	template<>
		 void deserialize<std::string>(
				const DataPack& data_pack, std::string& str, std::size_t& offset);
	template<>
		 void deserialize<std::vector<std::size_t>>(
				const DataPack& data_pack, std::vector<std::size_t>& vec, std::size_t& offset);


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
				std::size_t total_size = 0;
				std::vector<std::size_t> sizes(data.size());
				for(std::size_t i = 0; i < data.size(); i++) {
					data_pack[i] = Serializer<T>::serialize(data[i]);
					sizes[i] = data_pack[i].size;
					total_size += sizes[i];
				}
				DataPack total_data_pack(
						(data.size()+1) * sizeof(std::size_t) // sizes
						+ total_size, // data
						1
						);
				std::size_t offset = 0;
				communication::serialize(total_data_pack, sizes, offset);
				for(std::size_t i = 0; i < data_pack.size(); i++) {
					std::memcpy(
							&total_data_pack.buffer[offset],
							data_pack[i].buffer, data_pack[i].size
							);
					offset += sizes[i];
					data_pack[i].free();
				}
				return total_data_pack;
			}

			static std::vector<T> deserialize(const DataPack& pack) {
				std::vector<std::size_t> sizes;
				std::size_t offset = 0;
				communication::deserialize(pack, sizes, offset);

				std::vector<DataPack> data_packs(sizes.size());
				for(std::size_t i = 0; i < data_packs.size(); i++) {
					data_packs[i] = {(int) sizes[i], 1};
					std::memcpy(
							data_packs[i].buffer, 
							&pack.buffer[offset],
							sizes[i]
							);
					offset += sizes[i];
				}

				std::vector<T> data;
				for(std::size_t i = 0; i < sizes.size(); i++) {
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
