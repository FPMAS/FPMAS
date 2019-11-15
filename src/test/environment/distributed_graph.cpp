#include "gtest/gtest.h"

#include "environment/distributed_graph.h"
#include "utils/config.h"

#include "test_utils/test_utils.h"

using FPMAS::graph::DistributedGraph;

using FPMAS::test_utils::assert_contains;

class Mpi_ZoltanFunctionsTest : public ::testing::Test {
	protected:
		static Zoltan* zz;
		static std::array<int*, 3> data;

		static void SetUpTestSuite() {
			zz = new Zoltan(MPI_COMM_WORLD);
			FPMAS::config::zoltan_config(zz);

			for(int i = 0; i < 2; i++) {
				data[i] = new int(i);
			}
		}

		DistributedGraph<int> dg = DistributedGraph<int>(zz);

		void SetUp() override {
			dg.buildNode(0ul, 1., data[0]);
			dg.buildNode(2ul, 2., data[1]);
			dg.buildNode(85250ul, 3., data[2]);

		}

		static void TearDownTestSuite() {
			delete zz;
			for(auto i : data) {
				delete i;
			}
		}
};
Zoltan* Mpi_ZoltanFunctionsTest::zz = nullptr;
std::array<int*, 3> Mpi_ZoltanFunctionsTest::data;

using FPMAS::graph::zoltan_obj_list;

TEST_F(Mpi_ZoltanFunctionsTest, obj_list_fn_test) {

	unsigned int global_ids[6];
	unsigned int local_ids[0]; // unused

	float weights[3];
	int err;

	zoltan_obj_list<int>(
			&dg,
			2,
			0,
			global_ids,
			local_ids,
			1,
			weights,
			&err
			);

	// We don't know in which order nodes will be processed internally.
	// So, for the purpose of the test, we use weights to find which node
	// correspond to which index in weights and global_ids.
	int node1_index;
	assert_contains<float, 3>(weights, 1., &node1_index);

	int node2_index;
	assert_contains<float, 3>(weights, 2., &node2_index);

	int node3_index;
	assert_contains<float, 3>(weights, 3., &node3_index);

	ASSERT_EQ(FPMAS::graph::node_id(global_ids, 2 * node1_index), 0ul);
	ASSERT_EQ(FPMAS::graph::node_id(global_ids, 2 * node2_index), 2ul);
	ASSERT_EQ(FPMAS::graph::node_id(global_ids, 2 * node3_index), 85250ul);
}
