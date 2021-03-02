#include "format.h"

namespace fpmas { namespace utils {
	std::string format(std::string input, int rank) {
		return format<int>(input, "%r", rank);
	}
	std::string format(std::string input, api::scheduler::TimeStep time) {
		return format<api::scheduler::TimeStep>(input, "%t", time);
	}
	std::string format(std::string input, int rank, api::scheduler::TimeStep time) {
		std::string str = format(input, rank);
		return format(str, time);
	}
}}
