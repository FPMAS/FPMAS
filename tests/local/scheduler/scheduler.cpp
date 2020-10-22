#include "fpmas/scheduler/scheduler.h"
#include "../mocks/scheduler/mock_scheduler.h"


using fpmas::scheduler::Scheduler;
using fpmas::scheduler::Date;
using fpmas::scheduler::TimeStep;
using fpmas::scheduler::SubTimeStep;

using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Ref;
using ::testing::StrictMock;
using ::testing::FloatNear;
using ::testing::Matcher;

#define SUB_STEP_APPROXIMATION 10e-5
#define SUB_STEP_NEAR(SUB_STEP) FloatNear(SUB_STEP, SUB_STEP_APPROXIMATION)

TEST(BaseSchedulerTest, schedule) {
	Scheduler scheduler;
	StrictMock<MockEpoch> epoch;
	EXPECT_CALL(epoch, clear);
	scheduler.build(8, epoch);
}

class SchedulerTest : public ::testing::Test {
	protected:
		Scheduler scheduler;
		StrictMock<MockEpoch> epoch;
		typedef fpmas::api::scheduler::Epoch::JobList JobList;

		void SetUp() override {
			EXPECT_CALL(epoch, clear).Times(AnyNumber());
		}
};

MATCHER_P(RefWrapper, job, "") {return &job.get() == &arg.get();}
#define JOB_WRAPPER(job) Matcher<JobList>(ElementsAre(RefWrapper(std::ref(job))))

TEST_F(SchedulerTest, schedule_unique) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(8.3, job1);
	scheduler.schedule(8.1, job2);
	scheduler.schedule(12, job3);

	for(int i = 0; i < 8; i++) {
		scheduler.build(i, epoch);
	}
	EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(.3)));
	EXPECT_CALL(epoch, submit(JOB_WRAPPER(job2), SUB_STEP_NEAR(.1)));
	scheduler.build(8, epoch);

	for(int i = 9; i < 12; i++) {
		scheduler.build(i, epoch);
	}
	EXPECT_CALL(epoch, submit(JOB_WRAPPER(job3), SUB_STEP_NEAR(0)));
	scheduler.build(12, epoch);

	for(int i = 13; i < 123; i++) {
		scheduler.build(i, epoch);
	}
}

TEST_F(SchedulerTest, schedule_unique_list) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(8.3, {job1, job2});
	scheduler.schedule(12, job3);

	for(int i = 0; i < 8; i++) {
		scheduler.build(i, epoch);
	}
	EXPECT_CALL(epoch, submit(
				Matcher<JobList>(ElementsAre(RefWrapper(std::ref(job1)), RefWrapper(std::ref(job2)))),
				SUB_STEP_NEAR(.3)));
	scheduler.build(8, epoch);

	for(int i = 9; i < 12; i++) {
		scheduler.build(i, epoch);
	}
	EXPECT_CALL(epoch, submit(JOB_WRAPPER(job3), SUB_STEP_NEAR(0)));
	scheduler.build(12, epoch);

	for(int i = 13; i < 123; i++) {
		scheduler.build(i, epoch);
	}
}

TEST_F(SchedulerTest, schedule_recurrent_from_start) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(0.7, 2, job1);
	scheduler.schedule(0.2, 2, job2);
	scheduler.schedule(0.5, 5, job3);

	for(TimeStep step = 0; step < 124; step++) {
		if(step%2==0 && step%5==0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(.7)));
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job2), SUB_STEP_NEAR(.2)));
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job3), SUB_STEP_NEAR(.5)));
		} else if (step%2==0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(.7)));
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job2), SUB_STEP_NEAR(.2)));
		} else if (step%5==0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job3), SUB_STEP_NEAR(.5)));
		}
		scheduler.build(step, epoch);
	}
}

TEST_F(SchedulerTest, schedule_recurrent_list_from_start) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(0.7, 2, {job1, job2});
	scheduler.schedule(0.5, 5, job3);

	for(TimeStep step = 0; step < 124; step++) {
		if(step%2==0 && step%5==0) {
			EXPECT_CALL(epoch, submit(
				Matcher<JobList>(ElementsAre(RefWrapper(std::ref(job1)), RefWrapper(std::ref(job2)))),
				SUB_STEP_NEAR(.7)));
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job3), SUB_STEP_NEAR(.5)));
		} else if (step%2==0) {
			EXPECT_CALL(epoch, submit(
				Matcher<JobList>(ElementsAre(RefWrapper(std::ref(job1)), RefWrapper(std::ref(job2)))),
				SUB_STEP_NEAR(.7)));
		} else if (step%5==0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job3), SUB_STEP_NEAR(.5)));
		}
		scheduler.build(step, epoch);
	}
}

