#ifndef SERIALIZERS_H
#define SERIALIZERS_H

#include <nlohmann/json.hpp>
#include "graph.h"

using nlohmann::json;

template<class T> void to_json(json& j, const Node<T>& n) {
	j = json{{"label", n.getLabel()}, {"data", *n.getData()}};
};

#endif
