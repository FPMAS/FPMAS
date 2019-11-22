#include "gtest/gtest.h"

#include "environment/graph/parallel/olz.h"
#include "communication/communication.h"
#include "utils/config.h"

using FPMAS::communication::MpiCommunicator;

using FPMAS::graph::DistributedGraph;

class Mpi_OlzTest : ::testing::Test {
	protected:
		static Zoltan* zz;
		static MpiCommunicator* mpiCommunicator;
	
		static void SetUpTestSuite() {
			mpiCommunicator = new MpiCommunicator();
			zz = new Zoltan(mpiCommunicator->getMpiComm());
			FPMAS::config::zoltan_config(zz);
		}

		DistributedGraph<int> dg = DistributedGraph<int>(zz);

		void SetUp() override {

		}
};

TEST_F(Mpi_OlzTest, simpleGhostNodeTest) {
	

}
