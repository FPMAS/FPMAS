#ifndef FPMAS_RUNTIME_H
#define FPMAS_RUNTIME_H

/** \file src/fpmas/runtime/runtime.h
 * Runtime implementation.
 */

#include "fpmas/api/runtime/runtime.h"
#include "fpmas/scheduler/scheduler.h"

#include <random>

namespace fpmas { namespace runtime {
	using api::runtime::Date;

	/**
	 * api::runtime::Runtime implementation.
	 */
	class Runtime : public api::runtime::Runtime {
		private:
			api::scheduler::Scheduler& scheduler;
			scheduler::Epoch epoch;
			Date date = 0;
			std::mt19937 rd;

		public:
			/**
			 * Runtime constructor.
			 *
			 * @param scheduler scheduler to execute
			 */
			Runtime(api::scheduler::Scheduler& scheduler)
				: scheduler(scheduler) {}

			void run(Date end) override;
			void run(Date start, Date end) override;
			void execute(const api::scheduler::Job& job) override;
			void execute(const api::scheduler::JobList& job_list) override;
			Date currentDate() const override {return date;}
			void setCurrentDate(Date date) override {this->date = date;}
	};
}}
#endif
