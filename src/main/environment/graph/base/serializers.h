#ifndef SERIALIZERS_H
#define SERIALIZERS_H

#include <nlohmann/json.hpp>
#include <array>
#include "graph.h"

using nlohmann::json;

namespace FPMAS {
	namespace graph {
		template<class T> class Node;
		template<class T> class Arc;
		template<class T> class Graph;
	}
}

/**
 * Defines some rules to serialize / deserialize graph objects using the [JSON for Modern C++
 * library](https://github.com/nlohmann/json).
 */
namespace nlohmann {
	using FPMAS::graph::Node;

	/**
	 * Node serializer.
	 */
    template <class T>
    struct adl_serializer<Node<T>> {

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
		 * If a `data` field is specified, the Node data will be *instanciated
		 * dynamically using a `new` statement and the copy constructor of the
		 * specified data type T*.
		 *
		 * Some, in this case, memory will internally be allocated by the
		 * library, so it is *the responsability of the user* to free the data
		 * field.
		 *
		 * To avoid this, the `data` field can be ignored and it is still
		 * possible to manually associate data to nodes using the
		 * Node::setData(T*) function.
		 *
		 * @param j input json
		 * @return deserialized json
		 *
		 */
		static Node<T> from_json(const json& j) {
			Node<T> node = Node<T>(
					j.at("id").get<unsigned long>()
					);
			if(j.contains("data"))
				node.setData(
					new T(j.at("data").get<T>())
				);

			if(j.contains("weight"))
				node.setWeight(
					j.at("weight").get<float>()
					);
			return node;
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
		static void to_json(json& j, const Node<T>& node) {
			j = json{
				{"id", node.getId()},
				{"data", *node.getData()},
				{"weight", node.getWeight()}
			};
		}

	};

	using FPMAS::graph::Arc;
	/**
	 * Arc serializer.
	 */
	template<class T>
    struct adl_serializer<Arc<T>> {
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
		static Arc<T> from_json(const json& j) {
			std::array<unsigned long, 2> link =
				j.at("link")
				.get<std::array<unsigned long, 2>>();
			Node<T>* tempSource = new Node<T>(link[0]);
			Node<T>* tempTarget = new Node<T>(link[1]);

			return Arc<T>(
				j.at("id"),
				tempSource,
				tempTarget
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
		static void to_json(json& j, const Arc<T>& arc) {
			std::array<unsigned long, 2> link = {
				arc.getSourceNode()->getId(),
				arc.getTargetNode()->getId()
			};
			j = json{
				{"id", arc.getId()},
				{"link", link}
			};
		}

	};

	using FPMAS::graph::Graph;
	/**
	 * Graph serializer.
	 */
	template<class T>
    struct adl_serializer<Graph<T>> {

		/**
		 * This function defines rules to implicitly serialize graphs as json
		 * strings using the [JSON for Modern C++
		 * library](https://github.com/nlohmann/json/). For more information
		 * about how to serialize objects, see the [JSON library
		 * documentation](https://github.com/nlohmann/json/blob/develop/README.md#arbitrary-types-conversions)
		 * and the small example given in the Graph class documentation.
		 *
		 * Graph can be deserialized from json with the same format as the one
		 * described in the to_json(json&, const Graph<T>&) function.
		 *
		 * As for the to_json(json&, const Graph<T>&) function, any user data
		 * type can be properly deserialize as long as the proper [from_json
		 * function](https://github.com/nlohmann/json/blob/develop/README.md#basic-usage)
		 * has been defined.
		 *
		 * @param j input json
		 * @return deserialized graph instance
		 *
		 */
		static Graph<T> from_json(const json& j) {
			Graph<T> graph;
			// Builds nodes
			json nodes = j.at("nodes");
			for(json& node : nodes) {
				graph.buildNode(
					node.get<Node<T>>()
					);
			}

			// Build arcs
			json arcs = j.at("arcs");
			for(json& arc : arcs) {
				Arc<T> tempArc = arc.get<Arc<T>>();
				graph.link(
					tempArc.getSourceNode()->getId(),
					tempArc.getTargetNode()->getId(),
					tempArc.getId()
					);
				delete tempArc.getSourceNode();
				delete tempArc.getTargetNode();
			}
			return graph;
		}

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
		 * to_json(json&, const Node<T>&) function, allowing the user to
		 * serialize any custom graph without having to know about the internal
		 * serialization processes.
		 *
		 * Note that a reference to the graph object, not a reference to the
		 * pointer, must be used as argument.
		 * ```{.cpp}
		 * Graph<T> g = new Graph<T>();
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
		 * @param graph graph reference (NOT reference to pointer)
		 */
		static void to_json(json& j, const Graph<T>& graph) {
			std::vector<Node<T>> nodes;
			for(auto n : graph.getNodes()) {
				nodes.push_back(*n.second);
			}

			std::vector<Arc<T>> arcs;
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
