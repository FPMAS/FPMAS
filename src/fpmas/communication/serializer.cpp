#include "serializer.h"

namespace fpmas { namespace communication {

	/*
	 * DistributedId
	 */

	std::size_t BaseSerializer<DistributedId>::pack_size() {
		return
			communication::pack_size<int>()
			+ communication::pack_size<FPMAS_ID_TYPE>();
	}

	void BaseSerializer<DistributedId>::serialize(
			DataPack& data_pack, const DistributedId& id, std::size_t& offset) {
		communication::serialize(data_pack, id.rank(), offset);
		communication::serialize(data_pack, id.id(), offset);
	}

	void BaseSerializer<DistributedId>::deserialize(
			const DataPack& data_pack, DistributedId& id, std::size_t& offset) {
		int rank;
		communication::deserialize(data_pack, rank, offset);
		FPMAS_ID_TYPE _id;
		communication::deserialize(data_pack, _id, offset);
		id = {rank, _id};
	}

	/*
	 * std::string
	 */

	std::size_t BaseSerializer<std::string>::pack_size(const std::string &str) {
		return communication::pack_size<std::size_t>()
			+ str.size() * sizeof(std::string::value_type);
	};

	void BaseSerializer<std::string>::serialize(
			DataPack& data_pack, const std::string& str, std::size_t& offset) {
		communication::serialize(data_pack, str.size(), offset);
		std::memcpy(
				&data_pack.buffer[offset], str.data(),
				str.size() * sizeof(std::string::value_type)
				);
		offset += str.size() * sizeof(std::string::value_type);
	}

	void BaseSerializer<std::string>::deserialize(
			const DataPack& data_pack, std::string& str, std::size_t& offset) {
		std::size_t size;
		communication::deserialize(data_pack, size, offset);

		str = std::string(&data_pack.buffer[offset], size);
		offset += size * sizeof(char);
	}

	/*
	 * DataPack
	 */

	std::size_t BaseSerializer<std::vector<DataPack>>::pack_size(const std::vector<DataPack> &data) {
		std::size_t size
			= communication::pack_size<std::vector<std::size_t>>(data.size());
		for(auto item : data)
			size += item.size;
		return size;
	};

	void BaseSerializer<std::vector<DataPack>>::serialize(
			DataPack& data_pack, const std::vector<DataPack>& vec,
			std::size_t& offset) {
		std::vector<std::size_t> sizes(vec.size());
		for(std::size_t i = 0; i < vec.size(); i++)
			sizes[i] = vec[i].size;

		communication::serialize(data_pack, sizes, offset);
		for(std::size_t i = 0; i < vec.size(); i++) {
			std::memcpy(
					&data_pack.buffer[offset],
					vec[i].buffer, sizes[i]
					);
			offset += sizes[i];
		}
	}

	void BaseSerializer<std::vector<DataPack>>::deserialize(
			const DataPack &data_pack, std::vector<DataPack> &data,
			std::size_t &offset) {
		std::vector<std::size_t> sizes;
		communication::deserialize(data_pack, sizes, offset);

		data.resize(sizes.size());
		for(std::size_t i = 0; i < data.size(); i++) {
			data[i] = {(int) sizes[i], 1};
			std::memcpy(
					data[i].buffer, 
					&data_pack.buffer[offset],
					sizes[i]
					);
			offset += sizes[i];
		}
	}
}}
