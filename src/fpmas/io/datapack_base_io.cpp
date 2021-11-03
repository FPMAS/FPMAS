#include "datapack_base_io.h"

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
			+ str.size() * datapack::pack_size<char>();
	};

	void base_io<std::string>::write(
			DataPack& data_pack, const std::string& str, std::size_t& offset) {
		datapack::write(data_pack, str.size(), offset);
		std::memcpy(
				&data_pack.buffer[offset], str.data(),
				str.size() * datapack::pack_size<char>()
				);
		offset += str.size() * datapack::pack_size<char>();
	}

	void base_io<std::string>::read(
			const DataPack& data_pack, std::string& str, std::size_t& offset) {
		std::size_t size;
		datapack::read(data_pack, size, offset);

		str = std::string(&data_pack.buffer[offset], size);
		offset += size * datapack::pack_size<char>();
	}

	/*
	 * DataPack
	 */

	std::size_t base_io<DataPack>::pack_size(const DataPack& data) {
		return datapack::pack_size<std::size_t>() + data.size;
	}

	void base_io<DataPack>::write(DataPack& dest, const DataPack& source, std::size_t& offset) {
		datapack::write(dest, source.size, offset);

		std::memcpy(&dest.buffer[offset], source.buffer, source.size);
		offset+=source.size;
	}

	void base_io<DataPack>::read(const DataPack& source, DataPack& dest, std::size_t& offset) {
		std::size_t size;
		datapack::read(source, size, offset);

		dest = {size, 1};
		std::memcpy(dest.buffer, &source.buffer[offset], dest.size);
		offset+=dest.size;
	}
}}}
