#ifndef GRAPH_ITEM_H
#define GRAPH_ITEM_H

namespace FPMAS {
	namespace graph {
		/**
		 * A labelled object, super class for Nodes and Arcs.
		 */
		class GraphItem {
			private:
				unsigned long id;
			protected:
				GraphItem();
				GraphItem(unsigned long);
			public:
				unsigned long getId() const;
		};
	}
}

#endif
