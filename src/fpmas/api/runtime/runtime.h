#ifndef RUNTIME_API_H
#define RUNTIME_API_H

#include "fpmas/api/scheduler/scheduler.h"

namespace fpmas { namespace api { namespace runtime {
	using api::scheduler::Date;

	class Runtime {
		public:
			virtual void run(Date end) = 0;
			virtual void run(Date start, Date end) = 0;
			virtual Date currentDate() const = 0;
	};
}}}
#endif
