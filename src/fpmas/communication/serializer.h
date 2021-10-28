#ifndef FPMAS_COMM_SERIALIZER_H
#define FPMAS_COMM_SERIALIZER_H

#include "fpmas/api/communication/communication.h"
#include "fpmas/io/datapack.h"

namespace fpmas { namespace communication {
	using api::communication::DataPack;

	template<typename PackType, typename T>
			 void serialize(
					 std::unordered_map<int, DataPack>& data_pack,
					 const std::unordered_map<int, T>& data) {
				for(auto& item : data) {
					data_pack.emplace(
							item.first,
							PackType(item.second).dump()
							);
				}
			}

		template<typename PackType, typename T>
			void deserialize(
					std::unordered_map<int, DataPack> data_pack,
					std::unordered_map<int, T>& data
					) {
				for(auto& item : data_pack) {
					data.emplace(item.first, PackType::parse(
							std::move(item.second)
							).template get<T>());
					item.second.free();
				}
			}

}}
#endif
