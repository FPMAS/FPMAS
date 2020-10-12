#include "nlohmann/json.hpp"
#include <iostream>

int main() {
	std::cout << NLOHMANN_JSON_VERSION_MAJOR << "."
		<< NLOHMANN_JSON_VERSION_MINOR << "."
		<< NLOHMANN_JSON_VERSION_PATCH;
}
