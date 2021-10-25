#ifndef FPMAS_DISTRIBUTED_EDGE_H
#define FPMAS_DISTRIBUTED_EDGE_H

/** \file src/fpmas/graph/distributed_edge.h
 * DistributedEdge implementation.
 */

#include "fpmas/graph/edge.h"
#include "fpmas/graph/distributed_node.h"
#include "fpmas/communication/communication.h"

namespace fpmas { namespace graph {

	using api::graph::LayerId;

	/**
	 * api::graph::DistributedEdge implementation.
	 */
	template<typename T>
		class DistributedEdge :
			public graph::Edge<DistributedId, api::graph::DistributedNode<T>>,
			public api::graph::DistributedEdge<T> {
				typedef graph::Edge<DistributedId, api::graph::DistributedNode<T>>
					EdgeBase;

				private:
					LocationState _state = LocationState::LOCAL;
					std::unique_ptr<api::graph::TemporaryNode<T>> temp_src;
					std::unique_ptr<api::graph::TemporaryNode<T>> temp_tgt;

				public:
					using typename graph::Edge<DistributedId, api::graph::DistributedNode<T>>::IdType;

					/**
					 * DistributedEdge constructor.
					 *
					 * @param id edge id
					 * @param layer id of the layer on which the edge is located
					 */
					DistributedEdge(DistributedId id, LayerId layer)
						: EdgeBase(id, layer) {}

					api::graph::LocationState state() const override {return _state;}
					void setState(api::graph::LocationState state) override {this->_state = state;}

					void setTempSourceNode(
							std::unique_ptr<fpmas::api::graph::TemporaryNode<T>> temp_src
							) override {
						this->temp_src = std::move(temp_src);
					}
					std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>
						getTempSourceNode() override {
						return std::move(temp_src);
					};
					void setTempTargetNode(
							std::unique_ptr<fpmas::api::graph::TemporaryNode<T>> temp_tgt
							) override {
						this->temp_tgt = std::move(temp_tgt);
					}
					std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>
						getTempTargetNode() override {
						return std::move(temp_tgt);
					}

			};

	/**
	 * nlohmann::json based TemporaryNode implementation.
	 *
	 * The json representing the node is stored within the class.
	 * If build() is called, a new node is allocated from the deserialized
	 * json.
	 *
	 * @tparam T node data type
	 * @tparam JsonType json type, that defines deserialization rules (e.g.: `nlohmann::json` or `fpmas::io::json::light_json`)
	 */
	template<typename T, typename JsonType>
		class JsonTemporaryNode : public api::graph::TemporaryNode<T> {
			private:
				bool parsed_json;
				std::string json_str;
				JsonType j;
				DistributedId id;
				int location;

			public:
				/**
				 * JsonTemporaryNode constructor.
				 * 
				 * The provided json instance is
				 * [forwarded](https://en.cppreference.com/w/cpp/utility/forward)
				 * to the internal json instance.
				 */
				template<typename _JsonType>
					JsonTemporaryNode(DistributedId id, int location, _JsonType&& j) :
						parsed_json(true),
						j(std::forward<_JsonType>(j)), id(id), location(location) {
						}

				JsonTemporaryNode(
						DistributedId id, int location, std::string json_str) :
					parsed_json(false),
					json_str(json_str), id(id), location(location) {
					}

				DistributedId getId() const override {
					return id;
				}

				int getLocation() const override {
					return location;
				}

				api::graph::DistributedNode<T>* build() override {
					if(!parsed_json)
						j = JsonType::parse(json_str);
					NodePtrWrapper<T> node = j.template get<NodePtrWrapper<T>>();
					node->setLocation(location);
					return node;
				}
		};

	/**
	 * Alias for a DistributedEdge PtrWrapper
	 */
	template<typename T>
	using EdgePtrWrapper = api::utils::PtrWrapper<api::graph::DistributedEdge<T>>;
}}

namespace nlohmann {
	using fpmas::graph::EdgePtrWrapper;

	/**
	 * \anchor adl_serializer_DistributedEdge
	 *
	 * DistributedEdge JSON serialization.
	 */
	template<typename T>
		struct adl_serializer<EdgePtrWrapper<T>> {
			/**
			 * DistributedEdge json serialization.
			 *
			 * Source and target \DistributedNode are unserialized depending on
			 * the provided `JsonType`, that might be the classic
			 * `nlohmann::json` type or fpmas::io::json::light_json.
			 *
			 * @see \ref adl_serializer_DistributedNode_to_json
			 * "adl_serializer<NodePtrWrapper<T>>::to_json()"
			 * @see \ref light_serializer_DistributedNode_to_json
			 * "light_serializer<NodePtrWrapper<T>>::to_json()"
			 *
			 * @param j json
			 * @param edge wrapper of pointer to the DistributedEdge to serialize
			 */
			template<typename JsonType>
			static void to_json(JsonType& j, const EdgePtrWrapper<T>& edge) {
				j["id"] = edge->getId();
				j["layer"] = edge->getLayer();
				j["weight"] = edge->getWeight();
				j["src"] = {{}, edge->getSourceNode()->location()};
				NodePtrWrapper<T> src(edge->getSourceNode());
				j["src"][0] = src;
				j["tgt"] = {{}, edge->getTargetNode()->location()};
				NodePtrWrapper<T> tgt(edge->getTargetNode());
				j["tgt"][0] = tgt;
			}

