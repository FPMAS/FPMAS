#ifndef RUNTIME_API_H
#define RUNTIME_API_H

#include "scheduler/scheduler.h"

namespace fpmas::api::runtime {

	class Runtime {
		public:
			virtual void run(Date end) = 0;
			virtual void run(Date start, Date end) = 0;
			virtual fpmas::Date currentDate() const = 0;
	};
}
#endif
