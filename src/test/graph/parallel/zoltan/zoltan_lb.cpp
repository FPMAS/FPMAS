#include "gtest/gtest.h"

#include "api/graph/base/mock_arc.h"
#include "api/graph/base/mock_node.h"
#include "api/graph/parallel/proxy/mock_proxy.h"

#include "graph/parallel/zoltan/zoltan_load_balancing.h"
#include "graph/parallel/zoltan/zoltan_utils.h"

#include "utils/test.h"

using ::testing::AtLeast;
using ::testing::AnyNumber;
using FPMAS::test::ASSERT_CONTAINS;

using FPMAS::graph::parallel::zoltan::utils::read_zoltan_id;

using FPMAS::graph::parallel::zoltan::num_obj;
using FPMAS::graph::parallel::zoltan::obj_list;
using FPMAS::graph::parallel::zoltan::num_edges_multi_fn;
using FPMAS::graph::parallel::zoltan::edge_list_multi_fn;


class ZoltanLBFunctionsTest : public ::testing::Test {
	protected:
		DistributedId id1 {0, 1};
		std::vector<MockArc<int, DistributedId>*> outArcs1;
		MockArc<int, DistributedId>* mockArc1;
		MockArc<int, DistributedId>* mockArc2;

		DistributedId id2 {0, 2};
		std::vector<MockArc<int, DistributedId>*> outArcs2;

		DistributedId id3 {2, 3};
		std::vector<MockArc<int, DistributedId>*> outArcs3;
		MockArc<int, DistributedId>* mockArc3;

		std::unordered_map<DistributedId, MockNode<int, DistributedId>*> nodes;

		// Fake Zoltan buffers
		
		// Node lists
		unsigned int global_ids[6];
		unsigned int local_ids[0];
		float weights[3];

		std::unordered_map<DistributedId, int> nodeIndex;

		// Edge lists
		int num_edges[3];

		// Error code
		int err;

		void SetUp() override {
			nodes[id1] = new MockNode(id1, 0, 1.f);
			nodes[id2] = new MockNode(id2, 1, 2.f);
			nodes[id3] = new MockNode(id3, 2, 3.f);

			mockArc1 = new MockArc<int, DistributedId>(
					DistributedId(0, 0),
					nodes[id1], nodes[id2],
					0,
					1.5f
					);
			EXPECT_CALL(*mockArc1, getTargetNode).Times(AnyNumber());
			outArcs1.push_back(mockArc1);

			mockArc2 = new MockArc<int, DistributedId>(
					DistributedId(0, 1),
					nodes[id1], nodes[id3],
					1,
					3.f
					);
			EXPECT_CALL(*mockArc2, getTargetNode).Times(AnyNumber());
			outArcs1.push_back(mockArc2);

			mockArc3 = new MockArc<int, DistributedId>(
					DistributedId(0, 2),
					nodes[id2], nodes[id1],
					0,
					2.f
					);
			EXPECT_CALL(*mockArc3, getTargetNode).Times(AnyNumber());
			outArcs2.push_back(mockArc3);

			// Assumes that nodes are iterated in the same order within
			// obj_list implementation
			int index = 0;
			for(auto node : nodes) {
				nodeIndex[node.first] = index++;
			}
		}

		void TearDown() override {
			for(auto node : nodes)
				delete node.second;
			for(auto arc : outArcs1)
				delete arc;
			for(auto arc : outArcs2)
				delete arc;
		}

		void write_zoltan_global_ids() {
			EXPECT_CALL(*nodes[id1], getWeight).Times(1);
			EXPECT_CALL(*nodes[id2], getWeight).Times(1);
			EXPECT_CALL(*nodes[id3], getWeight).Times(1);
			obj_list<MockNode<int, DistributedId>>(
					&nodes,
					2,
					0,
					global_ids,
					local_ids,
					1,
					weights,
					&err
					);
		}

		void write_zoltan_num_edges() {
			EXPECT_CALL(*nodes[id1], getOutgoingArcs())
				.Times(AtLeast(1)).WillRepeatedly(Return(outArcs1));
			EXPECT_CALL(*nodes[id2], getOutgoingArcs())
				.Times(AtLeast(1)).WillRepeatedly(Return(outArcs2));
			EXPECT_CALL(*nodes[id3], getOutgoingArcs())
				.Times(AtLeast(1)).WillRepeatedly(Return(outArcs3));

			for(auto node : nodes) {
				EXPECT_CALL(*node.second, getId).Times(AnyNumber());
			}

			num_edges_multi_fn<MockNode<int, DistributedId>>(
					&nodes,
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

TEST_F(ZoltanLBFunctionsTest, num_obj) {
	int num = num_obj<MockNode<int, DistributedId>>(&nodes, &err);
	ASSERT_EQ(num, 3);
}

TEST_F(ZoltanLBFunctionsTest, obj_list_fn_test) {
	write_zoltan_global_ids();

	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[id1]]), id1);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[id2]]), id2);
	ASSERT_EQ(read_zoltan_id(&global_ids[2 * nodeIndex[id3]]), id3);
}


TEST_F(ZoltanLBFunctionsTest, obj_num_egdes_multi_test) {
	write_zoltan_global_ids();
	write_zoltan_num_edges();

	// Node 0 has 2 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[id1]], 2);
	// Node 1 has 1 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[id2]], 1);
	// Node 2 has 0 outgoing arcs
	ASSERT_EQ(num_edges[nodeIndex[id3]], 0);
}



TEST_F(ZoltanLBFunctionsTest, edge_list_multi_test) {
	write_zoltan_global_ids();
	write_zoltan_num_edges();

	MockProxy proxy;
	EXPECT_CALL(proxy, getCurrentLocation(id1))
		.WillRepeatedly(Return(0));
	EXPECT_CALL(proxy, getCurrentLocation(id2))
		.WillRepeatedly(Return(3));
	EXPECT_CALL(proxy, getCurrentLocation(id3))
		.WillRepeatedly(Return(5));

	EXPECT_CALL(*mockArc1, getWeight).Times(AtLeast(1));
	EXPECT_CALL(*mockArc2, getWeight).Times(AtLeast(1));
	EXPECT_CALL(*mockArc3, getWeight).Times(AtLeast(1));

	FPMAS::graph::parallel::zoltan::NodesProxyPack<MockNode<int, DistributedId>> pack
	{&nodes, &proxy};

	unsigned int nbor_global_id[6];
	int nbor_procs[3];
	float ewgts[3];

	edge_list_multi_fn<MockNode<int, DistributedId>>(
			&pack,
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

	int node1_offset = nodeIndex[id1] < nodeIndex[id2] ? 0 : 1;
	int node2_offset = nodeIndex[id1] < nodeIndex[id2] ? 2 : 0;

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[(node1_offset) * 2]), id2);
	ASSERT_EQ(ewgts[node1_offset], 1.5f);
	ASSERT_EQ(nbor_procs[node1_offset], 3);

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[(node1_offset + 1) * 2]), id3);
	ASSERT_EQ(ewgts[node1_offset+1], 3.f);
	ASSERT_EQ(nbor_procs[node1_offset+1], 5);

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[node2_offset * 2]), id1);
	ASSERT_EQ(ewgts[node2_offset], 2.f);
	ASSERT_EQ(nbor_procs[node2_offset], 0);
}


