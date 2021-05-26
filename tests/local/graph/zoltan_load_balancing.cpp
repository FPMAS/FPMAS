#include "graph/mock_distributed_edge.h"
#include "graph/mock_distributed_node.h"

#include "fpmas/graph/zoltan_load_balancing.h"

#define NUM_GID_ENTRIES fpmas::graph::zoltan::NUM_GID_ENTRIES

using namespace testing;

using fpmas::graph::zoltan::read_zoltan_id;

using fpmas::graph::zoltan::num_obj;
using fpmas::graph::zoltan::obj_list;
using fpmas::graph::zoltan::num_edges_multi_fn;
using fpmas::graph::zoltan::edge_list_multi_fn;
using fpmas::graph::zoltan::num_fixed_obj_fn;
using fpmas::graph::zoltan::fixed_obj_list_fn;

TEST(Zoltan, read_write_id) {
	fpmas::api::graph::DistributedId id(12, 34578);

	ZOLTAN_ID_TYPE zoltan_id[NUM_GID_ENTRIES];
	fpmas::graph::zoltan::write_zoltan_id(id, zoltan_id);

	fpmas::api::graph::DistributedId read_id
		= fpmas::graph::zoltan::read_zoltan_id(zoltan_id);

	ASSERT_EQ(id, read_id);
}

TEST(Zoltan, read_write_id_overflow) {
	fpmas::api::graph::DistributedId max_id(
			fpmas::api::graph::DistributedId::max_rank,
			fpmas::api::graph::DistributedId::max_id);

	ZOLTAN_ID_TYPE zoltan_id[NUM_GID_ENTRIES];
	fpmas::graph::zoltan::write_zoltan_id(max_id, zoltan_id);

	fpmas::api::graph::DistributedId read_id
		= fpmas::graph::zoltan::read_zoltan_id(zoltan_id);

	ASSERT_EQ(max_id, read_id);
}

class ZoltanFunctionsTest : public ::testing::Test {
	protected:
		DistributedId id1 {0, 1};
		std::vector<fpmas::api::graph::DistributedEdge<int>*> out_edges_1;
		MockDistributedEdge<int, NiceMock>* mock_edge_1;
		MockDistributedEdge<int, NiceMock>* mock_edge_2;

		DistributedId id2 {0, 2};
		std::vector<fpmas::api::graph::DistributedEdge<int>*> out_edges_2;

		DistributedId id3 {2, 3};
		std::vector<fpmas::api::graph::DistributedEdge<int>*> out_edges_3;
		MockDistributedEdge<int, NiceMock>* mock_edge_3;

		fpmas::graph::zoltan::ZoltanData<int> data;

		// Fake Zoltan buffers
		
		// Node lists
		unsigned int global_ids[3*NUM_GID_ENTRIES];
		unsigned int* local_ids;
		float weights[3];

		std::unordered_map<
			DistributedId, int, fpmas::api::graph::IdHash<DistributedId>
			> node_index;

		// Edge lists
		int num_edges[3];

		// Error code
		int err;

		void SetUp() override {
			auto node_1 = new MockDistributedNode<int, NiceMock>(id1, 0, 1.f);
			auto node_2 = new MockDistributedNode<int, NiceMock>(id2, 1, 2.f);
			auto node_3 = new MockDistributedNode<int, NiceMock>(id3, 2, 3.f);
			data.node_map = {{id1, node_1}, {id2, node_2}, {id3, node_3}};
			data.distributed_node_ids = {id1, id2, id3};

			// Mock edge id1 -> id2
			mock_edge_1 = new MockDistributedEdge<int, NiceMock>(
					DistributedId(0, 0), 0, 1.5f
					);
			ON_CALL(*mock_edge_1, getSourceNode)
				.WillByDefault(Return(node_1));
			ON_CALL(*mock_edge_1, getTargetNode)
				.WillByDefault(Return(node_2));
			out_edges_1.push_back(mock_edge_1);

			// Mock edge id1 -> id3
			mock_edge_2 = new MockDistributedEdge<int, NiceMock>(
					DistributedId(0, 1), 1, 3.f
					);
			ON_CALL(*mock_edge_2, getSourceNode)
				.WillByDefault(Return(node_3));
			ON_CALL(*mock_edge_2, getTargetNode)
				.WillByDefault(Return(node_3));
			out_edges_1.push_back(mock_edge_2);

			mock_edge_3 = new MockDistributedEdge<int, NiceMock>(
					DistributedId(0, 2),
					0,
					2.f
					);
			ON_CALL(*mock_edge_3, getSourceNode)
				.WillByDefault(Return(node_2));
			ON_CALL(*mock_edge_3, getTargetNode)
				.WillByDefault(Return(node_1));
			out_edges_2.push_back(mock_edge_3);

			ON_CALL(*node_1, getOutgoingEdges())
				.WillByDefault(Return(out_edges_1));
			ON_CALL(*node_2, getOutgoingEdges())
				.WillByDefault(Return(out_edges_2));
			ON_CALL(*node_3, getOutgoingEdges())
				.WillByDefault(Return(out_edges_3));

			ON_CALL(*node_1, location())
				.WillByDefault(Return(0));
			ON_CALL(*node_2, location())
				.WillByDefault(Return(3));
			ON_CALL(*node_3, location())
				.WillByDefault(Return(5));

			// Assumes that nodes are iterated in the same order within
			// obj_list implementation
			int index = 0;
			for(auto node : data.node_map) {
				node_index[node.first] = index++;
			}
		}

