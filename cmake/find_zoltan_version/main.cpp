#include <iostream>
#include "zoltan.h"

#define EXPAND_STR(VERSION_NUMBER) #VERSION_NUMBER
#define STR(VERSION_NUMBER) EXPAND_STR(VERSION_NUMBER)

int main() {
	std::cout << STR(ZOLTAN_VERSION_NUMBER);
	return 0;
}
