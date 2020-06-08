#ifndef BASIC_NODE_H
#define BASIC_NODE_H

#include "utils/log.h"
#include "utils/macros.h"
#include "api/graph/base/node.h"

namespace FPMAS::graph::base {

	template<typename IdType, typename _ArcType>
		class Node : public virtual FPMAS::api::graph::base::Node<IdType, _ArcType> {
			public:
				typedef FPMAS::api::graph::base::Node<IdType, _ArcType> NodeType;
				using typename NodeType::ArcType;
				using typename NodeType::LayerIdType;

			private:
				IdType id;
				//T _data;
				float weight = 1.;
				std::unordered_map<LayerIdType, std::vector<ArcType*>> incomingArcs;
				std::unordered_map<LayerIdType, std::vector<ArcType*>> outgoingArcs;

			public:
				Node(const IdType& id)
					: id(id) {}
				Node(const IdType& id, float weight)
					: id(id), weight(weight) {}

				IdType getId() const override {return id;};

				//T& data() override {return _data;};
				//const T& data() const override {return _data;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight = weight;};

				const std::vector<ArcType*> getIncomingArcs() override;
				const std::vector<ArcType*> getIncomingArcs(LayerIdType layer) override;

				const std::vector<ArcType*> getOutgoingArcs() override;
				const std::vector<ArcType*> getOutgoingArcs(LayerIdType layer) override;

				void linkIn(ArcType* arc) override;
				void linkOut(ArcType* arc) override;

				void unlinkIn(ArcType* arc) override;
				void unlinkOut(ArcType* arc) override;

		};

	template<typename IdType, typename ArcType>
		const std::vector<typename Node<IdType, ArcType>::ArcType*>
			Node<IdType, ArcType>::getIncomingArcs() {
				std::vector<ArcType*> in;
				for(auto layer : this->incomingArcs) {
					for(auto* arc : layer.second) {
						in.push_back(arc);
					}
				}
				return in;
		}

	template<typename IdType, typename ArcType>
		const std::vector<typename Node<IdType, ArcType>::ArcType*>
			Node<IdType, ArcType>::getIncomingArcs(LayerIdType id) {
				return incomingArcs[id];
		}
	
	template<typename IdType, typename ArcType>
		const std::vector<typename Node<IdType, ArcType>::ArcType*>
			Node<IdType, ArcType>::getOutgoingArcs() {
				std::vector<ArcType*> out;
				for(auto layer : this->outgoingArcs) {
					for(auto* arc : layer.second) {
						out.push_back(arc);
					}
				}
				return out;
		}

	template<typename IdType, typename ArcType>
		const std::vector<typename Node<IdType, ArcType>::ArcType*>
			Node<IdType, ArcType>::getOutgoingArcs(LayerIdType id) {
				return outgoingArcs[id];
		}
	
	template<typename IdType, typename ArcType>
		void Node<IdType, ArcType>::linkIn(ArcType* arc) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Linking in arc %s (%p)",
				ID_C_STR(id),
				ID_C_STR(arc->getId()), arc
				);
			incomingArcs[arc->getLayer()].push_back(arc);
		}

	template<typename IdType, typename ArcType>
		void Node<IdType, ArcType>::linkOut(ArcType* arc) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Linking out arc %s (%p)",
				ID_C_STR(id),
				ID_C_STR(arc->getId()), arc
				);
			outgoingArcs[arc->getLayer()].push_back(arc);
		}

	template<typename IdType, typename ArcType>
		void Node<IdType, ArcType>::unlinkIn(ArcType *arc) {
			FPMAS_LOGV(
				-1, "NODE", "%s : Unlink in arc %s (%p) (from %s)",
				ID_C_STR(id), ID_C_STR(arc->getId()), arc,
				ID_C_STR(arc->getSourceNode()->getId())
				);
			auto& arcs = incomingArcs.at(arc->getLayer());
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
			auto& arcs = outgoingArcs.at(arc->getLayer());
			FPMAS_LOGV(-1, "NODE", "%s : Current out arcs size on layer %i : %lu",
					ID_C_STR(id), arc->getLayer(), arcs.size());
			arcs.erase(std::remove(arcs.begin(), arcs.end(), arc));
			FPMAS_LOGV(-1, "NODE", "Done.%i", arcs.size());
		}
}
#endif
