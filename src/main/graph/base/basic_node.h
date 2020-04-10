#ifndef BASIC_NODE_H
#define BASIC_NODE_H

#include "api/graph/base/node.h"

namespace FPMAS::graph::base {

	using FPMAS::api::graph::base::LayerId;

	template<typename T, typename IdType>
		class BasicNode : public FPMAS::api::graph::base::Node<T, IdType> {
			public:
				typedef FPMAS::api::graph::base::Node<T, IdType> abstract_node_type;
				typedef typename abstract_node_type::arc_type abstract_arc_type;

			private:
				IdType id;
				T _data;
				float weight = 1.;
				std::unordered_map<LayerId, std::vector<abstract_arc_type*>> incomingArcs;
				std::unordered_map<LayerId, std::vector<abstract_arc_type*>> outgoingArcs;

			public:
				BasicNode(const IdType& id)
					: id(id) {}
				BasicNode(const IdType& id, T&& data)
					: id(id), _data(data) {}
				BasicNode(const IdType& id, T&& data, float weight)
					: id(id), _data(data), weight(weight) {}

				IdType getId() const override {return id;};

				T& data() override {return _data;};
				const T& data() const override {return _data;};

				float getWeight() const override {return weight;};
				void setWeight(float weight) override {this->weight = weight;};

				const std::vector<abstract_arc_type*> getIncomingArcs() override;
				const std::vector<abstract_arc_type*> getIncomingArcs(LayerId layer) override;

				const std::vector<abstract_arc_type*> getOutgoingArcs() override;
				const std::vector<abstract_arc_type*> getOutgoingArcs(LayerId layer) override;

				void linkIn(abstract_arc_type* arc, LayerId layer) override;
				void linkOut(abstract_arc_type* arc, LayerId layer) override;

				void unlinkIn(abstract_arc_type* arc, LayerId layer) override;
				void unlinkOut(abstract_arc_type* arc, LayerId layer) override;

		};

	template<typename T, typename IdType>
		const std::vector<typename BasicNode<T, IdType>::abstract_arc_type*>
			BasicNode<T, IdType>::getIncomingArcs() {
				std::vector<abstract_arc_type*> in;
				for(auto layer : this->incomingArcs) {
					for(auto* arc : layer.second) {
						in.push_back(arc);
					}
				}
				return in;
		}

	template<typename T, typename IdType>
		const std::vector<typename BasicNode<T, IdType>::abstract_arc_type*>
			BasicNode<T, IdType>::getIncomingArcs(LayerId id) {
				return incomingArcs[id];
		}
	
	template<typename T, typename IdType>
		const std::vector<typename BasicNode<T, IdType>::abstract_arc_type*>
			BasicNode<T, IdType>::getOutgoingArcs() {
				std::vector<abstract_arc_type*> out;
				for(auto layer : this->outgoingArcs) {
					for(auto* arc : layer.second) {
						out.push_back(arc);
					}
				}
				return out;
		}

	template<typename T, typename IdType>
		const std::vector<typename BasicNode<T, IdType>::abstract_arc_type*>
			BasicNode<T, IdType>::getOutgoingArcs(LayerId id) {
				return outgoingArcs[id];
		}
	
	template<typename T, typename IdType>
		void BasicNode<T, IdType>::linkIn(abstract_arc_type* arc, LayerId layer) {
			incomingArcs[layer].push_back(arc);
		}

	template<typename T, typename IdType>
		void BasicNode<T, IdType>::linkOut(abstract_arc_type* arc, LayerId layer) {
			outgoingArcs[layer].push_back(arc);
		}

	template<typename T, typename IdType>
		void BasicNode<T, IdType>::unlinkIn(abstract_arc_type *arc, LayerId layer) {
			incomingArcs.at(layer).erase(arc);
		}

	template<typename T, typename IdType>
		void BasicNode<T, IdType>::unlinkOut(abstract_arc_type *arc, LayerId layer) {
			outgoingArcs.at(layer).erase(arc);
		}
}
#endif
