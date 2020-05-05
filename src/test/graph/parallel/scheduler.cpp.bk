#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "graph/parallel/scheduler.h"

using FPMAS::graph::parallel::Scheduler;
class FakeData {
	public:
		int _schedule;
		int schedule() const {return _schedule;}
};

class MockNode {
	public:
		FakeData fakeData;
	MOCK_METHOD(FakeData*, data, (), (const));
};

using testing::Return;

class SchedulerTest : public ::testing::Test {
	protected:
		Scheduler<MockNode> scheduler;
		std::array<MockNode, 3> mockNodes;

		void SetUp() override {
			for(auto& mock : mockNodes) {
				EXPECT_CALL(mock, data())
					.WillRepeatedly(Return(&mock.fakeData));
			}
			scheduler.setSchedule(5, &mockNodes[0]);
		}
};

TEST_F(SchedulerTest, set_schedule) {
	ASSERT_EQ(scheduler.getSchedule(&mockNodes[0]), 5);
}

TEST_F(SchedulerTest, get) {
	auto& schedule = scheduler.get();
	ASSERT_EQ(schedule.size(), 1);
	ASSERT_EQ(schedule.at(5).size(), 1);
	ASSERT_EQ(schedule.at(5).count(&mockNodes[0]), 1);

	scheduler.setSchedule(5, &mockNodes[1]);
	scheduler.setSchedule(8, &mockNodes[2]);

	ASSERT_EQ(schedule.size(), 2);
	ASSERT_EQ(schedule.at(5).size(), 2);
	ASSERT_EQ(schedule.at(5).count(&mockNodes[0]), 1);
	ASSERT_EQ(schedule.at(5).count(&mockNodes[1]), 1);
	ASSERT_EQ(schedule.at(8).size(), 1);
	ASSERT_EQ(schedule.at(8).count(&mockNodes[2]), 1);
}

TEST_F(SchedulerTest, get_time) {
	scheduler.setSchedule(5, &mockNodes[1]);
	scheduler.setSchedule(8, &mockNodes[2]);

	ASSERT_EQ(scheduler.get(5).size(), 2);
	ASSERT_EQ(scheduler.get(5).count(&mockNodes[0]), 1);
	ASSERT_EQ(scheduler.get(5).count(&mockNodes[1]), 1);
	ASSERT_EQ(scheduler.get(8).size(), 1);
	ASSERT_EQ(scheduler.get(8).count(&mockNodes[2]), 1);
}

TEST_F(SchedulerTest, unschedule) {
	scheduler.unschedule(&mockNodes[0]);
	ASSERT_EQ(scheduler.get(5).size(), 0);
}

class Mpi_SchedulerTest : public ::testing::Test {
	protected:
		Scheduler<MockNode> scheduler;

		void SetUp() override {
			int rank;
			MPI_Comm_rank(MPI_COMM_WORLD, &rank);
			MockNode mockNodes[rank+1];
			//mockNodes.resize(rank);
			for (int i = 0; i <= rank; i++) {
				EXPECT_CALL(mockNodes[i], data())
					.WillRepeatedly(Return(&mockNodes[i].fakeData));
				scheduler.setSchedule(i, &mockNodes[i]);
			}
		}
};

TEST_F(Mpi_SchedulerTest, gather_periods) {
	auto resultPeriods = scheduler.gatherPeriods(MPI_COMM_WORLD);
	std::set<int> expectedPeriods;

	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	for (int i = 0; i < size; i++) {
		expectedPeriods.insert(i);
	}
	ASSERT_EQ(resultPeriods, expectedPeriods);
};
