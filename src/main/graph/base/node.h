#ifndef BASIC_NODE_H
#define BASIC_NODE_H

#include "utils/log.h"
#include "utils/macros.h"
#include "api/graph/base/node.h"
#include "utils/callback.h"

namespace fpmas::graph::base {

	template<typename IdType, typename _ArcType>
		class Node : public virtual fpmas::api::graph::base::Node<IdType, _ArcType> {
			public:
				typedef fpmas::api::graph::base::Node<IdType, _ArcType> NodeType;
				using typename NodeType::ArcType;
				using typename NodeType::LayerIdType;

			private:
				IdType id;
				float weight = 1.;
				std::unordered_map<LayerIdType, std::vector<ArcType*>> incoming_arcs;
				std::unordered_map<LayerIdType, std::vector<ArcType*>> outgoing_arcs;

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

				const std::vector<ArcType*> getIncomingArcs() const override;
				const std::vector<ArcType*> getIncomingArcs(LayerIdType layer) const override;

				const std::vector<ArcType*> getOutgoingArcs() const override;
				const std::vector<ArcType*> getOutgoingArcs(LayerIdType layer) const override;

				void linkIn(ArcType* arc) override;
				void linkOut(ArcType* arc) override;

				void unlinkIn(ArcType* arc) override;
				void unlinkOut(ArcType* arc) override;

				virtual ~Node() {}
		};

	template<typename IdType, typename ArcType>
		const std::vector<typename Node<IdType, ArcType>::ArcType*>
			Node<IdType, ArcType>::getIncomingArcs() const {
				std::vector<ArcType*> in;
				for(auto layer : this->incoming_arcs) {
					for(auto* arc : layer.second) {
						in.push_back(arc);
					}
				}
				return in;
		}

	template<typename IdType, typename ArcType>
		const std::vector<typename Node<IdType, ArcType>::ArcType*>
			Node<IdType, ArcType>::getIncomingArcs(LayerIdType id) const {
				try {
					return incoming_arcs.at(id);
				} catch(std::out_of_range&) {
					return {};
				}
		}
	
	template<typename IdType, typename ArcType>
		const std::vector<typename Node<IdType, ArcType>::ArcType*>
			Node<IdType, ArcType>::getOutgoingArcs() const {
				std::vector<ArcType*> out;
				for(auto layer : this->outgoing_arcs) {
					for(auto* arc : layer.second) {
						out.push_back(arc);
					}
				}
				return out;
		}

	template<typename IdType, typename ArcType>
		const std::vector<typename Node<IdType, ArcType>::ArcType*>
			Node<IdType, ArcType>::getOutgoingArcs(LayerIdType id) const {
				try {
					return outgoing_arcs.at(id);
				} catch(std::out_of_range&) {
					return {};
				}
		}
	
	template<typename IdType, typename ArcType>
		void Node<IdType, ArcType>::linkIn(ArcType* arc) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Linking in arc %s (%p)",
				ID_C_STR(id),
				ID_C_STR(arc->getId()), arc
				);
			incoming_arcs[arc->getLayer()].push_back(arc);
		}

	template<typename IdType, typename ArcType>
		void Node<IdType, ArcType>::linkOut(ArcType* arc) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Linking out arc %s (%p)",
				ID_C_STR(id),
				ID_C_STR(arc->getId()), arc
				);
			outgoing_arcs[arc->getLayer()].push_back(arc);
		}

	template<typename IdType, typename ArcType>
		void Node<IdType, ArcType>::unlinkIn(ArcType *arc) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Unlink in arc %s (%p) (from %s)",
				ID_C_STR(id), ID_C_STR(arc->getId()), arc,
				ID_C_STR(arc->getSourceNode()->getId())
				);
			auto& arcs = incoming_arcs.at(arc->getLayer());
			FPMAS_LOGV(-1, "NODE", "%s : Current in arcs size on layer %i: %lu",
					ID_C_STR(id), arc->getLayer(), arcs.size());
			arcs.erase(std::remove(arcs.begin(), arcs.end(), arc));
			FPMAS_LOGV(-1, "NODE", "Done.%i", arcs.size());
		}

	template<typename IdType, typename ArcType>
		void Node<IdType, ArcType>::unlinkOut(ArcType *arc) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Unlink out arc %s (%p) (to %s)",
				ID_C_STR(id), ID_C_STR(arc->getId()), arc,
				ID_C_STR(arc->getTargetNode()->getId())
				);
			auto& arcs = outgoing_arcs.at(arc->getLayer());
			FPMAS_LOGV(-1, "NODE", "%s : Current out arcs size on layer %i : %lu",
					ID_C_STR(id), arc->getLayer(), arcs.size());
			arcs.erase(std::remove(arcs.begin(), arcs.end(), arc));
			FPMAS_LOGV(-1, "NODE", "Done.%i", arcs.size());
		}
}
#endif
