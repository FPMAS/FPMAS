#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "gtest/gtest.h"
#include <vector>
#include <array>

#define PRINT_MIN_PROCS_WARNING(test, min) std::cout << "\e[33m[ WARNING  ]\e[0m " #test " test ignored : please use at least " #min " procs." << std::endl;
#define PRINT_PAIR_PROCS_WARNING(test) std::cout << "\e[33m[ WARNING  ]\e[0m " #test " test ignored : please use a pair number of procs." << std::endl;

namespace FPMAS {
	namespace test {

		template<typename T, typename C> void print_fail_message(const T& item, const C& container) {
			std::cout << "Value " << item << " not found." << std::endl;
			std::cout << "Current values :" << std::endl;
			for(auto item : container) {
				std::cout << "\t" << item << std::endl;
			}

		}
		template<typename T, typename C> void ASSERT_CONTAINS(const T& item, const C& container) {
			for(auto _item : container) {
				if(_item == item) {
					return;
				}
			}
			print_fail_message(item, container);
			FAIL();
		}
	}
}
#endif
