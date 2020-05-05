#ifndef PERCEPTIONS_H
#define PERCEPTIONS_H

#include "utils/macros.h"
#include "environment/grid_layers.h"
#include <type_traits>
#include <tuple>
#include <vector>

using FPMAS::graph::base::LayerId;

namespace FPMAS::agent {

	template<
		typename node_type,
		LayerId layer
	> class Perception {
		private:
			node_type* _node;
		public:
			Perception(const node_type* node) : _node(const_cast<node_type*>(node)) {}

			node_type* node() {
				return _node;
			}


	};

	template<typename node_type> class PerceptionsCollector {
		protected:
			template<LayerId layer> std::vector<Perception<node_type, layer>> collectLayer(const node_type* node) {
				std::vector<Perception<node_type, layer>> perceptions;
				auto neighbors = node->layer(layer).outNeighbors();
				for(auto neighbor : neighbors) {
					perceptions.push_back(Perception<node_type, layer>(neighbor));
				}
				return perceptions;
			}

			virtual void collect(const node_type*) = 0;

	};

	template<typename node_type, LayerId layer> class DefaultCollector : public PerceptionsCollector<node_type> {
		private:
			std::vector<Perception<node_type, layer>> perceptions;
		public:
			DefaultCollector(const node_type* node) {
				collect(node);
			}

			void collect(const node_type* node) override {
				perceptions = this->template collectLayer<layer>(node);
			}

			std::vector<Perception<node_type, layer>> get() {
				return perceptions;
			}
	};

	using FPMAS::environment::grid::MOVABLE_TO;
	template<typename node_type> class MovableTo : public DefaultCollector<node_type, MOVABLE_TO> {
		public:
			MovableTo(const node_type* node)
				: DefaultCollector<node_type, MOVABLE_TO>(node) {};

	};
}

#endif
