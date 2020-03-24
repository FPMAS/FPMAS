#ifndef GRAPH_EXCEPTIONS_H
#define GRAPH_EXCEPTIONS_H

#include <stdexcept>
#include "utils/macros.h"

/**
 * Exceptions used is the base Graph implementation.
 */
namespace FPMAS::graph::base::exceptions {
	/**
	 * Exception thrown when trying to access an Arc not contained in the
	 * current Graph.
	 */
	template<typename IdType>
	class arc_out_of_graph : public std::out_of_range {
		public:
			/**
			 * arc_out_of_graph constructor.
			 *
			 * @param arcId id of the not found Arc
			 */
			explicit arc_out_of_graph(IdType arcId)
				: std::out_of_range("Arc " + std::string(arcId) + " is not contained in the current graph.")
			{}

	};

	/**
	 * Exception thrown when trying to access a Node not contained in the
	 * current Graph.
	 */
	template<typename IdType>
	class node_out_of_graph : public std::out_of_range {
		public:
			/**
			 * node_out_of_graph constructor.
			 *
			 * @param nodeId id of the not found Node
			 */
			explicit node_out_of_graph(IdType nodeId)
				: std::out_of_range("Node " + std::string(nodeId) + " is not contained in the current graph.")
			{}

	};
}
#endif
