#include "fpmas/communication/serializer.h"

#include "gmock/gmock.h"

using namespace ::testing;

TEST(Serializer, DistributedId) {
	fpmas::api::graph::DistributedId id1(8, 421);
	fpmas::api::graph::DistributedId id2(2, 14);

	fpmas::api::communication::DataPack pack(2*sizeof(int) + 2*sizeof(FPMAS_ID_TYPE), 1);
	std::size_t offset = 0;

	fpmas::communication::serialize(pack, id1, offset);
	fpmas::communication::serialize(pack, id2, offset);

	offset = 0;
	DistributedId deserial_id1;
	fpmas::communication::deserialize<fpmas::api::graph::DistributedId>(
				pack, deserial_id1, offset);

	DistributedId deserial_id2;
	fpmas::communication::deserialize<fpmas::api::graph::DistributedId>(
				pack, deserial_id2, offset);

	ASSERT_EQ(id1, deserial_id1);
	ASSERT_EQ(id2, deserial_id2);
}

TEST(Serializer, size_vector) {
	std::array<std::vector<std::size_t>, 2> data = {{
		{34, 25000, 560},
		{1, 2, 72, 42}
	}};
	std::size_t offset = 0;
	fpmas::communication::DataPack pack((1+3+1+4)*sizeof(std::size_t), 1);
	fpmas::communication::serialize(pack, data[0], offset);
	fpmas::communication::serialize(pack, data[1], offset);

	offset = 0;
	std::array<std::vector<std::size_t>, 2> deserial_data;
	fpmas::communication::deserialize(pack, deserial_data[0], offset);
	fpmas::communication::deserialize(pack, deserial_data[1], offset);

	ASSERT_THAT(deserial_data[0], ElementsAreArray(data[0]));
	ASSERT_THAT(deserial_data[1], ElementsAreArray(data[1]));
}

TEST(Serializer, vector) {
	std::vector<std::string> data = {"ab", "zzzzz", "678908DSDJSK"};
	auto serial_data = fpmas::communication::Serializer<std::vector<std::string>>
		::serialize(data);
	auto deserial_data = fpmas::communication::Serializer<std::vector<std::string>>
		::deserialize(serial_data);

	ASSERT_THAT(deserial_data, ElementsAreArray(data));
}
