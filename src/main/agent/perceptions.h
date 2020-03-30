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
			node_type* node;
			Perception(const node_type* node) : node(const_cast<node_type*>(node)) {}
		};

	template<
		typename node_type,
		LayerId layer
	> class TypedPerception : public Perception<node_type> {
		public:
			TypedPerception(const node_type* node)
				: Perception<node_type>(node) {}

	};

	template<
		typename node_type,
		LayerId layer
	> class PerceptionSet {
		private:
			std::vector<TypedPerception<node_type, layer>> set;

		public:
			PerceptionSet(const node_type* node) {
				auto neighbors = node->layer(layer).outNeighbors();
					for(auto neighbor : neighbors) {
						set.push_back(TypedPerception<node_type, layer>(neighbor));
					}
			}

			const std::vector<TypedPerception<node_type, layer>>& get() const {
				return set;
			}
	};

	template<
		typename node_type,
		LayerId... layers
	> class Perceptions {
		private:
			const std::tuple<PerceptionSet<node_type, layers>...>
				perceptions;
			std::vector<Perception<node_type>> concat_perceptions;

			template<LayerId layer> void merge(PerceptionSet<node_type, layer> perception_set, std::vector<Perception<node_type>>& perceptions ) const {
				for(auto item : perception_set.get()) {
					perceptions.push_back(item);
				}
			};

		public:
			Perceptions(const node_type* node) :
				perceptions {PerceptionSet<node_type, layers>(node)...} {
				(merge(std::get<PerceptionSet<node_type, layers>>(perceptions), concat_perceptions),...);
			}

			template<LayerId layer> const std::vector<TypedPerception<node_type, layer>>& get() const {
				return std::get<PerceptionSet<node_type, layer>>(perceptions).get();
			}

			const std::vector<Perception<node_type>>& get() const {
				return concat_perceptions;
			}
	};
}

#endif
