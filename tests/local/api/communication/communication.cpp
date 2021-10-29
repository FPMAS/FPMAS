#include "fpmas/api/communication/communication.h"

#include "gmock/gmock.h"

using namespace ::testing;

using namespace fpmas::api::communication;

class DataPackTest : public Test {
	protected:
		DataPack pack {10, sizeof(std::uint64_t)};

		void SetUp() override {
			// Checks if write operations are possible, i.e. the buffer is properly
			// allocated
			for(std::size_t i = 0; i < 10; i++)
				((std::uint64_t*) pack.buffer)[i] = (std::uint64_t) i;
		}

};

TEST_F(DataPackTest, build) {
	ASSERT_EQ(pack.size, 80);

	// Valgrind can be used to ensure that pack.buffer is properly freed
}

TEST_F(DataPackTest, free) {
	pack.free();

	ASSERT_EQ(pack.size, 0);
	ASSERT_EQ(pack.count, 0);
}

TEST_F(DataPackTest, copy_constructor) {
	DataPack pack2(pack);

	ASSERT_EQ(pack, pack2);
}

TEST_F(DataPackTest, copy_assignment) {
	DataPack pack2 = pack;

	ASSERT_EQ(pack, pack2);
}

TEST_F(DataPackTest, move_constructor) {
	DataPack old = pack;
	DataPack pack2(std::move(pack));

	ASSERT_EQ(pack2, old);
	ASSERT_EQ(pack, DataPack());
}

TEST_F(DataPackTest, move_assignment) {
	DataPack old = pack;
	DataPack pack2 = std::move(pack);

	ASSERT_EQ(pack2, old);
	ASSERT_EQ(pack, DataPack());
}
