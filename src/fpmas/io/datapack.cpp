#include "datapack.h"

namespace fpmas { namespace io { namespace datapack {

	/*
	 * DistributedId
	 */

	std::size_t base_io<DistributedId>::pack_size() {
		return
			datapack::pack_size<int>()
			+ datapack::pack_size<FPMAS_ID_TYPE>();
	}

	std::size_t base_io<DistributedId>::pack_size(const DistributedId &) {
		return pack_size();
	}

	void base_io<DistributedId>::write(
			DataPack& data_pack, const DistributedId& id, std::size_t& offset) {
		datapack::write(data_pack, id.rank(), offset);
		datapack::write(data_pack, id.id(), offset);
	}

	void base_io<DistributedId>::read(
			const DataPack& data_pack, DistributedId& id, std::size_t& offset) {
		int rank;
		datapack::read(data_pack, rank, offset);
		FPMAS_ID_TYPE _id;
		datapack::read(data_pack, _id, offset);
		id = {rank, _id};
	}

	/*
	 * std::string
	 */

	std::size_t base_io<std::string>::pack_size(const std::string &str) {
		return datapack::pack_size<std::size_t>()
			+ str.size() * sizeof(std::string::value_type);
	};

	void base_io<std::string>::write(
			DataPack& data_pack, const std::string& str, std::size_t& offset) {
		datapack::write(data_pack, str.size(), offset);
		std::memcpy(
				&data_pack.buffer[offset], str.data(),
				str.size() * sizeof(std::string::value_type)
				);
		offset += str.size() * sizeof(std::string::value_type);
	}

	void base_io<std::string>::read(
			const DataPack& data_pack, std::string& str, std::size_t& offset) {
		std::size_t size;
		datapack::read(data_pack, size, offset);

		str = std::string(&data_pack.buffer[offset], size);
		offset += size * sizeof(char);
	}

	/*
	 * DataPack
	 */

	std::size_t base_io<std::vector<DataPack>>::pack_size(const std::vector<DataPack> &data) {
		std::size_t size
			= datapack::pack_size<std::vector<std::size_t>>(data.size());
		for(auto item : data)
			size += item.size;
		return size;
	};

	void base_io<std::vector<DataPack>>::write(
			DataPack& data_pack, const std::vector<DataPack>& vec,
			std::size_t& offset) {
		std::vector<std::size_t> sizes(vec.size());
		for(std::size_t i = 0; i < vec.size(); i++)
			sizes[i] = vec[i].size;

		datapack::write(data_pack, sizes, offset);
		for(std::size_t i = 0; i < vec.size(); i++) {
			std::memcpy(
					&data_pack.buffer[offset],
					vec[i].buffer, sizes[i]
					);
			offset += sizes[i];
		}
	}

	void base_io<std::vector<DataPack>>::read(
			const DataPack &data_pack, std::vector<DataPack> &data,
			std::size_t &offset) {
		std::vector<std::size_t> sizes;
		datapack::read(data_pack, sizes, offset);

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
}}}
