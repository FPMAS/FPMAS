#include "gtest/gtest.h"

#include "communication/communication.h"
#include "environment/graph/parallel/distributed_graph.h"
#include "environment/graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"

#include "test_utils/test_utils.h"

using FPMAS::communication::MpiCommunicator;

using FPMAS::graph::zoltan::utils::write_zoltan_id;

using FPMAS::graph::zoltan::node::obj_size_multi_fn;
using FPMAS::graph::zoltan::node::pack_obj_multi_fn;

class Mpi_ZoltanNodeMigrationFunctionsTest : public ::testing::Test {
	protected:
		static std::array<int*, 3> data;

		static void SetUpTestSuite() {
			for(int i = 0; i < 3; i++) {
				data[i] = new int(i);
			}
		}

		DistributedGraph<int> dg = DistributedGraph<int>();

		// Migration
		unsigned int transfer_global_ids[4];
		int sizes[2];
		int idx[2];
		char buf[250];

		// Error code
		int err;

		void SetUp() override {
			dg.buildNode(0, 1., data[0]);
			dg.buildNode(2, 2., data[1]);
			dg.buildNode(85250, 3., data[2]);

			dg.link(0, 2, 0);
			dg.link(2, 0, 1);

			dg.link(0, 85250, 2);


		}

		void write_migration_sizes() {
			// Transfer nodes 0 and 85250
			write_zoltan_id(0, &transfer_global_ids[0]);
			write_zoltan_id(85250, &transfer_global_ids[2]);

			obj_size_multi_fn<int>(
					&dg,
					2,
					0,
					2,
					transfer_global_ids,
					nullptr,
					sizes,
					&err
					);
		}

		void write_communication_buffer() {
			// Automatically generated by Zoltan in a real use case
			idx[0] = 0;
			idx[1] = sizes[0] + 1;

			// Unused
			int dest[2];

			pack_obj_multi_fn<int>(
					&dg,
					2,
					0,
					2,
					transfer_global_ids,
					nullptr,
					dest,
					sizes,
					idx,
					buf,
					&err
					);
		}

		static void TearDownTestSuite() {
			for(auto i : data) {
				delete i;
			}
		}
};
std::array<int*, 3> Mpi_ZoltanNodeMigrationFunctionsTest::data;

TEST_F(Mpi_ZoltanNodeMigrationFunctionsTest, obj_size_multi_test) {

	write_migration_sizes();

	int origin = dg.getMpiCommunicator().getRank();
	json node1_str = *dg.getNodes().at(0);
	node1_str["origin"] = origin;
	ASSERT_EQ(sizes[0], node1_str.dump().size() + 1);

	json node2_str = *dg.getNodes().at(85250);
	node2_str["origin"] = origin;
	ASSERT_EQ(sizes[1], node2_str.dump().size() + 1);
}


TEST_F(Mpi_ZoltanNodeMigrationFunctionsTest, pack_obj_multi_test) {

	write_migration_sizes();
	write_communication_buffer();

	std::string origin = std::to_string(dg.getMpiCommunicator().getRank());
	// Decompose and check buffer data
	ASSERT_STREQ(
		&buf[0],
		std::string(R"({"data":0,"id":0,"origin":)" + origin + R"(,"weight":1.0})").c_str()
		);

	ASSERT_STREQ(
		&buf[idx[1]],
		std::string(R"({"data":2,"id":85250,"origin":)" + origin + R"(,"weight":3.0})").c_str()
		);
}

using FPMAS::graph::zoltan::node::unpack_obj_multi_fn;

TEST_F(Mpi_ZoltanNodeMigrationFunctionsTest, unpack_obj_multi_test) {

	write_migration_sizes();
	write_communication_buffer();

	DistributedGraph<int> g = DistributedGraph<int>();
	unpack_obj_multi_fn<int>(
		&g,
		2,
		2,
		transfer_global_ids,
		sizes,
		idx,
		buf,
		&err);

	ASSERT_EQ(g.getNodes().size(), 2);

	ASSERT_EQ(g.getNodes().count(0), 1);
	FPMAS::graph::Node<int>* node0 = g.getNodes().at(0);
	ASSERT_EQ(node0->getId(), 0);
	ASSERT_EQ(*node0->getData(), 0);
	ASSERT_EQ(node0->getWeight(), 1.f);

	ASSERT_EQ(g.getNodes().count(85250ul), 1);
	FPMAS::graph::Node<int>* node1 = g.getNodes().at(85250);
	ASSERT_EQ(node1->getId(), 85250ul);
	ASSERT_EQ(*node1->getData(), 2);
	ASSERT_EQ(node1->getWeight(), 3.f);

}

using FPMAS::graph::zoltan::node::post_migrate_pp_fn;

// Only modify graph private internal structure (set up arc export lists)
// In consecuence, no real assertion is made there, but the function is still
// executed to check for memory issues or other runtime errors
TEST_F(Mpi_ZoltanNodeMigrationFunctionsTest, post_migrate_test) {
	unsigned int export_global_ids[4];
	write_zoltan_id(0ul, &export_global_ids[0]);
	write_zoltan_id(85250ul, &export_global_ids[2]);

	// Fake export info
	int export_procs[2] = {0, 1};
	int export_parts[2] = {0, 1};

	post_migrate_pp_fn<int>(
		&dg,
		2,
		0,
		0,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		2,
		export_global_ids,
		nullptr,
		export_procs,
		export_parts,
		&err);

	/*
	ASSERT_EQ(dg.getNodes().size(), 1);
	ASSERT_EQ(dg.getArcs().size(), 0);
	ASSERT_EQ(dg.getNodes().at(2ul)->getIncomingArcs().size(), 0);
	ASSERT_EQ(dg.getNodes().at(2ul)->getOutgoingArcs().size(), 0);
	*/
}

