#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "gtest/gtest.h"
#include <vector>

namespace FPMAS {
	namespace test_utils {

		template<class T> void print_fail_message(T* array_begin, int n, T value) {
			std::cout << "Value " << value << " not found." << std::endl;
			std::cout << "Current values :" << std::endl;
			for(int i=0; i < n; i++) {
				std::cout << "\t" << array_begin[i] << std::endl;
			}

		}

		template<class T> void check_contains(T* array_begin, int n, T value) {
			for(int i = 0; i < n; i++) {
				if(array_begin[i] == value) {
					return;
				}
			}
			print_fail_message(array_begin, n, value);
			FAIL();
		}

		template<class T> void check_contains_result(T* array_begin, int n, T value, int* result) {
			for(int i = 0; i < n; i++) {
				if(array_begin[i] == value) {
					*result = i;
					return;
				}
			}
			print_fail_message(array_begin, n, value);
			FAIL();
		}


		template<class T> void assert_contains(std::vector<T> v, T a, int* result) {
			for(int i = 0; i < v.size(); i++) {
				if(v[i] == a) {
					*result = i;
					return;
				}
			}
			FAIL();
		}
		template<class T> void assert_contains(std::vector<T> v, T a) {
			for(int i = 0; i < v.size(); i++) {
				if(v[i] == a) {
					return;
				}
			}
			FAIL();
		}

		template<class T, int n> void assert_contains(T* array, T value) {
			for(int i = 0; i < n; i++) {
				if(array[i] == value) {
					return;
				}
			}
			FAIL();
		}

		template<class T, int n> void assert_contains(T* array, T value, int* result) {
			check_contains_result<T>(array, n, value, result);
			/*
			for(int i = 0; i < n; i++) {
				if(array[i] == value) {
					*result = i;
					return;
				}
			}
			FAIL();
			*/
		}


	}
}
#endif
