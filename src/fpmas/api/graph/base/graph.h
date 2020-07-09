#ifndef GRAPH_API_H
#define GRAPH_API_H

#include <unordered_map>
#include "node.h"

#define GRAPH_PARAMS \
	typename NodeType,\
	typename EdgeType

#define GRAPH_PARAMS_SPEC NodeType, EdgeType

namespace fpmas { namespace api { namespace graph {

	template<GRAPH_PARAMS> class Graph {
		static_assert(std::is_base_of<typename NodeType::EdgeType, EdgeType>::value,
				"NodeType::EdgeType must be a base of EdgeType.");
		static_assert(std::is_base_of<typename EdgeType::NodeType, NodeType>::value,
				"EdgeType::NodeType must be a base of NodeType.");
		public:
			typedef typename EdgeType::NodeType NodeBase; // Maybe NodeType == node_base, but, in any case, NodeType is convertible to node_base.
			typedef typename NodeType::EdgeType EdgeBase;

			typedef typename NodeType::IdType NodeIdType;
			typedef typename EdgeType::IdType EdgeIdType;
			typedef typename EdgeType::LayerIdType LayerIdType;

			typedef fpmas::api::graph::IdHash<typename NodeType::IdType> NodeIdHash;
			typedef fpmas::api::graph::IdHash<typename EdgeType::IdType> EdgeIdHash;
			typedef std::unordered_map<
				NodeIdType, NodeType*, NodeIdHash
				> NodeMap;
			typedef std::unordered_map<
				EdgeIdType, EdgeType*, EdgeIdHash
				> EdgeMap;

		public:
			virtual void insert(NodeType* node) = 0;
			virtual void insert(EdgeType* edge) = 0;

			virtual void erase(NodeBase* node) = 0;
			virtual void erase(EdgeBase* edge) = 0;

			virtual void addCallOnInsertNode(api::utils::Callback<NodeType*>*) = 0;
			virtual void addCallOnEraseNode(api::utils::Callback<NodeType*>*) = 0;

			virtual void addCallOnInsertEdge(api::utils::Callback<EdgeType*>*) = 0;
			virtual void addCallOnEraseEdge(api::utils::Callback<EdgeType*>*) = 0;

			// Node getters
			virtual const NodeIdType& currentNodeId() const = 0;
			virtual NodeType* getNode(NodeIdType) = 0;
			virtual const NodeType* getNode(NodeIdType) const = 0;
			virtual const NodeMap& getNodes() const = 0;

			// Edge getters
			virtual const EdgeIdType& currentEdgeId() const = 0;
			virtual EdgeType* getEdge(EdgeIdType) = 0;
			virtual const EdgeType* getEdge(EdgeIdType) const = 0;
			virtual const EdgeMap& getEdges() const = 0;

			virtual void removeNode(NodeType*) = 0;
			void removeNode(const NodeIdType& id) {
				this->removeNode(this->getNode(id));
			}

			virtual void unlink(EdgeType*) = 0;
			void unlink(const EdgeIdType& id) {
				this->unlink(this->getEdge(id));
			}

			virtual void clear() = 0;
		
			virtual ~Graph() {}
	};
}}}
#endif
