#include "serializer.h"

namespace fpmas { namespace communication {

	template<>
		void serialize<DistributedId>(
				DataPack& data_pack, const DistributedId& id, std::size_t& offset) {
			serialize(data_pack, id.rank(), offset);
			serialize(data_pack, id.id(), offset);
		}

	template<>
		void serialize<std::string>(
				DataPack& data_pack, const std::string& data, std::size_t& offset) {
			serialize(data_pack, data.size(), offset);
			std::memcpy(
					&data_pack.buffer[offset], data.data(),
					data.size() * sizeof(std::string::value_type)
					);
			offset += data.size() * sizeof(std::string::value_type);
		}

	template<>
		void serialize<std::vector<std::size_t>>(
				DataPack& data_pack, const std::vector<std::size_t>& vec,
				std::size_t& offset) {
			serialize(data_pack, vec.size(), offset);
			for(auto item : vec)
				serialize(data_pack, item, offset);
		}

	template<>
		void deserialize<DistributedId>(
				const DataPack& data_pack, DistributedId& id, std::size_t& offset) {
				int rank;
				deserialize<int>(data_pack, rank, offset);
				FPMAS_ID_TYPE _id;
				deserialize<FPMAS_ID_TYPE>(data_pack, _id, offset);
				id = {rank, _id};
		}

	template<>
		void deserialize<std::string>(
				const DataPack& data_pack, std::string& str, std::size_t& offset) {
			std::size_t size;
			deserialize<std::size_t>(data_pack, size, offset);

			str = std::string(&data_pack.buffer[offset], size);
			offset += size * sizeof(char);
		}

	template<>
		 void deserialize<std::vector<std::size_t>>(
				const DataPack& data_pack, std::vector<std::size_t>& vec,
				std::size_t& offset) {
			 std::size_t size;
			 deserialize(data_pack, size, offset);
			 vec.resize(size);
			 for(std::size_t i = 0; i < size; i++)
				 deserialize(data_pack, vec[i], offset);
		 }
}}
