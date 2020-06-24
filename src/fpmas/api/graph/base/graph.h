#ifndef GRAPH_API_H
#define GRAPH_API_H

#include <unordered_map>
#include "node.h"

#define GRAPH_PARAMS \
	typename NodeType,\
	typename ArcType

#define GRAPH_PARAMS_SPEC NodeType, ArcType

namespace fpmas::api::graph::base {

	template<GRAPH_PARAMS> class Graph {
		static_assert(std::is_base_of<typename NodeType::ArcType, ArcType>::value);
		static_assert(std::is_base_of<typename ArcType::NodeType, NodeType>::value);
		public:
			typedef typename ArcType::NodeType NodeBase; // Maybe NodeType == node_base, but, in any case, NodeType is convertible to node_base.
			typedef typename NodeType::ArcType ArcBase;

			typedef typename NodeType::IdType NodeIdType;
			typedef typename ArcType::IdType ArcIdType;
			typedef typename ArcType::LayerIdType LayerIdType;

			typedef fpmas::api::graph::base::IdHash<typename NodeType::IdType> NodeIdHash;
			typedef fpmas::api::graph::base::IdHash<typename ArcType::IdType> ArcIdHash;
			typedef std::unordered_map<
				NodeIdType, NodeType*, NodeIdHash
				> NodeMap;
			typedef std::unordered_map<
				ArcIdType, ArcType*, ArcIdHash
				> ArcMap;

		public:
			virtual void insert(NodeType* node) = 0;
			virtual void insert(ArcType* arc) = 0;

			virtual void erase(NodeBase* node) = 0;
			virtual void erase(ArcBase* arc) = 0;

			virtual void addCallOnInsertNode(api::utils::Callback<NodeType*>*) = 0;
			virtual void addCallOnEraseNode(api::utils::Callback<NodeType*>*) = 0;

			virtual void addCallOnInsertArc(api::utils::Callback<ArcType*>*) = 0;
			virtual void addCallOnEraseArc(api::utils::Callback<ArcType*>*) = 0;

			// Node getters
			virtual const NodeIdType& currentNodeId() const = 0;
			virtual NodeType* getNode(NodeIdType) = 0;
			virtual const NodeType* getNode(NodeIdType) const = 0;
			virtual const NodeMap& getNodes() const = 0;

			// Arc getters
			virtual const ArcIdType& currentArcId() const = 0;
			virtual ArcType* getArc(ArcIdType) = 0;
			virtual const ArcType* getArc(ArcIdType) const = 0;
			virtual const ArcMap& getArcs() const = 0;

			virtual void removeNode(NodeType*) = 0;
			void removeNode(const NodeIdType& id) {
				this->removeNode(this->getNode(id));
			}

			virtual void unlink(ArcType*) = 0;
			void unlink(const ArcIdType& id) {
				this->unlink(this->getArc(id));
			}

			virtual void clear() = 0;
		
			virtual ~Graph() {}
	};

/*
 *    template<typename GraphImpl, typename... Args>
 *        typename GraphImpl::NodeType* buildNode(GraphImpl& graph, Args... args) {
 *            return graph.buildNode(args...);
 *        }
 *
 *    template<typename GraphImpl, typename... Args>
 *        typename GraphImpl::ArcType* link(
 *                GraphImpl& graph,
 *                typename GraphImpl::NodeBase* src,
 *                typename GraphImpl::NodeBase* tgt,
 *                Args... args) {
 *            return graph.link(src, tgt, args...);
 *        }
 */
}
#endif
