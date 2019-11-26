#include "gtest/gtest.h"

#include "communication/communication.h"
#include "environment/graph/parallel/distributed_graph.h"
#include "environment/graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"

#include "test_utils/test_utils.h"


using FPMAS::communication::MpiCommunicator;

class DistributeGraphTest : public ::testing::Test {
	protected:
		static MpiCommunicator* mpiCommunicator;
		static Zoltan* zz;

		DistributedGraph<int> dg = DistributedGraph<int>(zz);
		std::vector<int*> data;

		static void SetUpTestSuite() {
			mpiCommunicator = new MpiCommunicator();
			zz = new Zoltan(mpiCommunicator->getMpiComm());
			FPMAS::config::zoltan_config(zz);
		}


		static void TearDownTestSuite() {
			delete zz;
			delete mpiCommunicator;
		}

		void TearDown() override {
			for(auto d : data) {
				delete d;
			}
		}
};

MpiCommunicator* DistributeGraphTest::mpiCommunicator = nullptr;
Zoltan* DistributeGraphTest::zz = nullptr;

class Mpi_DistributeGraphWithoutArcTest : public DistributeGraphTest {
	protected:
		void SetUp() override {
			if(mpiCommunicator->getRank() == 0) {
				for (int i = 0; i < mpiCommunicator->getSize(); ++i) {
					data.push_back(new int(i));
					dg.buildNode((unsigned long) i, data.back());
				}
			}
		}
};

TEST_F(Mpi_DistributeGraphWithoutArcTest, distribute_without_arc_test) {
	if(mpiCommunicator->getRank() == 0) {
		ASSERT_EQ(dg.getNodes().size(), mpiCommunicator->getSize());
	}
	else {
		ASSERT_EQ(dg.getNodes().size(), 0);
	}

	int result = dg.distribute();

	ASSERT_EQ(result, ZOLTAN_OK);
	ASSERT_EQ(dg.getNodes().size(), 1);
}

class Mpi_DistributeGraphWithArcTest : public DistributeGraphTest {
	protected:

		void SetUp() override {
			if(mpiCommunicator->getRank() == 0) {
				for (int i = 0; i < mpiCommunicator->getSize(); ++i) {
					data.push_back(new int(2 * i));
					dg.buildNode((unsigned long) 2 * i, data.back());
					data.push_back(new int(2 * i + 1));
					dg.buildNode((unsigned long) 2 * i + 1, data.back());

					dg.link(2 * i, 2 * i + 1, i);
				}
			}
		}

};

TEST_F(Mpi_DistributeGraphWithArcTest, distribute_with_arc_test) {
	dg.distribute();

	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_EQ(dg.getArcs().size(), 1);

	Arc<int>* arc = dg.getArcs().begin()->second;
	for(auto node : dg.getNodes()) {
		if(node.second->getIncomingArcs().size() == 1) {
			ASSERT_EQ(arc->getTargetNode()->getId(), node.second->getId());
		}
		else {
			ASSERT_EQ(node.second->getIncomingArcs().size(), 0);
			ASSERT_EQ(node.second->getOutgoingArcs().size(), 1);
			ASSERT_EQ(arc->getSourceNode()->getId(), node.second->getId());
		}
	}
}
