#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#define FPMAS_MIN_PROCS(test, comm, min) \
	if(comm.getSize() < min) {\
		std::cout << "\033[33m[ WARNING  ]\033[0m " test " test ignored : please use at least " #min " procs." << std::endl; \
	} else

#endif
