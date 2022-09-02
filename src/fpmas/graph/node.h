#ifndef FPMAS_NODE_H
#define FPMAS_NODE_H

/** \file src/fpmas/graph/node.h
 * Node implementation.
 */

#include <unordered_map>
#include <algorithm>

#include "fpmas/utils/log.h"
#include "fpmas/utils/macros.h"
#include "fpmas/api/graph/node.h"
#include "fpmas/utils/callback.h"

namespace fpmas { namespace graph {

	/**
	 * api::graph::Node implementation.
	 */
	template<typename _IdType, typename _EdgeType>
		class Node : public virtual api::graph::Node<_IdType, _EdgeType> {
			public:
				using typename api::graph::Node<_IdType, _EdgeType>::IdType;
				using typename api::graph::Node<_IdType, _EdgeType>::EdgeType;

			private:
				IdType id;
				float weight;
				mutable std::unordered_map<api::graph::LayerId, std::vector<EdgeType*>> incoming_edges;
				mutable std::unordered_map<api::graph::LayerId, std::vector<EdgeType*>> outgoing_edges;
				std::size_t num_in_edges = 0;
				std::size_t num_out_edges = 0;

			public:
				/**
				 * Node constructor.
				 *
				 * The Node is initialized with a default weight of 1.
				 *
				 * @param id node id
				 */
				Node(const IdType& id)
					: id(id), weight(1.f) {}

				Node(const Node&) = delete;
				Node(Node&&) = delete;
				Node& operator=(const Node&) = delete;
				Node& operator=(Node&&) = delete;

				IdType getId() const override {return id;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight = weight;};

				const std::vector<EdgeType*> getIncomingEdges() const override;
				const std::vector<EdgeType*> getIncomingEdges(api::graph::LayerId layer) const override;
				const std::vector<typename EdgeType::NodeType*> inNeighbors() const override;
				const std::vector<typename EdgeType::NodeType*> inNeighbors(api::graph::LayerId) const override;

				const std::vector<EdgeType*> getOutgoingEdges() const override;
				const std::vector<EdgeType*> getOutgoingEdges(api::graph::LayerId layer) const override;
				const std::vector<typename EdgeType::NodeType*> outNeighbors() const override;
				const std::vector<typename EdgeType::NodeType*> outNeighbors(api::graph::LayerId) const override;

				void linkIn(EdgeType* edge) override;
				void linkOut(EdgeType* edge) override;

				void unlinkIn(EdgeType* edge) override;
				void unlinkOut(EdgeType* edge) override;

				virtual ~Node() {}
		};

	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType*>
			Node<IdType, EdgeType>::getIncomingEdges() const {
				std::vector<EdgeType*> in;
				in.reserve(num_in_edges);
				for(auto layer : this->incoming_edges) {
					for(auto* edge : layer.second) {
						in.emplace_back(edge);
					}
				}
				return in;
		}

	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType*>
			Node<IdType, EdgeType>::getIncomingEdges(api::graph::LayerId id) const {
				return incoming_edges[id];
		}
	
	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType*>
			Node<IdType, EdgeType>::getOutgoingEdges() const {
				std::vector<EdgeType*> out;
				out.reserve(num_out_edges);
				for(auto layer : this->outgoing_edges) {
					for(auto* edge : layer.second) {
						out.emplace_back(edge);
					}
				}
				return out;
		}

	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType*>
			Node<IdType, EdgeType>::getOutgoingEdges(api::graph::LayerId id) const {
				return outgoing_edges[id];
		}
	
	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType::NodeType*>
		Node<IdType, EdgeType>::inNeighbors() const {
			std::vector<typename EdgeType::NodeType*> neighbors;
			neighbors.reserve(num_in_edges);
			for(auto edge : this->getIncomingEdges()) {
				neighbors.emplace_back(edge->getSourceNode());
			}
			return neighbors;
		}

	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType::NodeType*>
		Node<IdType, EdgeType>::inNeighbors(api::graph::LayerId layer) const {
			std::vector<typename EdgeType::NodeType*> neighbors;
			auto edges = this->getIncomingEdges(layer);
			neighbors.reserve(edges.size());
			for(auto edge : edges) {
				neighbors.emplace_back(edge->getSourceNode());
			}
			return neighbors;
		}

	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType::NodeType*>
		Node<IdType, EdgeType>::outNeighbors() const {
			std::vector<typename EdgeType::NodeType*> neighbors;
			neighbors.reserve(num_out_edges);
			for(auto& edge : this->getOutgoingEdges()) {
				neighbors.emplace_back(edge->getTargetNode());
			}
			return neighbors;
		}

	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType::NodeType*>
		Node<IdType, EdgeType>::outNeighbors(api::graph::LayerId layer) const {
			std::vector<typename EdgeType::NodeType*> neighbors;
			auto edges = this->getOutgoingEdges(layer);
			neighbors.reserve(edges.size());
			for(auto edge : edges) {
				neighbors.emplace_back(edge->getTargetNode());
			}
			return neighbors;
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::linkIn(EdgeType* edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Linking in edge %s (%p)",
				FPMAS_C_STR(id),
				FPMAS_C_STR(edge->getId()), edge
				);
			incoming_edges[edge->getLayer()].emplace_back(edge);
			++num_in_edges;
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::linkOut(EdgeType* edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Linking out edge %s (%p)",
				FPMAS_C_STR(id),
				FPMAS_C_STR(edge->getId()), edge
				);
			outgoing_edges[edge->getLayer()].emplace_back(edge);
			++num_out_edges;
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::unlinkIn(EdgeType *edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Unlink in edge %s (%p) (from %s)",
				FPMAS_C_STR(id), FPMAS_C_STR(edge->getId()), edge,
				FPMAS_C_STR(edge->getSourceNode()->getId())
				);
			auto& edges = incoming_edges.find(edge->getLayer())->second;
			// Removes only first edge matching, assumes no duplicate
			auto it = std::find(edges.begin(), edges.end(), edge);
			if(it != edges.end()) {
				edges.erase(it);
				--num_in_edges;
			}
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::unlinkOut(EdgeType *edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Unlink out edge %s (%p) (to %s)",
				FPMAS_C_STR(id), FPMAS_C_STR(edge->getId()), edge,
				FPMAS_C_STR(edge->getTargetNode()->getId())
				);
			auto& edges = outgoing_edges.find(edge->getLayer())->second;
			// Removes only first edge matching, assumes no duplicate
			auto it = std::find(edges.begin(), edges.end(), edge);
			if(it != edges.end()) {
				edges.erase(it);
				--num_out_edges;
			}
		}
}}
#endif
