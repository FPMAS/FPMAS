#ifndef SERIALIZERS_H
#define SERIALIZERS_H

#include <nlohmann/json.hpp>
#include <array>
#include "graph.h"

using nlohmann::json;


namespace FPMAS {
	namespace graph {

		// Node
		template<class T> class Node;
		/**
		 * Defines rules to serialize node instances in the following general
		 * JSON format :
		 * ```json
		 * {"id":"<node_id>", "data":<json_data_representation>}
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
		 * @param n node reference
		 */
		template<class T> void to_json(json& j, const FPMAS::graph::Node<T>& n) {
			j = json{{"id", n.getId()}, {"data", *n.getData()}};
		}

		// Arc
		template<class T> class Arc;
		/**
		 * Defines rules to serialize arc instances in the following general
		 * JSON format :
		 * ```{.json}
		 * {"id":"<arc_id>","link":["<source_node_id>", "<target_node_id>"]}
		 * ```
		 * The to_json(json&, const Graph<T>&) implicitly uses this function to
		 * serialize graphs.
		 *
		 * @param j current json reference
		 * @param a arc reference
		 */
		template<class T> void to_json(json& j, const FPMAS::graph::Arc<T>& a) {
			std::array<std::string, 2> link = {
				a.getSourceNode()->getId(),
				a.getTargetNode()->getId()
			};
			j = json{
				{"id", a.getId()},
				{"link", link}
			};
		}

		// Graph
		template<class T> class Graph;

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
		 * 		{"id":"<node_id>","data":<json_data_representation>},
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
		template<class T> void to_json(json& j, const FPMAS::graph::Graph<T>& graph) {
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
		 *
		 */
		template<class T> void from_json(const json& j, FPMAS::graph::Graph<T>& graph) {
			// Builds nodes
			json nodes = j.at("nodes");
			for(json& node : nodes) {
				graph.buildNode(
					node.at("id").get<std::string>(),
					new T(node.at("data").get<T>())
					);
			}

			// Build arcs
			json arcs = j.at("arcs");
			for(json& arc : arcs) {
				std::array<std::string, 2> link =
					arc.at("link")
					.get<std::array<std::string, 2>>();
				graph.link(
					graph.getNode(link.at(0)),
					graph.getNode(link.at(1)),
					arc.at("id").get<std::string>()
					);
			}
		}
	}
};

#endif
