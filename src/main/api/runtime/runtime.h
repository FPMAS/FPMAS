#ifndef RUNTIME_API_H
#define RUNTIME_API_H

#include "scheduler/scheduler.h"

namespace FPMAS::api::runtime {

	class Runtime {
		public:
			virtual void execute(Date end) = 0;
	};
}
#endif
