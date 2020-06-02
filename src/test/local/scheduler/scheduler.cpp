#include "scheduler/scheduler.h"

#include "gmock/gmock.h"

using FPMAS::scheduler::Scheduler;

using ::testing::ElementsAre;
using ::testing::IsEmpty;

class SchedulerTest : public ::testing::Test {
	protected:
		Scheduler scheduler;

};

TEST_F(SchedulerTest, schedule_unique_override) {
	scheduler.schedule(8, FPMAS::STACK);
	auto& job = scheduler.schedule(8, FPMAS::OVERRIDE);

	for(int i = 0; i < 8; i++) {
		ASSERT_THAT(scheduler.epoch(i).jobs(), IsEmpty());
	}
	ASSERT_THAT(scheduler.epoch(8).jobs(), ElementsAre(&job));

	for(int i = 9; i < 123; i++) {
		ASSERT_THAT(scheduler.epoch(i).jobs(), IsEmpty());
	}
}

TEST_F(SchedulerTest, schedule_unique_stack) {
	auto& job1 = scheduler.schedule(8, FPMAS::STACK);
	auto& job2 = scheduler.schedule(8, FPMAS::STACK);

	for(int i = 0; i < 8; i++) {
		ASSERT_THAT(scheduler.epoch(i).jobs(), IsEmpty());
	}
	ASSERT_THAT(scheduler.epoch(8).jobs(), ElementsAre(&job1, &job2));

	for(int i = 9; i < 123; i++) {
		ASSERT_THAT(scheduler.epoch(i).jobs(), IsEmpty());
	}
}

TEST_F(SchedulerTest, schedule_recurrent_from_start_override) {
	scheduler.schedule(0, 3, FPMAS::STACK);
	auto& job = scheduler.schedule(0, 3, FPMAS::OVERRIDE);

	for(int i = 0; i < 145; i++) {
		if(i%3==0) {
			ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&job));
		} else {
			ASSERT_THAT(scheduler.epoch(i).jobs(), IsEmpty());
		}
	}
}

TEST_F(SchedulerTest, schedule_recurrent_from_start_stack) {
	auto& job1 = scheduler.schedule(0, 3, FPMAS::STACK);
	auto& job2 = scheduler.schedule(0, 3, FPMAS::STACK);

	for(int i = 0; i < 145; i++) {
		if(i%3==0) {
			ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&job1, &job2));
		} else {
			ASSERT_THAT(scheduler.epoch(i).jobs(), IsEmpty());
		}
	}
}

TEST_F(SchedulerTest, schedule_multiple_from_start_override) {
	auto& job1 = scheduler.schedule(0, 1, FPMAS::STACK);
	auto& job2 = scheduler.schedule(0, 3, FPMAS::OVERRIDE);

	for(int i = 0; i < 145; i++) {
		if(i%3==0) {
			ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&job2));
		} else {
			ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&job1));
		}
	}

}

TEST_F(SchedulerTest, schedule_multiple_from_start_stack) {
	auto& job1 = scheduler.schedule(0, 1, FPMAS::STACK);
	auto& job2 = scheduler.schedule(0, 3, FPMAS::STACK);

	for(int i = 0; i < 145; i++) {
		if(i%3==0) {
			ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&job1, &job2));
		} else {
			ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&job1));
		}
	}

}

TEST_F(SchedulerTest, schedule_unique_on_multiple_stack) {
	auto& recurring_job = scheduler.schedule(0, 1, FPMAS::STACK);
	auto& unique_job =  scheduler.schedule(76, FPMAS::STACK);

	for(int i = 0; i < 76; i++) {
		ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&recurring_job));
	}
	ASSERT_THAT(scheduler.epoch(76).jobs(), ElementsAre(&recurring_job, &unique_job));
	for(int i = 77; i < 139; i++) {
		ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&recurring_job));
	}
}

TEST_F(SchedulerTest, schedule_unique_on_multiple_override) {
	auto& recurring_job = scheduler.schedule(0, 1, FPMAS::STACK);
	auto& unique_job =  scheduler.schedule(76, FPMAS::OVERRIDE);

	for(int i = 0; i < 76; i++) {
		ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&recurring_job));
	}
	ASSERT_THAT(scheduler.epoch(76).jobs(), ElementsAre(&unique_job));
	for(int i = 77; i < 139; i++) {
		ASSERT_THAT(scheduler.epoch(i).jobs(), ElementsAre(&recurring_job));
	}
}
