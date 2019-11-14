#include "graph.h"

using FPMAS::graph::GraphItem;

/**
 * GraphItem constructor.
 *
 * @param id graph item id
 */
GraphItem::GraphItem(std::string id) : id(id) {

}

/**
 * Returns the id of this graph item.
 *
 * @return id of this item
 */
std::string GraphItem::getId() const {
	return this->id;
}
