#include "grid.h"

namespace fpmas { namespace api { namespace model {
	bool operator==(const DiscretePoint& p1, const DiscretePoint& p2) {
		return p1.x == p2.x && p1.y == p2.y;
	}
}}}
