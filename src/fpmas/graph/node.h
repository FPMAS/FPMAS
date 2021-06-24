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
				for(auto layer : this->incoming_edges) {
					for(auto* edge : layer.second) {
						in.push_back(edge);
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
				for(auto layer : this->outgoing_edges) {
					for(auto* edge : layer.second) {
						out.push_back(edge);
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
			for(auto edge : this->getIncomingEdges()) {
				neighbors.push_back(edge->getSourceNode());
			}
			return neighbors;
		}

	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType::NodeType*>
		Node<IdType, EdgeType>::inNeighbors(api::graph::LayerId layer) const {
			std::vector<typename EdgeType::NodeType*> neighbors;
			for(auto edge : this->getIncomingEdges(layer)) {
				neighbors.push_back(edge->getSourceNode());
			}
			return neighbors;
		}

	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType::NodeType*>
		Node<IdType, EdgeType>::outNeighbors() const {
			std::vector<typename EdgeType::NodeType*> neighbors;
			for(auto edge : this->getOutgoingEdges()) {
				neighbors.push_back(edge->getTargetNode());
			}
			return neighbors;
		}

	template<typename IdType, typename EdgeType>
		const std::vector<typename Node<IdType, EdgeType>::EdgeType::NodeType*>
		Node<IdType, EdgeType>::outNeighbors(api::graph::LayerId layer) const {
			std::vector<typename EdgeType::NodeType*> neighbors;
			for(auto edge : this->getOutgoingEdges(layer)) {
				neighbors.push_back(edge->getTargetNode());
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
			incoming_edges[edge->getLayer()].push_back(edge);
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::linkOut(EdgeType* edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Linking out edge %s (%p)",
				FPMAS_C_STR(id),
				FPMAS_C_STR(edge->getId()), edge
				);
			outgoing_edges[edge->getLayer()].push_back(edge);
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::unlinkIn(EdgeType *edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Unlink in edge %s (%p) (from %s)",
				FPMAS_C_STR(id), FPMAS_C_STR(edge->getId()), edge,
				FPMAS_C_STR(edge->getSourceNode()->getId())
				);
			auto& edges = incoming_edges.at(edge->getLayer());
			edges.erase(std::remove(edges.begin(), edges.end(), edge));
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::unlinkOut(EdgeType *edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Unlink out edge %s (%p) (to %s)",
				FPMAS_C_STR(id), FPMAS_C_STR(edge->getId()), edge,
				FPMAS_C_STR(edge->getTargetNode()->getId())
				);
			auto& edges = outgoing_edges.at(edge->getLayer());
			edges.erase(std::remove(edges.begin(), edges.end(), edge));
		}
}}
#endif
