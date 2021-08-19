#include "fpmas/model/spatial/grid_load_balancing.h"
#include "../../../mocks/communication/mock_communication.h"

using namespace testing;

TEST(StripProcessMapping, width_sup_height) {
	MockMpiCommunicator<2, 4> comm;
	int width = 8;
	int height = 2;
	fpmas::model::StripProcessMapping mapping(width, height, comm);

	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 2; j++)
			for(int k = 0; k < height; k++)
				ASSERT_EQ(mapping.process({2*i+j, k}), i);
}

TEST(StripProcessMapping, height_sup_width) {
	MockMpiCommunicator<2, 4> comm;
	int width = 2;
	int height = 8;
	fpmas::model::StripProcessMapping mapping(width, height, comm);

	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 2; j++)
			for(int k = 0; k < width; k++)
				ASSERT_EQ(mapping.process({j, 2*i+j}), i);
}

TEST(TreeProcessMapping, 4_processes) {
	MockMpiCommunicator<2, 4> comm;

	fpmas::model::TreeProcessMapping mapping(10, 10, comm);

	std::vector<int> processes {
		mapping.process({0, 0}),
		mapping.process({0, 9}),
		mapping.process({9, 0}),
		mapping.process({9, 9})
	};
	ASSERT_THAT(processes, UnorderedElementsAre(0, 1, 2, 3));
}

TEST(TreeProcessMapping, 16_processes) {
	MockMpiCommunicator<2, 16> comm;

	fpmas::model::TreeProcessMapping mapping(8, 8, comm);
	mapping.process({0, 0});

	std::vector<int> processes;
	std::vector<int> expected;
	for(int i = 0; i < 8; i++) {
		for(int j = 0; j < 8; j++) {
			processes.push_back(mapping.process({i, j}));
		}
	}
	for(int i = 0; i < 16; i++)
		for(int k = 0; k < 4; k++)
			expected.push_back(i);
	ASSERT_THAT(processes, UnorderedElementsAreArray(expected));
}
