#ifndef NESTED_GRAPH_H
#define NESTED_GRAPH_H

#include "base/graph.h"

namespace FPMAS {
	namespace graph {

		template<class T> class NestedGraph : public Graph<NestedGraph<T>> {

		};
	}
}

#endif

