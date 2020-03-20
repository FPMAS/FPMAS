#ifndef MACROS_H
#define MACROS_H

namespace FPMAS::graph::base {
	//typedef unsigned long NodeId;
	//typedef unsigned long ArcId;
	typedef int LayerId;
	static constexpr int DefaultLayer = 0;
}
namespace FPMAS::graph::parallel {
	typedef unsigned long IdType;
}

#define SYNC_MODE template<typename, int> class S

#endif
