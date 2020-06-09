#include "scheduler/scheduler.h"
#include "../mocks/scheduler/mock_scheduler.h"


using FPMAS::scheduler::Scheduler;

using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Ref;
using ::testing::StrictMock;

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

		void SetUp() override {
			EXPECT_CALL(epoch, clear).Times(AnyNumber());
		}
};

TEST_F(SchedulerTest, schedule_unique) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(8, job1);
	scheduler.schedule(8, job2);
	scheduler.schedule(12, job3);

	for(int i = 0; i < 8; i++) {
		scheduler.build(i, epoch);
	}
	EXPECT_CALL(epoch, submit(Ref(job1)));
	EXPECT_CALL(epoch, submit(Ref(job2)));
	scheduler.build(8, epoch);

	for(int i = 9; i < 12; i++) {
		scheduler.build(i, epoch);
	}
	EXPECT_CALL(epoch, submit(Ref(job3)));
	scheduler.build(12, epoch);

	for(int i = 13; i < 123; i++) {
		scheduler.build(i, epoch);
	}
}

TEST_F(SchedulerTest, schedule_recurrent_from_start) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(0, 2, job1);
	scheduler.schedule(0, 2, job2);
	scheduler.schedule(0, 5, job3);

	for(FPMAS::Date date = 0; date < 124; date++) {
		if(date%2==0 && date%5==0) {
			EXPECT_CALL(epoch, submit(Ref(job1)));
			EXPECT_CALL(epoch, submit(Ref(job2)));
			EXPECT_CALL(epoch, submit(Ref(job3)));
		} else if (date%2==0) {
			EXPECT_CALL(epoch, submit(Ref(job1)));
			EXPECT_CALL(epoch, submit(Ref(job2)));
		} else if (date%5==0) {
			EXPECT_CALL(epoch, submit(Ref(job3)));
		}
		scheduler.build(date, epoch);
	}
}

TEST_F(SchedulerTest, schedule_recurrent_with_start) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(0, 2, job1);
	scheduler.schedule(13, 2, job2);
	scheduler.schedule(6, 5, job3);

	for(FPMAS::Date date = 0; date < 6; date++) {
		if(date % 2 == 0) {
			EXPECT_CALL(epoch, submit(Ref(job1)));
		}
		scheduler.build(date, epoch);
	}
	for(FPMAS::Date date = 6; date < 12; date++) {
		if (date % 2 == 0) {
			EXPECT_CALL(epoch, submit(Ref(job1)));
		}
		if ((date-6) % 5 == 0) {
			EXPECT_CALL(epoch, submit(Ref(job3)));
		}
		scheduler.build(date, epoch);
	}
	for(FPMAS::Date date = 13; date < 154; date++) {
		if(date % 2 == 0) {
			EXPECT_CALL(epoch, submit(Ref(job1)));
		}
		if((date - 6) % 5 == 0) {
			EXPECT_CALL(epoch, submit(Ref(job3)));
		}
		if((date - 13) % 2 == 0) {
			EXPECT_CALL(epoch, submit(Ref(job2)));
		}
		scheduler.build(date, epoch);
	}
}

TEST_F(SchedulerTest, schedule_unique_on_recurrent) {
	MockJob job1;
	MockJob job2;
	scheduler.schedule(23, 2, job1);
	scheduler.schedule(29, job2);

	for (FPMAS::Date date = 0; date < 22; ++date) {
		scheduler.build(date, epoch);
	}
	for (FPMAS::Date date = 23; date < 29; ++date) {
		if((date - 23) % 2 == 0) {
			EXPECT_CALL(epoch, submit(Ref(job1)));
		}
		scheduler.build(date, epoch);
	}
	EXPECT_CALL(epoch, submit(Ref(job1)));
	EXPECT_CALL(epoch, submit(Ref(job2)));
	scheduler.build(29, epoch);
	for (FPMAS::Date date = 30; date < 292; ++date) {
		if((date-23) % 2 == 0) {
			EXPECT_CALL(epoch, submit(Ref(job1)));
		}
		scheduler.build(date, epoch);
	}
}

TEST_F(SchedulerTest, schedule_limited_recurrent) {
	MockJob job1;

	scheduler.schedule(23, 40, 5, job1);

	for (FPMAS::Date date = 0; date < 22; ++date) {
		scheduler.build(date, epoch);
	}
	for (FPMAS::Date date = 23; date < 40; ++date) {
		if((date-23) % 5 == 0) {
			EXPECT_CALL(epoch, submit(Ref(job1)));
		}
		scheduler.build(date, epoch);
	}
	for (FPMAS::Date date = 40; date < 60; ++date) {
		scheduler.build(date, epoch);
	}
}

TEST_F(SchedulerTest, schedule_recurrent_limited_recurrent_and_unique) {
	MockJob job1;
	MockJob job2;
	MockJob job3;
	scheduler.schedule(0, 2, job1);
	scheduler.schedule(13, 22, 2, job2);
	scheduler.schedule(17, job3);

	for (FPMAS::Date date = 0; date < 13; ++date) {
		if(date % 2 == 0)
			EXPECT_CALL(epoch, submit(Ref(job1)));
		scheduler.build(date, epoch);
	}
	for (FPMAS::Date date = 13; date < 17; ++date) {
		if(date % 2 == 0)
			EXPECT_CALL(epoch, submit(Ref(job1)));
		if((date-13) % 2 == 0)
			EXPECT_CALL(epoch, submit(Ref(job2)));
		scheduler.build(date, epoch);
	}

	EXPECT_CALL(epoch, submit(Ref(job2)));
	EXPECT_CALL(epoch, submit(Ref(job3)));
	scheduler.build(17, epoch);

	for (FPMAS::Date date = 18; date < 22; ++date) {
		if(date % 2 == 0)
			EXPECT_CALL(epoch, submit(Ref(job1)));
		if((date-13) % 2 == 0)
			EXPECT_CALL(epoch, submit(Ref(job2)));
		scheduler.build(date, epoch);
	}
	for (FPMAS::Date date = 23; date < 67; ++date) {
		if(date % 2 == 0)
			EXPECT_CALL(epoch, submit(Ref(job1)));
		scheduler.build(date, epoch);
	}
}
