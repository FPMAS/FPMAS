#ifndef GRAPH_ITEM_H
#define GRAPH_ITEM_H

namespace FPMAS::graph::base {
	/**
	 * A labelled object, super class for Nodes and Arcs.
	 */
	template<typename id_t> class GraphItem {
		private:
			id_t id;
		protected:
			GraphItem();
			GraphItem(id_t);
		public:
			id_t getId() const;
	};
	/**
	 * Default GraphItem constructor.
	 */
	template<typename id_t> GraphItem<id_t>::GraphItem() {
	}

	/**
	 * GraphItem constructor.
	 *
	 * @param id graph item id
	 */
	template<typename id_t> GraphItem<id_t>::GraphItem(id_t id) : id(id) {

	}

	/**
	 * Returns the id of this graph item.
	 *
	 * @return id of this item
	 */
	template<typename id_t> id_t GraphItem<id_t>::getId() const {
		return this->id;
	}
}

#endif
