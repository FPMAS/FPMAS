#ifndef FPMAS_MODEL_H
#define FPMAS_MODEL_H

/** \file src/fpmas/model/model.h
 * Model implementation.
 */

#include "fpmas/api/graph/graph_builder.h"
#include "guards.h"
#include "detail/model.h"

#include <random>

namespace fpmas { namespace model {

	/**
	 * Helper class to represent an Agent as the neighbor of an other.
	 *
	 * A Neighbor can be used through pointer like accesses, and the input
	 * Agent is automatically downcast to AgentType.
	 *
	 * @tparam AgentType Type of the input agent
	 */
	template<typename AgentType>
		class Neighbor : public api::model::Neighbor<AgentType> {
			private:
				AgentPtr* agent;
				AgentEdge* _edge = nullptr;

			public:
				/**
				 * @deprecated
				 *
				 * Neighbor constructor.
				 *
				 * @param agent pointer to a generic AgentPtr
				 */
				Neighbor(AgentPtr* agent)
					: agent(agent) {}

				Neighbor(AgentPtr* agent, AgentEdge* edge)
					: agent(agent), _edge(edge) {}

				/**
				 * Implicit conversion operator to `AgentType*`.
				 */
				operator AgentType*() const override {
					return dynamic_cast<AgentType*>(agent->get());
				}

				/**
				 * Member of pointer access operator.
				 *
				 * @return pointer to neighbor agent
				 */
				AgentType* operator->() const override {
					return dynamic_cast<AgentType*>(agent->get());
				}

				/**
				 * Indirection operator.
				 *
				 * @return reference to neighbor agent
				 */
				AgentType& operator*() const override {
					return *dynamic_cast<AgentType*>(agent->get()); }

				AgentEdge* edge() const override {
					return _edge;
				}
		};

	/**
	 * Helper class used to represent \Agents neighbors.
	 *
	 * Neighbors are defined as \Agents contained in neighbors of the node
	 * associated to a given Agent.
	 *
	 * This class allows to easily iterate over \Agents neighbors thanks to
	 * predefined iterators and the shuffle() method.
	 *
	 * @tparam AgentType Type of agents in the neighbors set
	 */
	template<typename AgentType>
		class Neighbors {
			private:
				typedef typename std::vector<Neighbor<AgentType>>::iterator iterator;
				typedef typename std::vector<Neighbor<AgentType>>::const_iterator const_iterator;
				std::vector<Neighbor<AgentType>> neighbors;
				static std::mt19937 rd;

			public:
				/**
				 * Neighbors constructor.
				 *
				 * @param neighbors list of agent neighbors
				 */
				Neighbors(const std::vector<Neighbor<AgentType>>& neighbors)
					: neighbors(neighbors) {}

				/**
				 * Begin iterator.
				 *
				 * @return begin iterator
				 */
				iterator begin() {
					return neighbors.begin();
				}
				/**
				 * Const begin iterator.
				 *
				 * @return const begin iterator
				 */
				const_iterator begin() const {
					return neighbors.begin();
				}

				/**
				 * End iterator.
				 *
				 * @return end iterator
				 */
				iterator end() {
					return neighbors.end();
				}

				/**
				 * Const end iterator.
				 *
				 * @return const end iterator
				 */
				const_iterator end() const {
					return neighbors.end();
				}

				/**
				 * Returns the count of neighbors in this list.
				 *
				 * @return neighbors count
				 */
				std::size_t count() const {
					return neighbors.size();
				}

				/**
				 * Returns a reference to the neighbor at position `i`, with
				 * bounds checking.
				 *
				 * @param i neighbor position
				 *
				 * @throws std::out_of_range if `!(i < count())`
				 */
				Neighbor<AgentType>& at(std::size_t i) {
					try {
						return neighbors.at(i);
					} catch(const std::out_of_range&) {
						throw;
					}
				}

				/**
				 * \copydoc at
				 */
				const Neighbor<AgentType>& at(std::size_t i) const {
					try {
						return neighbors.at(i);
					} catch(const std::out_of_range&) {
						throw;
					}
				}

