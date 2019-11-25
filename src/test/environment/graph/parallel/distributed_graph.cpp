#include "gtest/gtest.h"

#include "communication/communication.h"
#include "environment/graph/parallel/distributed_graph.h"
#include "environment/graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"

#include "test_utils/test_utils.h"


using FPMAS::communication::MpiCommunicator;

class Mpi_DistributeGraphTest : public ::testing::Test {
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

		void SetUp() override {
			if(mpiCommunicator->getRank() == 0) {
				for (int i = 0; i < mpiCommunicator->getSize(); ++i) {
					data.push_back(new int(i));
					dg.buildNode((unsigned long) i, data.back());
				}
			}
		}

		void TearDown() override {
			for(auto d : data) {
				delete d;
			}
		}


		static void TearDownTestSuite() {
			delete zz;
			delete mpiCommunicator;
		}
};

MpiCommunicator* Mpi_DistributeGraphTest::mpiCommunicator = nullptr;
Zoltan* Mpi_DistributeGraphTest::zz = nullptr;

TEST_F(Mpi_DistributeGraphTest, distribute_test) {
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
