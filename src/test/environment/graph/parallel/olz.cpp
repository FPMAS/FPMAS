#include "gtest/gtest.h"

#include <typeinfo>

#include "environment/graph/parallel/olz.h"
#include "communication/communication.h"
#include "utils/config.h"

using FPMAS::communication::MpiCommunicator;

using FPMAS::graph::DistributedGraph;

class Mpi_OlzTest : public ::testing::Test {
	protected:
		static Zoltan* zz;
		static MpiCommunicator* mpiCommunicator;

		static std::array<int*, 3> data;
	
		static void SetUpTestSuite() {
			mpiCommunicator = new MpiCommunicator();
			zz = new Zoltan(mpiCommunicator->getMpiComm());
			FPMAS::config::zoltan_config(zz);
			for (int i = 0; i < 3; ++i) {
				data[i] = new int(i);
			}
		}

		DistributedGraph<int> dg = DistributedGraph<int>(zz);

		void SetUp() override {
			dg.buildNode(0ul, data[0]);
			dg.buildNode(1ul, data[1]);
			dg.buildNode(2ul, data[2]);

			dg.link(0ul, 2ul, 0ul);
			dg.link(2ul, 1ul, 1ul);

		}

		static void TearDownTestSuite() {
			delete zz;
			delete mpiCommunicator;
			for(auto d : data) {
				delete d;
			}
		}
};
MpiCommunicator* Mpi_OlzTest::mpiCommunicator = nullptr;
Zoltan* Mpi_OlzTest::zz = nullptr;
std::array<int*, 3> Mpi_OlzTest::data;

TEST_F(Mpi_OlzTest, simpleGhostNodeTest) {
	// Builds ghost node from node 2
	Node<int> node = *(dg.getNode(2ul));	
	dg.buildGhostNode(node, 0);

	// Node 0 is linked to the ghost node
	ASSERT_EQ(dg.getNode(0ul)->getOutgoingArcs().size(), 2);
	// Node 1 is linked to the ghost node
	ASSERT_EQ(dg.getNode(1ul)->getIncomingArcs().size(), 2);
	// Nothing has changed for node 2
	ASSERT_EQ(dg.getNode(2ul)->getIncomingArcs().size(), 1);
	ASSERT_EQ(dg.getNode(2ul)->getOutgoingArcs().size(), 1);

	// Removes the real node
	dg.removeNode(2ul);

	// Only the ghost node persist
	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_EQ(dg.getNode(0ul)->getOutgoingArcs().size(), 1);
	ASSERT_EQ(dg.getNode(0ul)->getIncomingArcs().size(), 0);
	ASSERT_EQ(dg.getNode(1ul)->getIncomingArcs().size(), 1);
	ASSERT_EQ(dg.getNode(1ul)->getOutgoingArcs().size(), 0);

	// Ghost node data is accessible from node 0
	ASSERT_EQ(
			*(dg.getNode(0ul)->getOutgoingArcs().at(0)->getTargetNode()->getData()),
			2
			);
	ASSERT_EQ(
			dg.getNode(0ul)->getOutgoingArcs().at(0)->getTargetNode()->getId(),
			2ul
			);

	// Ghost node data is accessible from node 1
	ASSERT_EQ(
			*(dg.getNode(1ul)->getIncomingArcs().at(0)->getSourceNode()->getData()),
			2
			);
	ASSERT_EQ(
			dg.getNode(1ul)->getIncomingArcs().at(0)->getSourceNode()->getId(),
			2ul
			);

}
