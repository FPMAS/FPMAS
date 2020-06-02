#include "scheduler/scheduler.h"

#include "../mocks/scheduler/mock_scheduler.h"

using FPMAS::scheduler::Epoch;

using ::testing::UnorderedElementsAreArray;
using ::testing::IsEmpty;

class EpochTest : public ::testing::Test {

	protected:
		Epoch epoch;
};

TEST_F(EpochTest, submit) {
	std::array<MockJob*, 6> jobs;
	for(auto job : jobs)
		epoch.submit(job);

	ASSERT_THAT(epoch.jobs(), UnorderedElementsAreArray(jobs));

}

TEST_F(EpochTest, clear) {
	std::array<MockJob*, 6> jobs;
	for(auto job : jobs)
		epoch.submit(job);

	epoch.clear();
	ASSERT_THAT(epoch.jobs(), IsEmpty());
}