			/**
			 * DistributedEdge json unserialization.
			 *
			 * Source and target \DistributedNode are unserialized depending on
			 * the provided `JsonType`, that might be the classic
			 * `nlohmann::json` type or fpmas::io::json::light_json.
			 *
			 * @see \ref adl_serializer_DistributedNode_from_json
			 * "nlohmann::adl_serializer<NodePtrWrapper<T>>::from_json()"
			 * @see \ref light_serializer_DistributedNode_from_json
			 * "light_serializer<NodePtrWrapper<T>>::from_json()"
			 *
			 * @param j json
			 * @param edge_ptr output edge
			 */
			template<typename JsonType>
			static void from_json(const JsonType& j, EdgePtrWrapper<T>& edge_ptr) {
				fpmas::api::graph::DistributedEdge<T>* edge
					= new fpmas::graph::DistributedEdge<T> {
					j.at("id").template get<DistributedId>(),
					j.at("layer").template get<typename fpmas::graph::LayerId>()
				};
				edge->setWeight(j.at("weight").template get<float>());

				edge->setTempSourceNode(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
						new fpmas::graph::JsonTemporaryNode<T, JsonType>(
							j.at("src")[0].at("id").template get<DistributedId>(),
							j.at("src")[1].template get<int>(),
							std::move(j.at("src")[0])
							)
						));

				edge->setTempTargetNode(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
						new fpmas::graph::JsonTemporaryNode<T, JsonType>(
							j.at("tgt")[0].at("id").template get<DistributedId>(),
							j.at("tgt")[1].template get<int>(),
							std::move(j.at("tgt")[0])
							)
						));

				edge_ptr = {edge};
			}
		};
}

namespace fpmas { namespace communication {

	template<typename T>
		struct Serializer<fpmas::graph::EdgePtrWrapper<T>> {
			static DataPack serialize(const fpmas::graph::EdgePtrWrapper<T>& edge) {
				constexpr std::size_t id_size = sizeof(int) + sizeof(FPMAS_ID_TYPE);
				std::string src = fpmas::io::json::light_json(
						fpmas::graph::NodePtrWrapper<T>(edge->getSourceNode())
						).dump();
				std::string tgt = fpmas::io::json::light_json(
						fpmas::graph::NodePtrWrapper<T>(edge->getTargetNode())
						).dump();

				std::size_t size =
					pack_size<DistributedId>() + // edge id
					pack_size<int>() + // layer
					pack_size<float>() + // edge weight
					pack_size<DistributedId>() + // src id
					pack_size<int>() + // src location
					pack_size(src) + // src json
					pack_size<DistributedId>() + // tgt id
					pack_size<int>() + // tgt location
					pack_size(tgt); // tgt json

				DataPack pack(size, 1);

				std::size_t current_offset = 0;
				communication::serialize(pack, edge->getId(), current_offset);
				communication::serialize(pack, edge->getLayer(), current_offset);
				communication::serialize(pack, edge->getWeight(), current_offset);
				communication::serialize(
						pack, edge->getSourceNode()->getId(), current_offset);
				communication::serialize(
						pack, edge->getSourceNode()->location(), current_offset);
				communication::serialize(pack, src, current_offset);
				communication::serialize(
						pack, edge->getTargetNode()->getId(), current_offset);
				communication::serialize(
						pack, edge->getTargetNode()->location(), current_offset);
				communication::serialize(pack, tgt, current_offset);

				return pack;
			}

			static fpmas::graph::EdgePtrWrapper<T> deserialize(const DataPack& pack) {
				std::size_t current_offset = 0;
				DistributedId edge_id;
				int edge_layer;
				float edge_weight;

				communication::deserialize<DistributedId>(pack, edge_id, current_offset);
				communication::deserialize<int>(pack, edge_layer, current_offset);
				communication::deserialize<float>(pack, edge_weight, current_offset);

				fpmas::api::graph::DistributedEdge<T>* edge
					= new fpmas::graph::DistributedEdge<T>(edge_id, edge_layer);
				edge->setWeight(edge_weight);

				DistributedId id;
				int loc;
				std::string json;

				communication::deserialize<DistributedId>(pack, id, current_offset);
				communication::deserialize<int>(pack, loc, current_offset);
				communication::deserialize<std::string>(pack, json, current_offset);

				edge->setTempSourceNode(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
						new fpmas::graph::JsonTemporaryNode<T, fpmas::io::json::light_json>(
							id, loc, json
							)
						));

				communication::deserialize<DistributedId>(pack, id, current_offset);
				communication::deserialize<int>(pack, loc, current_offset);
				communication::deserialize<std::string>(pack, json, current_offset);

				edge->setTempTargetNode(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
						new fpmas::graph::JsonTemporaryNode<T, fpmas::io::json::light_json>(
							id, loc, json
							)
						));

				return {edge};
			}
		};

	/**
	 * \anchor TypedMpiDistributedEdge
	 *
	 * \DistributedEdge TypedMpi specialization, used to bypass the default
	 * fpmas::communication::TypedMpi specialization based on nlohmann::json.
	 *
	 * Here, fpmas::io::json::light_json is used instead. This allows to use
	 * "light" serialization rules of source and target \DistributedNodes,
	 * since sending their data is not required when transmitting
	 * \DistributedEdges.
	 *
	 * @tparam T type of data contained in associated \DistributedNodes
	 */
	/*
	 *template<typename T>
	 *    struct TypedMpi<fpmas::graph::EdgePtrWrapper<T>> : public detail::TypedMpi<fpmas::graph::EdgePtrWrapper<T>, fpmas::io::json::light_json> {
	 *        using detail::TypedMpi<fpmas::graph::EdgePtrWrapper<T>, fpmas::io::json::light_json>::TypedMpi;
	 *    };
	 */

}}
#endif
