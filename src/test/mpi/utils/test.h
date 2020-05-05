#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#define PRINT_MIN_PROCS_WARNING(test, min) std::cout << "\e[33m[ WARNING  ]\e[0m " #test " test ignored : please use at least " #min " procs." << std::endl;
#define PRINT_PAIR_PROCS_WARNING(test) std::cout << "\e[33m[ WARNING  ]\e[0m " #test " test ignored : please use a pair number of procs." << std::endl;

#endif
