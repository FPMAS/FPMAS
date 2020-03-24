#ifndef GRAPH_EXCEPTIONS_H
#define GRAPH_EXCEPTIONS_H

#include <stdexcept>
#include "utils/macros.h"

namespace FPMAS::graph::base::exceptions {
	template<typename IdType>
	class arc_out_of_graph : public std::out_of_range {
		public:
			explicit arc_out_of_graph(IdType arcId)
				: std::out_of_range("Arc " + std::string(arcId) + " is not contained in the current graph.")
			{}

	};
	template<typename IdType>
	class node_out_of_graph : public std::out_of_range {
		public:
			explicit node_out_of_graph(IdType nodeId)
				: std::out_of_range("Node " + std::string(nodeId) + " is not contained in the current graph.")
			{}

	};
}
#endif
