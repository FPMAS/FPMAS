#ifndef FPMAS_FORMAT_H
#define FPMAS_FORMAT_H

/**
 * \file src/fpmas/utils/format.h
 *
 * Defines some string format utilities.
 */

#include "fpmas/api/scheduler/scheduler.h"
#include <sstream>

namespace fpmas { namespace utils {
	/**
	 * Replaces all the occurences of `replace` in the `input` string by `arg`,
	 * serialized using the output stream operator `<<`.
	 *
	 * @param input input string
	 * @param replace sequence to replace
	 * @param arg replacement
	 * @return formatted string
	 */
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
	/**
	 * Replaces `"%r"` sequences in `input` by `rank`.
	 *
	 * @param input input string
	 * @param rank replacement rank
	 * @return formatted string
	 */
	std::string format(std::string input, int rank);

	/**
	 * Replaces `"%t"` sequences in `input` by `time`.
	 *
	 * @param input input string
	 * @param time replacement time
	 * @return formatted string
	 */
	std::string format(std::string input, api::scheduler::TimeStep time);

	/**
	 * Replaces `"%r"` and `"%t"` sequences in `input` respectibely by `rank` and
	 * `time`.
	 *
	 * @param input input string
	 * @param rank replacement rank
	 * @param time replacement time
	 * @return formatted string
	 */
	std::string format(std::string input, int rank, api::scheduler::TimeStep time);
}}

#endif