				/**
				 * Returns a reference to the neighbor at position `i`, without
				 * bounds checking.
				 *
				 * @param i neighbor position
				 */
				Neighbor<AgentType>& operator[](std::size_t i) {
					return neighbors[i];
				}

				/**
				 * \copydoc operator[]
				 */
				const Neighbor<AgentType>& operator[](std::size_t i) const {
					return neighbors[i];
				}

				/**
				 * Internally shuffles the neighbors list using
				 * [std::shuffle](https://en.cppreference.com/w/cpp/algorithm/random_shuffle).
				 *
				 * Can be used to iterate randomly over the neighbors set.
				 *
				 * @return reference to this Neighbors instance
				 */
				Neighbors& shuffle() {
					std::shuffle(neighbors.begin(), neighbors.end(), rd);
					return *this;
				}
		};
	template<typename AgentType> std::mt19937 Neighbors<AgentType>::rd;

	/**
	 * Base implementation of the \Agent API.
	 *
	 * Users can use this class to easily define their own \Agents with custom
	 * behaviors.
	 */
	template<typename AgentType>
	class AgentBase : public virtual api::model::Agent {
		public:
			static const api::model::TypeId TYPE_ID;

		private:
			api::model::GroupId group_id;
			api::model::AgentGroup* _group;
			api::model::AgentTask* _task;
			api::model::AgentNode* _node;
			api::model::Model* _model;
		public:
			api::model::GroupId groupId() const override {return group_id;}
			void setGroupId(api::model::GroupId id) override {this->group_id = id;}

			api::model::AgentGroup* group() override {return _group;}
			const api::model::AgentGroup* group() const override {return _group;}
			void setGroup(api::model::AgentGroup* group) override {this->_group = group;}

			api::model::TypeId typeId() const override {return TYPE_ID;}
			api::model::Agent* copy() const override {return new AgentType(*dynamic_cast<const AgentType*>(this));}

			void copyAssign(api::model::Agent* agent) override {
				// Uses AgentType copy assignment operator
				*dynamic_cast<AgentType*>(this) = *dynamic_cast<const AgentType*>(agent);
			}

			void moveAssign(api::model::Agent* agent) override {
				// Sets and overrides the fields that must be preserved
				agent->setGroup(this->group());
				agent->setTask(this->task());
				agent->setNode(this->node());
				agent->setModel(this->model());

				// Uses AgentType move assignment operator
				*dynamic_cast<AgentType*>(this) = std::move(*dynamic_cast<const AgentType*>(agent));
			}

			api::model::AgentNode* node() override {return _node;}
			const api::model::AgentNode* node() const override {return _node;}
			void setNode(api::model::AgentNode* node) override {_node = node;}

			api::model::Model* model() override {return _model;}
			const api::model::Model* model() const override {return _model;}
			void setModel(api::model::Model* model) override {_model = model;}

			api::model::AgentTask* task() override {return _task;}
			const api::model::AgentTask* task() const override {return _task;}
			void setTask(api::model::AgentTask* task) override {_task = task;}

			/**
			 * Returns a typed list of agents that are out neighbors of the current
			 * agent.
			 *
			 * Agents are added to the list if and only if :
			 * 1. they are contained in a node that is an out neighbor of this
			 * agent's node
			 * 2. they can be cast to `NeighborAgentType`
			 *
			 * @return out neighbor agents
			 *
			 * @see api::graph::Node::outNeighbors()
			 */
			template<typename NeighborAgentType> Neighbors<NeighborAgentType> outNeighbors() const {
				std::vector<Neighbor<NeighborAgentType>> out;
				for(api::model::AgentNode* _node : node()->outNeighbors()) {
					if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(_node->data().get())) {
						out.push_back(&_node->data());
					}
				}
				return out;
			}

			/**
			 * Returns a typed list of agents that are out neighbors of the current
			 * agent on the given layer.
			 *
			 * Agents are added to the list if and only if :
			 * 1. they are contained in a node that is an out neighbor of this
			 * agent's node, connected on the specified layer
			 * 2. they can be cast to `NeighborAgentType`
			 *
			 * @return out neighbor agents
			 *
			 * @see api::graph::Node::outNeighbors()
			 */
			template<typename NeighborAgentType> Neighbors<NeighborAgentType> outNeighbors(api::graph::LayerId layer) const {
				std::vector<Neighbor<NeighborAgentType>> out;
				for(api::model::AgentNode* _node : node()->outNeighbors(layer)) {
					if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(_node->data().get())) {
						out.push_back(&_node->data());
					}
				}
				return out;
			}


