#ifndef NESTED_GRAPH_H
#define NESTED_GRAPH_H

#include "graph.h"

template<class T> class NestedGraph : public Graph<NestedGraph<T>> {

};

#endif

