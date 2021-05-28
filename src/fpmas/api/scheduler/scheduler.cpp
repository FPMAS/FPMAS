#include "scheduler.h"

namespace fpmas { namespace api { namespace scheduler {
	TimeStep time_step(Date date) {
		return std::floor(date);
	}

	SubTimeStep sub_time_step(Date date) {
		float int_part;
		return std::modf(date, &int_part);
	}

	Date sub_step_end(TimeStep step) {
		return std::nextafter((SubTimeStep) (step+1), (SubTimeStep) 0);
	}

}}}
