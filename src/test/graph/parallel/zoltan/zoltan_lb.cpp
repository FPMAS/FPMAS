#include "gtest/gtest.h"

#include "communication/communication.h"
#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"
#include "graph/parallel/synchro/ghost_mode.h"

#include "utils/test.h"

using FPMAS::communication::MpiCommunicator;

using FPMAS::graph::parallel::DistributedGraph;

using FPMAS::test::ASSERT_CONTAINS;

using FPMAS::graph::parallel::zoltan::utils::read_zoltan_id;
using FPMAS::graph::parallel::zoltan::utils::write_zoltan_id;

using FPMAS::graph::parallel::zoltan::obj_list;
using FPMAS::graph::parallel::zoltan::num_edges_multi_fn;

using FPMAS::graph::parallel::synchro::modes::GhostMode;

class Mpi_ZoltanFunctionsTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();

		// Fake Zoltan buffers
		
		// Node lists
		unsigned int global_ids[6];
		unsigned int local_ids[0];
		float weights[3];

		// Resulting global ids
		/*
		 *int nodeIndex[0ul];
		 *int nodeIndex[2ul];
		 *int nodeIndex[85250ul];
		 */

		std::unordered_map<NodeId, int> nodeIndex;

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
			obj_list<int, 1, GhostMode>(
					&dg,
					2,
					0,
					global_ids,
					local_ids,
					1,
					weights,
					&err
					);

			// Assumes that nodes are iterated in the same order within
			// obj_list implementation
			int index = 0;
			for(auto node : dg.getNodes()) {
				nodeIndex[node.first] = index++;
			}
		}

		void write_zoltan_num_edges() {
			num_edges_multi_fn<int, 1, GhostMode>(
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

	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[0ul]]), 0ul);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[2ul]]), 2ul);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[85250ul]]), 85250ul);
}


TEST_F(Mpi_ZoltanFunctionsTest, obj_num_egdes_multi_test) {

	write_zoltan_global_ids();

	write_zoltan_num_edges();

	// Node 0 has 2 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[0ul]], 2);
	// Node 1 has 1 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[2ul]], 1);
	// Node 2 has 0 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[85250ul]], 0);
}

using FPMAS::graph::parallel::zoltan::edge_list_multi_fn;


TEST_F(Mpi_ZoltanFunctionsTest, edge_list_multi_test) {

	write_zoltan_global_ids();

	write_zoltan_num_edges();

	unsigned int nbor_global_id[6];
	int nbor_procs[3];
	float ewgts[3];

	edge_list_multi_fn<int, 1, GhostMode>(
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

	int node1_offset = nodeIndex[0ul] < nodeIndex[2ul] ? 0 : 1;
	int node2_offset = nodeIndex[0ul] < nodeIndex[2ul] ? 2 : 0;

	std::array<unsigned long, 2> node1_edges = {
		read_zoltan_id(&nbor_global_id[(node1_offset) * 2]),
		read_zoltan_id(&nbor_global_id[(node1_offset + 1) * 2]),
	};

	ASSERT_CONTAINS(2, node1_edges);
	ASSERT_CONTAINS(85250, node1_edges);

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[node2_offset * 2]), 0);

	for(int i = 0; i < 3; i++) {
		ASSERT_EQ(ewgts[i], 1.f);
		ASSERT_EQ(nbor_procs[i], dg.getMpiCommunicator().getRank());
	}

}


