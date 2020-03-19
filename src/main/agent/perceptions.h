#ifndef PERCEPTIONS_H
#define PERCEPTIONS_H

#include "utils/macros.h"
#include <type_traits>
#include <tuple>
#include <vector>

using FPMAS::graph::base::LayerId;

namespace FPMAS::agent {

	template<
		typename node_type
		> class Perception {
			public:
			const node_type* node;
			Perception(node_type* node) : node(node) {}
		};

	template<
		typename node_type
	> class PerceptionSet {
		private:
			std::vector<Perception<node_type>> set;

		public:
			PerceptionSet(const node_type* node, LayerId layer) {
				auto neighbors = node->layer(layer).outNeighbors();
					for(auto neighbor : neighbors) {
						set.push_back(Perception<node_type>(neighbor));
					}
			}
			const std::vector<Perception<node_type>>& get() {
				return set;
			}
	};

	template<
		typename node_type,
		LayerId... layers
	> class Perceptions {
		private:
			const std::array<PerceptionSet<node_type>, sizeof...(layers)>
				perceptions;
			std::vector<Perception<node_type>> concat_perceptions;

		public:
			Perceptions(const node_type* node) :
				perceptions {PerceptionSet(node,layers)...} {
				for(auto _perceptions : perceptions) {
					for(auto item : _perceptions.get()) {
						concat_perceptions.push_back(item);
					}
				}
			}

			std::vector<Perception<node_type>> get() {
					return concat_perceptions;
				}
	};
}

#endif