		void TearDown() override {
			for(auto node : data.node_map)
				delete node.second;
			for(auto edge : out_edges_1)
				delete edge;
			for(auto edge : out_edges_2)
				delete edge;
		}

		void write_zoltan_global_ids() {
			obj_list<int>(
					&data,
					NUM_GID_ENTRIES,
					0,
					global_ids,
					local_ids,
					1,
					weights,
					&err
					);
		}

		void write_zoltan_num_edges() {
			num_edges_multi_fn<int>(
					&data,
					NUM_GID_ENTRIES,
					0,
					3,
					global_ids,
					local_ids,
					num_edges,
					&err
					);
		}
};

TEST_F(ZoltanFunctionsTest, num_obj) {
	int num = num_obj<int>(&data, &err);
	ASSERT_EQ(num, 3);
}

TEST_F(ZoltanFunctionsTest, obj_list_fn_test) {
	write_zoltan_global_ids();

	ASSERT_EQ(read_zoltan_id(&global_ids[NUM_GID_ENTRIES * node_index[id1]]), id1);
	ASSERT_EQ(read_zoltan_id(&global_ids[NUM_GID_ENTRIES * node_index[id2]]), id2);
	ASSERT_EQ(read_zoltan_id(&global_ids[NUM_GID_ENTRIES * node_index[id3]]), id3);
}


TEST_F(ZoltanFunctionsTest, obj_num_egdes_multi_test) {
	write_zoltan_global_ids();
	write_zoltan_num_edges();

	// Node 0 has 2 outgoing edges
	ASSERT_EQ(num_edges[node_index[id1]], 2);
	// Node 1 has 1 outgoing edges
	ASSERT_EQ(num_edges[node_index[id2]], 1);
	// Node 2 has 0 outgoing edges
	ASSERT_EQ(num_edges[node_index[id3]], 0);
}



TEST_F(ZoltanFunctionsTest, edge_list_multi_test) {
	write_zoltan_global_ids();
	write_zoltan_num_edges();

	unsigned int nbor_global_id[3*NUM_GID_ENTRIES];
	int nbor_procs[3];
	float ewgts[3];

	edge_list_multi_fn<int>(
			&data,
			NUM_GID_ENTRIES,
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

	int node1_offset = node_index[id1] < node_index[id2] ? 0 : 1;
	int node2_offset = node_index[id1] < node_index[id2] ? 2 : 0;

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[(node1_offset) * NUM_GID_ENTRIES]), id2);
	ASSERT_EQ(ewgts[node1_offset], 1.5f);
	ASSERT_EQ(nbor_procs[node1_offset], 3);

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[(node1_offset + 1) * NUM_GID_ENTRIES]), id3);
	ASSERT_EQ(ewgts[node1_offset+1], 3.f);
	ASSERT_EQ(nbor_procs[node1_offset+1], 5);

	ASSERT_EQ(read_zoltan_id(&nbor_global_id[node2_offset * NUM_GID_ENTRIES]), id1);
	ASSERT_EQ(ewgts[node2_offset], 2.f);
	ASSERT_EQ(nbor_procs[node2_offset], 0);
}

TEST_F(ZoltanFunctionsTest, num_fixed_obj) {
	int err;
	int num = num_fixed_obj_fn<int>(&data.node_map, &err);

	ASSERT_EQ(num, 3);
}

TEST_F(ZoltanFunctionsTest, fixed_obj_list) {
	fpmas::graph::PartitionMap map;
	map[id1] = 7;
	map[id2] = 2;
	map[id3] = 3;

	std::unordered_map<DistributedId, int> node_index;
	// Assumes that nodes are iterated in the same order within
	// fixed_obj_list implementation
	int index = 0;
	for(auto node : map) {
		node_index[node.first] = index++;
	}

	unsigned int fixed_ids[3*NUM_GID_ENTRIES];
	int fixed_parts[3];
	fixed_obj_list_fn<int>(
			&map,
			3,
			NUM_GID_ENTRIES,
			fixed_ids,
			fixed_parts,
			&err
			);

	ASSERT_EQ(read_zoltan_id(&fixed_ids[NUM_GID_ENTRIES * node_index[id1]]), id1);
	ASSERT_EQ(read_zoltan_id(&fixed_ids[NUM_GID_ENTRIES * node_index[id2]]), id2);
	ASSERT_EQ(read_zoltan_id(&fixed_ids[NUM_GID_ENTRIES * node_index[id3]]), id3);

	ASSERT_EQ(fixed_parts[node_index[id1]], 7);
	ASSERT_EQ(fixed_parts[node_index[id2]], 2);
	ASSERT_EQ(fixed_parts[node_index[id3]], 3);
}
