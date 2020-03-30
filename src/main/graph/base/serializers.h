#ifndef SERIALIZERS_H
#define SERIALIZERS_H

#include <nlohmann/json.hpp>
#include <array>
#include "utils/macros.h"
#include "graph.h"

using nlohmann::json;

namespace FPMAS::graph::base {
	template<typename, typename> class Node;
	template<typename, typename> class Arc;
	template<typename, typename> class Graph;
}

/**
 * Defines some rules to serialize / deserialize graph objects using the [JSON for Modern C++
 * library](https://github.com/nlohmann/json).
 */
namespace nlohmann {
	using FPMAS::graph::base::LayerId;
	using FPMAS::graph::base::Node;

	/**
	 * Node serializer.
	 */
    template <typename T, typename IdType>
    struct adl_serializer<Node<T, IdType>> {

		/**
		 * Defines rules to deserialize a Node from a JSON string.
		 *
		 * The following JSON format can be used to unserialize nodes :
		 * ```{.json}
		 * {
		 * 	"id":"<node_id>",
		 * 	"data":<json_data_representation>,
		 * 	"weight":<node_weight>
		 * }
		 * ```
		 * `data` and `weight` fields are both optional, with a default weight
		 * value set to 1.
		 *
		 * If a `data` field is specified, the Node data will be unserialized
		 * using the proper nlohmann deserialization function. It might be the
		 * responsability of the user to define those function.
		 *
		 * Else, the `data` field can be ignored and it is still
		 * possible to manually associate data to nodes using the
		 * Node::setData(T) function.
		 *
		 * @param j input json
		 * @return deserialized json
		 *
		 */
		static Node<T, IdType> from_json(const json& j) {
			return Node<T, IdType>(
					j.at("id").get<IdType>(),
					j.at("weight").get<float>(),
					j.at("data").get<T>()
					);
		}

		/**
		 * Defines rules to serialize node instances in the following general
		 * JSON format :
		 * ```{.json}
		 * {
		 * 	"id":"<node_id>",
		 * 	"data":<json_data_representation>,
		 * 	"weight":<node_weight>
		 * }
		 * ```
		 * The huge interest of this json implementation is that the [JSON for
		 * Modern C++ library](https://github.com/nlohmann/json/) will
		 * automatically calls the appropriate functions to serialize the `data`
		 * field, whatever its type is. In consecuence, users can specify their
		 * own [to_json
		 * function](https://github.com/nlohmann/json/blob/develop/README.md#basic-usage)
		 * to serialize their data type, and any node (and so any graph) can be serialized
		 * properly without altering or even know about this internal node and graph
		 * serialization process.
		 *
		 * The to_json(json&, const Graph<T>&) implicitly uses this function to
		 * serialize graphs.
		 *
		 * @param j current json reference
		 * @param node node reference
		 */
		static void to_json(json& j, const Node<T, IdType>& node) {
			json data = node.data();
			j = json{
				{"id", node.getId()},
				{"data", data},
				{"weight", node.getWeight()}
			};
		}

	};

	using FPMAS::graph::base::Arc;
	/**
	 * Arc serializer.
	 */
	template<typename T, typename IdType>
    struct adl_serializer<Arc<T, IdType>> {
		/**
		 * Defines rules to unserialize Arcs.
		 *
		 * Format used :
		 * ```{.json}
		 * {"id":<arc_id>,"link":[<source_node_id>, <target_node_id>]}
		 * ```
		 *
		 * It is very important to node that in order to build a valid Arc
		 * instance, temporary source and target nodes are created from a
		 * dynamically allocated memory, and so must be deleted later.
		 *
		 * Those nodes actually only contain the IDs respectively defined in
		 * the `link` field.
		 *
		 * As an example, the FPMAS::graph::Graph::link(Arc<T>) function will
		 * take such an arc as argument, build a real arc from the deserialized
		 * arc, and correctly delete the temporary fake nodes.
		 *
		 * @param j json to deserialize
		 * @return temporary arc
		 */
		static Arc<T, IdType> from_json(const json& j) {
			auto link = j.at("link");
			Node<T, IdType>* tempSource = new Node<T, IdType>(
					link[0].template get<IdType>()
					);
			Node<T, IdType>* tempTarget = new Node<T, IdType>(
					link[1].template get<IdType>()
					);

			return Arc<T, IdType>(
				j.at("id").get<IdType>(),
				tempSource,
				tempTarget,
				j.at("layer").get<LayerId>()
				);

		}


		/**
		 * Defines rules to serialize arc instances in the following general
		 * JSON format :
		 * ```{.json}
		 * {"id":<arc_id>,"link":[<source_node_id>, <target_node_id>]}
		 * ```
		 * The to_json(json&, const Graph<T>&) implicitly uses this function to
		 * serialize graphs.
		 *
		 * @param j current json reference
		 * @param arc arc reference
		 */
		static void to_json(json& j, const Arc<T, IdType>& arc) {
			std::array<IdType, 2> link = {
				arc.getSourceNode()->getId(),
				arc.getTargetNode()->getId()
			};
			j = json{
				{"id", arc.getId()},
				{"link", {
					arc.getSourceNode()->getId(),
					arc.getTargetNode()->getId()
						 }
				},
				{"layer", arc.layer}
			};
		}

	};

	using FPMAS::graph::base::Graph;
	/**
	 * Graph serializer.
	 */
	template<typename T, typename IdType>
    struct adl_serializer<Graph<T, IdType>> {
		/**
		 * This function defines rules to implicitly serialize graphs as json
		 * strings using the [JSON for Modern C++
		 * library](https://github.com/nlohmann/json/). For more information
		 * about how to serialize objects, see the [JSON library
		 * documentation](https://github.com/nlohmann/json/blob/develop/README.md#arbitrary-types-conversions)
		 * and the small example given in the Graph class documentation.
		 *
		 * The general JSON (prettified) format used to represent graph is as
		 * follow :
		 * ```{.json}
		 * {
		 * 	"arcs":[
		 * 		...
		 * 		{"id":"<arc_id>","link":["<source_node_id>","<target_node_id>"]},
		 * 		...
		 * 		],
		 * 	"nodes":[
		 * 		...
		 * 		{"id":"<node_id>","data":<json_data_representation>,"weight":<node_weight>},
		 * 		...
		 * 		]
		 * }
		 * ```
		 * Notice that the real serialized version does not contains any
		 * indentation, line breaks or spaces.
		 *
		 * Any custom data type can be easily serialized, as explained in the
		 * to_json(json&, const Node<T, LayerType, N>&) function, allowing the user to
		 * serialize any custom graph without having to know about the internal
		 * serialization processes.
		 *
		 * Note that a reference to the graph object, not a reference to the
		 * pointer, must be used as argument.
		 * ```{.cpp}
		 * Graph<T>* g = new Graph<T>();
		 *
		 * // Do processing to build the graph
		 *
		 * // This WON'T WORK
		 * json j = g;
		 *
		 * // This is ok
		 * json j = *g;
		 *
		 * delete g;
		 * ```
		 *
		 * @param j current json reference
		 * @param graph graph reference
		 */
		static void to_json(json& j, const Graph<T, IdType>& graph) {
			std::vector<Node<T, IdType>> nodes;
			for(auto n : graph.getNodes()) {
				nodes.push_back(*n.second);
			}

			std::vector<Arc<T, IdType>> arcs;
			for(auto a : graph.getArcs()) {
				arcs.push_back(*a.second);
			}

			j = json{
				{"nodes", nodes},
				{"arcs", arcs}
			};
		}

	};
}
#endif
