#ifndef FPMAS_COMM_SERIALIZER_H
#define FPMAS_COMM_SERIALIZER_H

#include "fpmas/api/communication/communication.h"
#include "fpmas/io/datapack.h"

namespace fpmas { namespace communication {
	using api::communication::DataPack;

	template<typename T, typename JsonType>
		struct JsonSerializer {
			static DataPack to_datapack(const T& data) {
				std::string str = JsonType(data).dump();
				DataPack pack(str.size(), sizeof(char));
				std::memcpy(pack.buffer, str.data(), str.size() * sizeof(char));
				return pack;
			}

			static T from_datapack(const DataPack& pack) {
				return JsonType::parse(
						std::string(pack.buffer, pack.count)
						).template get<T>();
			}
		};

	template<typename T>
		struct Serializer : public JsonSerializer<T, nlohmann::json> {
		};

	template<typename T>
			 void serialize(
					 std::unordered_map<int, DataPack>& data_pack,
					 const std::unordered_map<int, T>& data) {
				for(auto& item : data) {
					data_pack.emplace(item.first,
							Serializer<T>::to_datapack(
							item.second
							));
				}
			}

		template<typename T>
			void deserialize(
					std::unordered_map<int, DataPack> data_pack,
					std::unordered_map<int, T>& data
					) {
				for(auto& item : data_pack) {
					data.emplace(item.first, Serializer<T>::from_datapack(
							item.second
							));
					item.second.free();
				}
			}

}}
#endif
