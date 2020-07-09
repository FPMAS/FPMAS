#ifndef BASIC_NODE_H
#define BASIC_NODE_H

#include "fpmas/utils/log.h"
#include "fpmas/utils/macros.h"
#include "fpmas/api/graph/base/node.h"
#include "fpmas/utils/callback.h"

namespace fpmas { namespace graph {

	template<typename IdType, typename _EdgeType>
		class Node : public virtual api::graph::Node<IdType, _EdgeType> {
			public:
				typedef api::graph::Node<IdType, _EdgeType> NodeType;
				using typename NodeType::EdgeType;
				using typename NodeType::LayerIdType;

			private:
				IdType id;
				float weight = 1.;
				std::unordered_map<LayerIdType, std::vector<EdgeType*>> incoming_edges;
				std::unordered_map<LayerIdType, std::vector<EdgeType*>> outgoing_edges;

			public:
				Node(const IdType& id)
					: id(id) {}
				Node(const IdType& id, float weight)
					: id(id), weight(weight) {}

				Node(const Node&) = delete;
				Node(Node&&) = delete;
				Node& operator=(const Node&) = delete;
				Node& operator=(Node&&) = delete;

				IdType getId() const override {return id;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight = weight;};

				const std::vector<EdgeType*> getIncomingEdges() const override;
				const std::vector<EdgeType*> getIncomingEdges(LayerIdType layer) const override;
				const std::vector<typename EdgeType::NodeType*> inNeighbors() const override;
				const std::vector<typename EdgeType::NodeType*> inNeighbors(LayerIdType) const override;

				const std::vector<EdgeType*> getOutgoingEdges() const override;
				const std::vector<EdgeType*> getOutgoingEdges(LayerIdType layer) const override;
				const std::vector<typename EdgeType::NodeType*> outNeighbors() const override;
				const std::vector<typename EdgeType::NodeType*> outNeighbors(LayerIdType) const override;

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
			Node<IdType, EdgeType>::getIncomingEdges(LayerIdType id) const {
				try {
					return incoming_edges.at(id);
				} catch(std::out_of_range&) {
					return {};
				}
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
			Node<IdType, EdgeType>::getOutgoingEdges(LayerIdType id) const {
				try {
					return outgoing_edges.at(id);
				} catch(std::out_of_range&) {
					return {};
				}
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
		Node<IdType, EdgeType>::inNeighbors(LayerIdType layer) const {
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
		Node<IdType, EdgeType>::outNeighbors(LayerIdType layer) const {
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
				ID_C_STR(id),
				ID_C_STR(edge->getId()), edge
				);
			incoming_edges[edge->getLayer()].push_back(edge);
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::linkOut(EdgeType* edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Linking out edge %s (%p)",
				ID_C_STR(id),
				ID_C_STR(edge->getId()), edge
				);
			outgoing_edges[edge->getLayer()].push_back(edge);
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::unlinkIn(EdgeType *edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Unlink in edge %s (%p) (from %s)",
				ID_C_STR(id), ID_C_STR(edge->getId()), edge,
				ID_C_STR(edge->getSourceNode()->getId())
				);
			auto& edges = incoming_edges.at(edge->getLayer());
			edges.erase(std::remove(edges.begin(), edges.end(), edge));
		}

	template<typename IdType, typename EdgeType>
		void Node<IdType, EdgeType>::unlinkOut(EdgeType *edge) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Unlink out edge %s (%p) (to %s)",
				ID_C_STR(id), ID_C_STR(edge->getId()), edge,
				ID_C_STR(edge->getTargetNode()->getId())
				);
			auto& edges = outgoing_edges.at(edge->getLayer());
			edges.erase(std::remove(edges.begin(), edges.end(), edge));
		}
}}
#endif
