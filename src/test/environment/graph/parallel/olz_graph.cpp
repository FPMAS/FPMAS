#include "gtest/gtest.h"

#include "environment/graph/parallel/distributed_graph.h"

class DistributeGraphTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();
		std::vector<int*> data;
		void TearDown() override {
			for(auto d : data) {
				delete d;
			}
		}
};

