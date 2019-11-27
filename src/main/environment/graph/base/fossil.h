#ifndef FOSSIL_H
#define FOSSIL_H

#include <vector>

#include "node.h"
#include "arc.h"

namespace FPMAS {
	namespace graph {
		template<class T> class Fossil {
			public:
			std::vector<Node<T>*> nodes;
			std::vector<Arc<T>*> arcs;
			void merge(Fossil<T> fossil);
		};

		template<class T> void Fossil<T>::merge(Fossil<T> fossil) {
			for(auto arc : fossil.arcs) {
				this->arcs.push_back(arc);
			}
			for(auto node : fossil.nodes) {
				this->nodes.push_back(node);
			}
		}
	}
}
#endif
