#include "graph.h"

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
std::string GraphItem::getLabel() {
	return this->label;
}
