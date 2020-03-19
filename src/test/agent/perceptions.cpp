#include "gtest/gtest.h"

#include "example_agents.h"

class Mpi_PerceptionsTest : public ::testing::Test {
	protected:
		Environment<GhostMode, 2, Wolf, Sheep> env;
		node_ptr wolf_1;
		node_ptr wolf_2;
		node_ptr wolf_3;
		node_ptr sheep_1;
		node_ptr sheep_2;
	
	void SetUp() override {
		wolf_1 = env.buildNode(0, std::unique_ptr<PreyPredAgent>(new Wolf()));
		wolf_2 = env.buildNode(1, std::unique_ptr<PreyPredAgent>(new Wolf()));
		wolf_3 = env.buildNode(2, std::unique_ptr<PreyPredAgent>(new Wolf()));
		sheep_1 = env.buildNode(3, std::unique_ptr<PreyPredAgent>(new Sheep()));
		sheep_2 = env.buildNode(4, std::unique_ptr<PreyPredAgent>(new Sheep()));

		env.link(wolf_1->getId(), sheep_1->getId(), 0, Layer_A);
		env.link(wolf_1->getId(), sheep_2->getId(), 1, Layer_A);
		env.link(wolf_1->getId(), wolf_2->getId(), 2, Layer_B);
		env.link(wolf_3->getId(), wolf_1->getId(), 2, Layer_B);
	}


};

TEST_F(Mpi_PerceptionsTest, single_layer_perception) {
	auto perceptions = wolf_1->data()->read()->perceptions<Layer_A>(wolf_1);
	ASSERT_EQ(perceptions.get().size(), 2);
}

TEST_F(Mpi_PerceptionsTest, multiple_layer_perception) {
	auto perceptions = wolf_1->data()->read()->perceptions<Layer_A, Layer_B>(wolf_1);
	ASSERT_EQ(perceptions.get().size(), 3);
}
