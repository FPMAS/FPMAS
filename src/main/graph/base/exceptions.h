#ifndef GRAPH_EXCEPTIONS_H
#define GRAPH_EXCEPTIONS_H

#include <stdexcept>
#include "utils/macros.h"

using FPMAS::graph::base::ArcId;
using FPMAS::graph::base::NodeId;

namespace FPMAS::graph::base::exceptions {
	class arc_out_of_graph : public std::out_of_range {
		public:
			explicit arc_out_of_graph(ArcId arcId)
				: std::out_of_range("Arc " + std::to_string(arcId) + " is not contained in the current graph.")
			{}

	};
	class node_out_of_graph : public std::out_of_range {
		public:
			explicit node_out_of_graph(NodeId nodeId)
				: std::out_of_range("Node " + std::to_string(nodeId) + " is not contained in the current graph.")
			{}

	};
}
#endif
