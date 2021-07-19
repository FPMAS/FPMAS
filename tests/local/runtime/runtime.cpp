#include "scheduler/mock_scheduler.h"
#include "fpmas.h"

using namespace testing;

using fpmas::runtime::Runtime;
using fpmas::runtime::Date;

class RuntimeTest : public ::testing::Test {
	protected:
		MockScheduler scheduler;
		Runtime runtime {scheduler};
		std::array<MockTask, 4> mock_tasks;
		std::array<MockJob, 3> mock_jobs;
		std::vector<fpmas::api::scheduler::Task*> job_0_tasks;
		std::vector<fpmas::api::scheduler::Task*> job_1_tasks;
		std::vector<fpmas::api::scheduler::Task*> job_2_tasks;

		std::vector<MockJob*> epoch_0_jobs;
		std::vector<MockJob*> epoch_1_jobs;
		std::vector<MockJob*> epoch_2_jobs;

		void SetUp() override {

			for(MockJob& job : mock_jobs) {
				EXPECT_CALL(job, getBeginTask)
					.Times(AnyNumber())
					.WillRepeatedly(ReturnRef(mock_tasks[0]));
				EXPECT_CALL(job, getEndTask)
					.Times(AnyNumber())
					.WillRepeatedly(ReturnRef(mock_tasks[3]));
			}
			// Job 0 has no tasks

			// Adds tasks to Job 1
			job_1_tasks.push_back(&mock_tasks[2]);
			job_1_tasks.push_back(&mock_tasks[0]);

			// Adds tasks to Job 2
			job_2_tasks.push_back(&mock_tasks[0]);
			job_2_tasks.push_back(&mock_tasks[2]);
			job_2_tasks.push_back(&mock_tasks[1]);
			job_2_tasks.push_back(&mock_tasks[3]);

			setUpJob(static_cast<MockJob&>(mock_jobs[0]), job_0_tasks);
			setUpJob(static_cast<MockJob&>(mock_jobs[1]), job_1_tasks);
			setUpJob(static_cast<MockJob&>(mock_jobs[2]), job_2_tasks);

			// Adds jobs to epoch 0
			epoch_0_jobs.push_back(&mock_jobs[0]);

			// Epoch 1 has no jobs

			// Adds jobs to epoch 2
			epoch_2_jobs.push_back(&mock_jobs[1]);
			epoch_2_jobs.push_back(&mock_jobs[0]);
			epoch_2_jobs.push_back(&mock_jobs[2]);
		}
		void expectTasks(
				Sequence& sequence,
				std::vector<fpmas::api::scheduler::Task*> job_tasks,
				std::vector<fpmas::api::scheduler::Task*>& call_order
				) {
			Expectation _job_begin = EXPECT_CALL(
					mock_tasks[0], run)
				.InSequence(sequence);
			ExpectationSet _job_tasks;
			for(fpmas::api::scheduler::Task* task : job_tasks)
				_job_tasks += EXPECT_CALL(
						*static_cast<MockTask*>(task), run)
					.After(_job_begin)
					.WillOnce(Invoke([&call_order, task]() {
								call_order.push_back(task);
								}));

			EXPECT_CALL(
					mock_tasks[3], run)
				.InSequence(sequence)
				.After(_job_tasks);
		}
		void expectTasks(
				Sequence& sequence,
				std::vector<fpmas::api::scheduler::Task*> job_tasks
				) {
			static std::vector<fpmas::api::scheduler::Task*> call_order;
			expectTasks(sequence, job_tasks, call_order);
			call_order.clear();
		}

	private:
		void setUpJob(MockJob& job, std::vector<fpmas::api::scheduler::Task*>& tasks) {
				EXPECT_CALL(job, tasks)
					.Times(AnyNumber())
					.WillRepeatedly(ReturnRef(tasks));
				EXPECT_CALL(job, begin)
					.Times(AnyNumber())
					.WillRepeatedly(Return(tasks.begin()));
				EXPECT_CALL(job, end)
					.Times(AnyNumber())
					.WillRepeatedly(Return(tasks.end()));
		}
};

