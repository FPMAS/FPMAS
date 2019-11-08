#include "gtest/gtest.h"
#include "mpi.h"

class MpiEnvironment : public ::testing::Environment {
	private:
		int argc;
		char **argv;

	public:
		MpiEnvironment(int argc, char **argv) : argc(argc), argv(argv){};

		virtual ~MpiEnvironment() {}

		void SetUp() override {
			MPI::Init(this->argc, this->argv);
		}

		void TearDown() override {
			MPI::Finalize();
		}
};

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	// MpiEnvironment env = MpiEnvironment(argc, argv);
	// ::testing::AddGlobalTestEnvironment(&env);
	MPI::Init(argc, argv);
	int result;
	if(MPI::COMM_WORLD.Get_rank() == 0)
		result =  RUN_ALL_TESTS();
	MPI::Finalize();
	return result;
}