TEST_F(SchedulerTest, schedule_recurrent_with_start) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(0, 2, job1);
	scheduler.schedule(13, 2, job2);
	scheduler.schedule(6, 5, job3);

	for(TimeStep step = 0; step < 6; step++) {
		if(step % 2 == 0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(0)));
		}
		scheduler.build(step, epoch);
	}
	for(TimeStep step = 6; step < 12; step++) {
		if (step % 2 == 0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(0)));
		}
		if ((step-6) % 5 == 0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job3), SUB_STEP_NEAR(0)));
		}
		scheduler.build(step, epoch);
	}
	for(TimeStep step = 13; step < 154; step++) {
		if(step % 2 == 0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(0)));
		}
		if((step - 6) % 5 == 0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job3), SUB_STEP_NEAR(0)));
		}
		if((step - 13) % 2 == 0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job2), SUB_STEP_NEAR(0)));
		}
		scheduler.build(step, epoch);
	}
}

TEST_F(SchedulerTest, schedule_unique_on_recurrent) {
	MockJob job1;
	MockJob job2;
	scheduler.schedule(23.4, 2, job1);
	scheduler.schedule(29.045, job2);

	for (TimeStep step = 0; step < 22; ++step) {
		scheduler.build(step, epoch);
	}
	for (TimeStep step = 23; step < 29; ++step) {
		if((step - 23) % 2 == 0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(.4)));
		}
		scheduler.build(step, epoch);
	}
	EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(.4)));
	EXPECT_CALL(epoch, submit(JOB_WRAPPER(job2), SUB_STEP_NEAR(.045)));
	scheduler.build(29, epoch);
	for (TimeStep step = 30; step < 292; ++step) {
		if((step-23) % 2 == 0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(.4)));
		}
		scheduler.build(step, epoch);
	}
}

TEST_F(SchedulerTest, schedule_limited_recurrent) {
	MockJob job1;

	scheduler.schedule(23.7, 40, 5, job1);

	for (TimeStep step = 0; step < 22; ++step) {
		scheduler.build(step, epoch);
	}
	for (TimeStep step = 23; step < 40; ++step) {
		if((step-23) % 5 == 0) {
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(.7)));
		}
		scheduler.build(step, epoch);
	}
	for (TimeStep step = 40; step < 60; ++step) {
		scheduler.build(step, epoch);
	}
}

TEST_F(SchedulerTest, schedule_limited_recurrent_list) {
	MockJob job1;
	MockJob job2;

	scheduler.schedule(23.7, 40, 5, {job1, job2});

	for (TimeStep step = 0; step < 22; ++step) {
		scheduler.build(step, epoch);
	}
	for (TimeStep step = 23; step < 40; ++step) {
		if((step-23) % 5 == 0) {
			EXPECT_CALL(epoch, submit(
						Matcher<JobList>(ElementsAre(RefWrapper(std::ref(job1)), RefWrapper(std::ref(job2)))),
						SUB_STEP_NEAR(.7)));
		}
		scheduler.build(step, epoch);
	}
	for (TimeStep step = 40; step < 60; ++step) {
		scheduler.build(step, epoch);
	}
}


TEST_F(SchedulerTest, schedule_recurrent_limited_recurrent_and_unique) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(0, 2, job1);
	scheduler.schedule(13, 22, 2, job2);
	scheduler.schedule(17, job3);

	for (TimeStep step = 0; step < 13; ++step) {
		if(step % 2 == 0)
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(0)));
		scheduler.build(step, epoch);
	}
	for (TimeStep step = 13; step < 17; ++step) {
		if(step % 2 == 0)
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(0)));
		if((step-13) % 2 == 0)
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job2), SUB_STEP_NEAR(0)));
		scheduler.build(step, epoch);
	}

	EXPECT_CALL(epoch, submit(JOB_WRAPPER(job2), SUB_STEP_NEAR(0)));
	EXPECT_CALL(epoch, submit(JOB_WRAPPER(job3), SUB_STEP_NEAR(0)));
	scheduler.build(17, epoch);

	for (TimeStep step = 18; step < 22; ++step) {
		if(step % 2 == 0)
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(0)));
		if((step-13) % 2 == 0)
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job2), SUB_STEP_NEAR(0)));
		scheduler.build(step, epoch);
	}
	for (TimeStep step = 23; step < 67; ++step) {
		if(step % 2 == 0)
			EXPECT_CALL(epoch, submit(JOB_WRAPPER(job1), SUB_STEP_NEAR(0)));
		scheduler.build(step, epoch);
	}
}
