#ifndef MACROS_H
#define MACROS_H

namespace FPMAS::graph::base {
	typedef int LayerId;
	static constexpr int DefaultLayer = 0;
}

#define NODE_PARAMS class T, int N
#define NODE_PARAMS_SPEC T, N
#define NODE Node<NODE_PARAMS_SPEC>
#define ARC Arc<NODE_PARAMS_SPEC>
// #define GRAPH FPMAS::graph::base::Graph<T, LayerType, N>

#define SYNC_MODE template<class> class S

#endif
