#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "gtest/gtest.h"
#include <vector>
#include <array>

#define PRINT_MIN_PROCS_WARNING(test, min) std::cout << "\e[33m[ WARNING  ]\e[0m " #test " test ignored : please use at least " #min " procs." << std::endl;
#define PRINT_PAIR_PROCS_WARNING(test) std::cout << "\e[33m[ WARNING  ]\e[0m " #test " test ignored : please use a pair number of procs." << std::endl;

namespace FPMAS {
	namespace test_utils {

		template<class T> void print_fail_message(T* array_begin, int n, T value) {
			std::cout << "Value " << value << " not found." << std::endl;
			std::cout << "Current values :" << std::endl;
			for(int i=0; i < n; i++) {
				std::cout << "\t" << array_begin[i] << std::endl;
			}

		}

		template<class T, int n> void assert_contains(T* array_begin, T value) {
			for(int i = 0; i < n; i++) {
				if(array_begin[i] == value) {
					return;
				}
			}
			print_fail_message(array_begin, n, value);
			FAIL();
		}

		template<class T, int n> void assert_contains(T* array_begin, T value, int* result) {
			for(int i = 0; i < n; i++) {
				if(array_begin[i] == value) {
					*result = i;
					return;
				}
			}
			print_fail_message(array_begin, n, value);
			FAIL();
		}
	}
}
#endif