			/**
			 * Returns a typed list of agents that are in neighbors of the current
			 * agent.
			 *
			 * Agents are added to the list if and only if :
			 * 1. they are contained in a node that is an in neighbor of this
			 * agent's node
			 * 2. they can be cast to `NeighborAgentType`
			 *
			 * @return in neighbor agents
			 *
			 * @see api::graph::Node::inNeighbors()
			 */
			template<typename NeighborAgentType> Neighbors<NeighborAgentType> inNeighbors() const {
				std::vector<Neighbor<NeighborAgentType>> in;
				for(api::model::AgentNode* _node : node()->inNeighbors()) {
					if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(_node->data().get())) {
						in.push_back(&_node->data());
					}
				}
				return in;
			}

			/**
			 * Returns a typed list of agents that are in neighbors of the current
			 * agent on the given layer.
			 *
			 * Agents are added to the list if and only if :
			 * 1. they are contained in a node that is an in neighbor of this
			 * agent's node, connected on the specified layer
			 * 2. they can be cast to `NeighborAgentType`
			 *
			 * @return in neighbor agents
			 *
			 * @see api::graph::Node::inNeighbors()
			 */
			template<typename NeighborAgentType> Neighbors<NeighborAgentType> inNeighbors(api::graph::LayerId layer) const {
				std::vector<Neighbor<NeighborAgentType>> in;
				for(api::model::AgentNode* _node : node()->inNeighbors(layer)) {
					if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(_node->data().get())) {
						in.push_back(&_node->data());
					}
				}
				return in;
			}
			virtual ~AgentBase() {}
	};
	/**
	 * Agent's TypeId.
	 *
	 * Equal to `typeid(AgentType)`.
	 */
	template<typename AgentType>
		const api::model::TypeId AgentBase<AgentType>::TYPE_ID = typeid(AgentType);

	/**
	 * Callback specialization that can be extended to define user callbacks.
	 */
	typedef api::utils::Callback<AgentNode*> AgentNodeCallback;
	
	/**
	 * An api::graph::NodeBuilder implementation that can be used to easily
	 * generate a random model, that can be provided to a
	 * api::graph::GraphBuilder implementation.
	 *
	 * AgentNodes are built automatically, and can be linked according to the
	 * api::graph::GraphBuilder algorithm implementation.
	 */
	class AgentNodeBuilder : public api::graph::NodeBuilder<AgentPtr> {
		private:
			api::model::AgentGroup& group;
			std::vector<api::model::Agent*> agents;

		public:
			/**
			 * AgentNodeBuilder constructor.
			 *
			 * @param group group to which agents will be added
			 */
			AgentNodeBuilder(api::model::AgentGroup& group)
				: group(group) {}

			/**
			 * Pushes an agent into the node builder, that will be inserted into
			 * the graph with the buildNode() function.
			 *
			 * @param agent agent to add to the graph
			 */
			void push(api::model::Agent* agent) {
				agents.push_back(agent);
			}

			/**
			 * Adds the agent to the specified group, using the
			 * api::model::AgentGroup::add() method.
			 *
			 * In consequence, a node is automatically built and added to the
			 * group's underlying graph.
			 *
			 * Such node can be linked automatically as specified by the
			 * api::graph::GraphBuilder algorithm implementation.
			 */
			api::graph::DistributedNode<AgentPtr>*
				buildNode(api::graph::DistributedGraph<AgentPtr>&) override {
					auto* agent = agents.back();
					group.add(agent);
					agents.pop_back();

					return agent->node();
				}

			/**
			 * Returns the count of agents left in the AgentNodeBuilder.
			 *
			 * @return agent count
			 */
			std::size_t nodeCount() override {
				return agents.size();
			}
	};

	/**
	 * FPMAS default Model.
	 */
	template<template<typename> class SyncMode>
		using Model = detail::DefaultModel<SyncMode>;

	using api::model::AgentGroup;
}}
#endif
