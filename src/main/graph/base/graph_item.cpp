#include "graph_item.h"

using FPMAS::graph::GraphItem;

/**
 * Default GraphItem constructor.
 */
GraphItem::GraphItem() {
}

/**
 * GraphItem constructor.
 *
 * @param id graph item id
 */
GraphItem::GraphItem(unsigned long id) : id(id) {

}

/**
 * Returns the id of this graph item.
 *
 * @return id of this item
 */
unsigned long GraphItem::getId() const {
	return this->id;
}
