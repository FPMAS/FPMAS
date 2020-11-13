#include "basic_id.h"

namespace fpmas {
	std::string to_string(const BasicId& id) {
		return std::to_string(id.value);
	}
}
