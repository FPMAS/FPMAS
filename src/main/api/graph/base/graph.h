#ifndef GRAPH_API_H
#define GRAPH_API_H

#include <unordered_map>
#include "node.h"

#define GRAPH_PARAMS \
	typename NodeType,\
	typename ArcType

#define GRAPH_PARAMS_SPEC NodeType, ArcType

namespace FPMAS::api::graph::base {

	template<GRAPH_PARAMS> class Graph {
		static_assert(std::is_base_of<typename NodeType::arc_type, ArcType>::value);
		static_assert(std::is_base_of<typename ArcType::node_type, NodeType>::value);
		public:
			typedef NodeType node_type;
			typedef typename ArcType::node_type node_base; // Maybe node_type == node_base, but, in any case, node_type is convertible to node_base.
			typedef ArcType arc_type;
			typedef typename NodeType::arc_type arc_base;

			typedef typename NodeType::id_type node_id_type;
			typedef typename ArcType::id_type arc_id_type;
			typedef typename arc_type::layer_id_type layer_id_type;

			typedef FPMAS::api::graph::base::IdHash<typename NodeType::id_type> node_id_hash;
			typedef FPMAS::api::graph::base::IdHash<typename ArcType::id_type> arc_id_hash;
			typedef std::unordered_map<
				node_id_type, node_type*, node_id_hash
				> node_map;
			typedef std::unordered_map<
				arc_id_type, arc_type*, arc_id_hash
				> arc_map;


		protected:
			virtual void insert(node_type* node) = 0;
			virtual void insert(arc_type* arc) = 0;

			virtual void erase(node_base* node) = 0;
			virtual void erase(arc_base* arc) = 0;

		public:
			// Node getters
			virtual const node_id_type& currentNodeId() const = 0;
			virtual node_type* getNode(node_id_type) = 0;
			virtual const node_type* getNode(node_id_type) const = 0;
			virtual const node_map& getNodes() const = 0;

			// Arc getters
			virtual const arc_id_type& currentArcId() const = 0;
			virtual arc_type* getArc(arc_id_type) = 0;
			virtual const arc_type* getArc(arc_id_type) const = 0;
			virtual const arc_map& getArcs() const = 0;

			virtual void removeNode(node_type*) = 0;
			void removeNode(const node_id_type& id) {
				this->removeNode(this->getNode(id));
			};

			virtual void unlink(arc_type*) = 0;
			void unlink(const arc_id_type& id) {
				this->unlink(this->getArc(id));
			}
		
			virtual ~Graph() {}
	};

	template<typename GraphImpl, typename... Args>
		typename GraphImpl::node_type* buildNode(GraphImpl& graph, Args... args) {
			return graph.buildNode(args...);
		}

	template<typename GraphImpl, typename... Args>
		typename GraphImpl::arc_type* link(
				GraphImpl& graph,
				typename GraphImpl::node_base* src,
				typename GraphImpl::node_base* tgt,
				Args... args) {
			return graph.link(src, tgt, args...);
		}
}
#endif
