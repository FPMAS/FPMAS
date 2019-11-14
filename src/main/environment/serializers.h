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
		template<class T> void to_json(json& j, const FPMAS::graph::Node<T>& n) {
			j = json{{"id", n.getId()}, {"data", *n.getData()}};
		}

		// Arc
		template<class T> class Arc;
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
		template<class T> void to_json(json& j, const FPMAS::graph::Graph<T>& graph) {
			std::vector<Node<T>> nodes;
			for(auto n : graph.nodes) {
				nodes.push_back(*n.second);
			}

			std::vector<Arc<T>> arcs;
			for(auto a : graph.arcs) {
				arcs.push_back(*a.second);
			}

			j = json{
				{"nodes", nodes},
				{"arcs", arcs}
			};

		}

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
