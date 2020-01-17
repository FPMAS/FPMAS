#include "gtest/gtest.h"

#include "communication/communication.h"
#include "environment/graph/parallel/distributed_graph.h"
#include "environment/graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"

#include "test_utils/test_utils.h"

using FPMAS::communication::MpiCommunicator;

using FPMAS::graph::DistributedGraph;

using FPMAS::test_utils::assert_contains;

using FPMAS::graph::zoltan::utils::read_zoltan_id;
using FPMAS::graph::zoltan::utils::write_zoltan_id;

using FPMAS::graph::zoltan::obj_list;
using FPMAS::graph::zoltan::num_edges_multi_fn;

class Mpi_ZoltanFunctionsTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();

		// Fake Zoltan buffers
		
		// Node lists
		unsigned int global_ids[6];
		unsigned int local_ids[0];
		float weights[3];

		// Resulting global ids
		int node1_index;
		int node2_index;
		int node3_index;

		// Edge lists
		int num_edges[3];

		// Error code
		int err;

		void SetUp() override {
			dg.buildNode(0, 1., 0);
			dg.buildNode(2, 2., 1);
			dg.buildNode(85250, 3., 2);

			dg.link(0, 2, 0);
			dg.link(2, 0, 1);

			dg.link(0, 85250, 2);


		}

		void write_zoltan_global_ids() {
			obj_list<int>(
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
			assert_contains<float, 3>(weights, 1., &node1_index);
			assert_contains<float, 3>(weights, 2., &node2_index);
			assert_contains<float, 3>(weights, 3., &node3_index);
		}

		void write_zoltan_num_edges() {
			num_edges_multi_fn<int>(
					&dg,
					2,
					0,
					3,
					global_ids,
					local_ids,
					num_edges,
					&err
					);
		}
};


TEST_F(Mpi_ZoltanFunctionsTest, obj_list_fn_test) {

	write_zoltan_global_ids();

	ASSERT_EQ(read_zoltan_id(&global_ids[2 * node1_index]), 0ul);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * node2_index]), 2ul);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * node3_index]), 85250ul);
}


TEST_F(Mpi_ZoltanFunctionsTest, obj_num_egdes_multi_test) {

	write_zoltan_global_ids();

	write_zoltan_num_edges();

	// Node 0 has 2 outgoing arcs
	ASSERT_EQ(num_edges[node1_index], 2);
	// Node 1 has 1 outgoing arcs
	ASSERT_EQ(num_edges[node2_index], 1);
	// Node 2 has 0 outgoing arcs
	ASSERT_EQ(num_edges[node3_index], 0);
}

using FPMAS::graph::zoltan::edge_list_multi_fn;


TEST_F(Mpi_ZoltanFunctionsTest, edge_list_multi_test) {

	write_zoltan_global_ids();

	write_zoltan_num_edges();

	unsigned int nbor_global_id[6];
	int nbor_procs[3];
	float ewgts[3];

	edge_list_multi_fn<int>(
			&dg,
			2,
			0,
			3,
			global_ids,
			local_ids,
			num_edges,
			nbor_global_id,
			nbor_procs,
			1,
			ewgts,
			&err
			);

	int node1_offset = node1_index < node2_index ? 0 : 1;
	int node2_offset = node1_index < node2_index ? 2 : 0;

	unsigned long node1_edges[] = {
		read_zoltan_id(&nbor_global_id[(node1_offset) * 2]),
		read_zoltan_id(&nbor_global_id[(node1_offset + 1) * 2]),
	};

	assert_contains<unsigned long, 2>(node1_edges, 2);
	assert_contains<unsigned long, 2>(node1_edges, 85250);

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[node2_offset * 2]), 0);

	for(int i = 0; i < 3; i++) {
		ASSERT_EQ(ewgts[i], 1.f);
		ASSERT_EQ(nbor_procs[i], dg.getMpiCommunicator().getRank());
	}

}


