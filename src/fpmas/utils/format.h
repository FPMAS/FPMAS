#ifndef FPMAS_FORMAT_H
#define FPMAS_FORMAT_H

#include "fpmas/api/scheduler/scheduler.h"
#include <sstream>

namespace fpmas { namespace utils {
	template<typename T>
		std::string format(std::string input, std::string replace, T arg) {
			std::string str = input;
			auto _rank = str.find(replace);
			while(_rank != std::string::npos) {
				std::stringstream s_arg;
				s_arg << arg;
				str.replace(_rank, replace.size(), s_arg.str());
				_rank = str.find(replace, _rank+1);
			}
			return str;
		}
	std::string format(std::string input, int rank);
	std::string format(std::string input, api::scheduler::TimeStep time);
	std::string format(std::string input, int rank, api::scheduler::TimeStep time);
}}

#endif
