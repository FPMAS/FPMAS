#include "graph.h"

using FPMAS::graph::GraphItem;

/**
 * GraphItem constructor.
 *
 * @param label graph item label
 */
GraphItem::GraphItem(std::string label) : label(label) {

}

/**
 * Returns the label of this graph item.
 *
 * @return label of this item
 */
std::string GraphItem::getLabel() const {
	return this->label;
}
