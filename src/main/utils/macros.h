#ifndef MACROS_H
#define MACROS_H

#define NODE_PARAMS class T, typename LayerType, int N
#define NODE_PARAMS_SPEC T, LayerType, N
#define NODE Node<NODE_PARAMS_SPEC>
#define ARC Arc<NODE_PARAMS_SPEC>
// #define GRAPH FPMAS::graph::base::Graph<T, LayerType, N>

#define SYNC_MODE template<class, typename, int> class S

#endif