class MockBuildEpoch {
	private:
		std::vector<MockJob*> jobs;
	public:
		MockBuildEpoch(std::vector<MockJob*> jobs) : jobs(jobs) {}
		void operator()(fpmas::scheduler::TimeStep, fpmas::api::scheduler::Epoch& epoch) {
			epoch.clear();
			for(MockJob* job : jobs)
				epoch.submit(*job, 0);
		}
};

TEST_F(RuntimeTest, run) {
	Sequence seq;

	Sequence epoch_0;
	// Epoch 0
	EXPECT_CALL(scheduler, build(0, _))
		.InSequence(seq)
		.WillOnce(Invoke(MockBuildEpoch(epoch_0_jobs)));
		//.WillOnce(SetArgReferee<1>(epoch0));

	expectTasks(seq, job_0_tasks);

	// Epoch 1
	Expectation epoch_1 = EXPECT_CALL(scheduler, build(1, _))
		.InSequence(seq)
		.WillOnce(Invoke(MockBuildEpoch(epoch_1_jobs)));
	// Nothing
	
	Sequence epoch_2;
	// Epoch 2
	EXPECT_CALL(scheduler, build(2, _))
		.InSequence(seq)
		.WillOnce(Invoke(MockBuildEpoch(epoch_2_jobs)));
	expectTasks(seq, job_1_tasks);
	expectTasks(seq, job_0_tasks);
	expectTasks(seq, job_2_tasks);

	runtime.run(0, 3);
}

TEST_F(RuntimeTest, execute_job) {
	Sequence seq;
	expectTasks(seq, job_0_tasks);

	runtime.execute(mock_jobs[0]);
}

TEST_F(RuntimeTest, execute_job_list) {
	fpmas::api::scheduler::JobList job_list;
	job_list.push_back(mock_jobs[0]);
	job_list.push_back(mock_jobs[1]);
	job_list.push_back(mock_jobs[2]);
	Sequence seq;
	expectTasks(seq, job_0_tasks);
	expectTasks(seq, job_1_tasks);
	expectTasks(seq, job_2_tasks);

	runtime.execute(job_list);
}

TEST_F(RuntimeTest, seed) {
	fpmas::api::scheduler::JobList job_list;
	job_list.push_back(mock_jobs[0]);
	job_list.push_back(mock_jobs[1]);
	job_list.push_back(mock_jobs[2]);


	// Base seed
	fpmas::seed(0);

	Sequence seq1;
	std::vector<fpmas::api::scheduler::Task*> call_order_1;
	expectTasks(seq1, job_0_tasks, call_order_1);
	expectTasks(seq1, job_1_tasks, call_order_1);
	expectTasks(seq1, job_2_tasks, call_order_1);

	runtime.execute(job_list);

	// Seed update: should produce a different execution order
	fpmas::seed(1);
	Sequence seq2;
	std::vector<fpmas::api::scheduler::Task*> call_order_2;
	expectTasks(seq2, job_0_tasks, call_order_2);
	expectTasks(seq2, job_1_tasks, call_order_2);
	expectTasks(seq2, job_2_tasks, call_order_2);

	runtime.execute(job_list);

	ASSERT_THAT(call_order_1, Not(ElementsAreArray(call_order_2)));


	// Get back to base seed: should ensure reproducibility
	fpmas::seed(0);
	Sequence seq3;
	std::vector<fpmas::api::scheduler::Task*> call_order_3;
	expectTasks(seq3, job_0_tasks, call_order_3);
	expectTasks(seq3, job_1_tasks, call_order_3);
	expectTasks(seq3, job_2_tasks, call_order_3);

	runtime.execute(job_list);

	ASSERT_THAT(call_order_3, ElementsAreArray(call_order_1));
}
