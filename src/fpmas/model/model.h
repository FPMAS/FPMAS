#ifndef FPMAS_MODEL_H
#define FPMAS_MODEL_H

/** \file src/fpmas/model/model.h
 * Model implementation.
 */

#include "fpmas/api/graph/graph_builder.h"
#include "guards.h"
#include "detail/model.h"
#include "fpmas/random/random.h"
#include "fpmas/io/breakpoint.h"
#include "fpmas/utils/format.h"
#include <fstream>


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
		class Neighbor  {
			private:
				AgentPtr* _agent;
				AgentEdge* _edge;

			public:
				/**
				 * Initializes a null Neighbor.
				 *
				 * The Neighbor can eventually be assigned later with a
				 * non-null Neighbor.
				 *
				 * @par Example
				 * ```cpp
				 * Neighbor neighbor;
				 *
				 * neighbor = agent->outNeighbors<...>().random();
				 * ```
				 */
				Neighbor() : Neighbor(nullptr, nullptr) {}

				/**
				 * @deprecated
				 *
				 * Neighbor constructor.
				 *
				 * @param agent pointer to a generic AgentPtr
				 */
				[[deprecated]]
				Neighbor(AgentPtr* agent)
					: Neighbor(agent, nullptr) {}

				/**
				 * Neighbor constructor.
				 *
				 * @param agent pointer to a generic AgentPtr
				 * @param edge edge with wich the neighbor is linked to agent
				 */
				Neighbor(AgentPtr* agent, AgentEdge* edge)
					: _agent(agent), _edge(edge) {}

				/**
				 * Implicit conversion operator to `AgentType*`.
				 */
				operator AgentType*() const {
					return dynamic_cast<AgentType*>(_agent->get());
				}

				/**
				 * Member of pointer access operator.
				 *
				 * @return pointer to neighbor agent
				 */
				AgentType* operator->() const {
					return dynamic_cast<AgentType*>(_agent->get());
				}

				/**
				 * Indirection operator.
				 *
				 * @return reference to neighbor agent
				 */
				AgentType& operator*() const {
					return *dynamic_cast<AgentType*>(_agent->get()); }

				/**
				 * Returns the edge with which this neighbor is linked to
				 * agent.
				 *
				 * @return neighbor edge
				 */
				AgentEdge* edge() const {
					return _edge;
				}

				/**
				 * Returns a pointer to the internal Agent, or a `nullptr` if
				 * the Neighbor is null.
				 *
				 * @return pointer to internal agent
				 */
				AgentType* agent() const {
					if(_agent == nullptr)
						return nullptr;
					return dynamic_cast<AgentType*>(_agent->get());
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
				static random::DistributedGenerator<> rd;

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

				/**
				 * Filters the neighbor list.
				 *
				 * The specified `Filter` must be an unary predicate such that
				 * for each `agent` in the current neighbor list, `agent` is
				 * **kept** only if `_filter(agent)` returns `true`.
				 *
				 * @param _filter filter to apply
				 * @return filtered list
				 */
				template<typename Filter>
				Neighbors& filter(Filter _filter) {
					neighbors.erase(
							std::remove_if(
								neighbors.begin(), neighbors.end(),
								[&_filter] (const Neighbor<AgentType>& agent) {return !_filter(agent);}),
							neighbors.end()
							);
					return *this;
				}

				/**
				 * Returns a random element of this neighbors list.
				 *
				 * @return random neighbor
				 */
				Neighbor<AgentType> random() {
					random::UniformIntDistribution<std::size_t> index(0, this->count()-1);
					return neighbors[index(rd)];
				}
		};
	template<typename AgentType> random::DistributedGenerator<> Neighbors<AgentType>::rd;

	/**
	 * api::model::Behavior implementation.
	 *
	 * This Behavior can be defined from any sequence of member methods of
	 * AgentType.
	 */
	template<typename AgentType>
		class Behavior : public api::model::Behavior {
			private:
				std::vector<void(AgentType::*)()> _behaviors;

			public:
				/**
				 * Behavior constructor.
				 *
				 * Builds an Agent behavior from the member methods specified
				 * as arguments. Those methods must take no argument, and will
				 * be called in the order specified on each Agent to which this
				 * behavior is applied.
				 *
				 * Each `agent` to which this behavior is applied must be of type
				 * AgentType, or be a child class of AgentType, so that
				 * specified methods can be called on `agent`. Otherwise, the
				 * behavior of execute() is undefined.
				 *
				 * Notice that AgentType might be an abstract class, and
				 * specified methods can be virtual. In such a case, this
				 * Behavior can be applied to any AgentType implementation.
				 *
				 * @par example
				 *
				 * ```cpp
				 * class Agent : public fpmas::model::AgentBase<Agent> {
				 *     public:
				 *         void behavior1() {};
				 *
				 *         void behavior2() {};
				 * };
				 *
				 * int main() {
				 *     ...
				 *     fpmas::model::Model model;
				 *
				 *     // behavior1() and behavior2() methods will be called
				 *     // successively on Agents to which this behavior is
				 *     // applied.
				 *     fpmas::model::Behavior<Agent> behavior {
				 *         &Agent::behavior1, &Agent::behavior2
				 *     };
				 *
				 *     // behavior will be applied to any agent of this group
				 *     auto& group = model.buildGroup(AGENT_GROUP_ID, behavior);
				 *
				 *     // Agents added to the group must be convertible to
				 *     // Agent.
				 *     group.add(new Agent);
				 * }
				 * ```
				 *
				 * @param behaviors A comma separated list of AgentType member methods,
				 * in the form `&AgentType::method`
				 */
				template<typename ...T>
					Behavior(T... behaviors)
					: _behaviors({behaviors...}){
					}

				void execute(api::model::Agent* agent) const {
					for(auto behavior : _behaviors)
						(dynamic_cast<AgentType*>(agent)->*behavior)();
				}
		};

	/**
	 * An api::model::Behavior implementation that can take some arbitrary
	 * arguments.
	 *
	 * The arguments are provided in the constructor, and are passed to the
	 * specified behavior each time execute() is called. Using pointers or
	 * references can be used as arguments if dynamic parameter update is
	 * required.
	 */
	template<typename AgentType, typename ... Args>
		class BehaviorWithArgs : public api::model::Behavior {
			private:
				void (AgentType::* behavior)(Args...);
				std::tuple<Args...> args;

				// helpers for tuple unrolling
				// From: https://riptutorial.com/cplusplus/example/24746/storing-function-arguments-in-std--tuple
				template<int ...> struct seq {};
				template<int N, int ...S> struct gens {
					public:
						typedef typename gens<N-1, N-1, S...>::type type;
				};
				template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };

				// invocation helper 
				template<int ...S>
					void call_fn_internal(api::model::Agent* agent, const seq<S...>) const
					{
						(dynamic_cast<AgentType*>(agent)->*behavior)(std::get<S>(args) ...);
					}

			public:
				/**
				 * BehaviorWithArgs constructor.
				 *
				 * @param behavior The AgentType method that will be called by
				 * execute() with the specified arguments
				 * @param args argument list that will be passed to `behavior`
				 */
				BehaviorWithArgs(
						void (AgentType::* behavior)(Args...),
						Args... args)
					: behavior(behavior), args(args...) {}

				/**
				 * Calls `behavior` on the specified `agent` with the arguments
				 * specified in the constructor.
				 *
				 * @param agent agent to execute
				 */
				void execute(api::model::Agent* agent) const override {
					return call_fn_internal(agent, typename gens<sizeof...(Args)>::type());
				}
		};

	/**
	 * Defines useful templates to obtain typed neighbors list directly from an
	 * api::model::Agent.
	 */
	class NeighborsAccess : public virtual api::model::Agent {
		public:
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
			for(api::model::AgentEdge* _edge : node()->getOutgoingEdges()) {
				api::model::AgentNode* _node = _edge->getTargetNode();
				if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(_node->data().get())) {
					out.push_back({&_node->data(), _edge});
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
			for(api::model::AgentEdge* _edge : node()->getOutgoingEdges(layer)) {
				api::model::AgentNode* _node = _edge->getTargetNode();
				if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(_node->data().get())) {
					out.push_back({&_node->data(), _edge});
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
			for(api::model::AgentEdge* _edge : node()->getIncomingEdges()) {
				api::model::AgentNode* _node = _edge->getSourceNode();
				if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(_node->data().get())) {
					in.push_back({&_node->data(), _edge});
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
			for(api::model::AgentEdge* _edge : node()->getIncomingEdges(layer)) {
				api::model::AgentNode* _node = _edge->getSourceNode();
				if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(_node->data().get())) {
					in.push_back({&_node->data(), _edge});
				}
			}
			return in;
		}

	};

	/**
	 * Base implementation of the \Agent API.
	 *
	 * Users can use this class to easily define their own \Agents with custom
	 * behaviors.
	 */
	template<typename AgentType, typename TypeIdBase = AgentType>
	class AgentBase : public virtual api::model::Agent, public NeighborsAccess {
		public:
			static const api::model::TypeId TYPE_ID;
			
		private:
			std::vector<api::model::GroupId> group_ids;
			std::vector<api::model::AgentGroup*> _groups;
			std::unordered_map<api::model::GroupId, api::model::AgentTask*> _tasks;
			api::model::AgentNode* _node;
			api::model::Model* _model;

		public:
			/**
			 * \copydoc fpmas::api::model::Agent::groupId
			 */
			GroupId groupId() const override {
				if(group_ids.size() > 0)
					return group_ids.back();
				return {};
			}
			/**
			 * \copydoc fpmas::api::model::Agent::groupIds
			 */
			std::vector<GroupId> groupIds() const override {return group_ids;}
			/**
			 * \copydoc fpmas::api::model::Agent::setGroupId
			 */
			void setGroupId(api::model::GroupId id) override {
				group_ids.push_back(id);}
			/**
			 * \copydoc fpmas::api::model::Agent::addGroupId
			 */
			void addGroupId(api::model::GroupId id) override {
				group_ids.push_back(id);
			}

			/**
			 * \copydoc fpmas::api::model::Agent::removeGroupId
			 */
			void removeGroupId(api::model::GroupId id) override {
				group_ids.erase(std::remove(group_ids.begin(), group_ids.end(), id));
			}

			/**
			 * \copydoc fpmas::api::model::Agent::group
			 */
			api::model::AgentGroup* group() override {
				if(_groups.size() > 0)
					return _groups.back();
				return nullptr;
			}
			/**
			 * \copydoc fpmas::api::model::Agent::groups
			 */
			std::vector<const api::model::AgentGroup*> groups() const override {
				return {_groups.begin(), _groups.end()};
			}

			/**
			 * \copydoc fpmas::api::model::Agent::group
			 */
			const api::model::AgentGroup* group() const override {return _groups.back();}
			/**
			 * \copydoc fpmas::api::model::Agent::groups
			 */
			std::vector<api::model::AgentGroup*> groups() override {return _groups;}

			/**
			 * \copydoc fpmas::api::model::Agent::setGroup
			 */
			void setGroup(api::model::AgentGroup* group) override {
				_groups.push_back(group);
			}
			/**
			 * \copydoc fpmas::api::model::Agent::addGroup
			 */
			void addGroup(api::model::AgentGroup* group) override {
				_groups.push_back(group);
			}

			/**
			 * \copydoc fpmas::api::model::Agent::removeGroup
			 */
			void removeGroup(api::model::AgentGroup* group) override {
				_groups.erase(std::remove(_groups.begin(), _groups.end(), group));
				_tasks.erase(group->groupId());
			}

			/**
			 * \copydoc fpmas::api::model::Agent::typeId
			 */
			api::model::TypeId typeId() const override {return TYPE_ID;}
			/**
			 * \copydoc fpmas::api::model::Agent::copy
			 */
			api::model::Agent* copy() const override {
				return new AgentType(*dynamic_cast<const AgentType*>(this));
			}

			/**
			 * \copydoc fpmas::api::model::Agent::copyAssign
			 */
			void copyAssign(api::model::Agent* agent) override {
				// Uses AgentType copy assignment operator
				*dynamic_cast<AgentType*>(this) = *dynamic_cast<const AgentType*>(agent);
			}

			/**
			 * \copydoc fpmas::api::model::Agent::moveAssign
			 */
			void moveAssign(api::model::Agent* agent) override {
				// TODO: this should probably be improved in 2.0
				// The purpose of "moveAssign" is clearly inconsistent with its
				// current behavior, that does much more that just "moving" the
				// agent.

				// Sets and overrides the fields that must be preserved
				std::set<api::model::GroupId> local_ids;
				for(auto id : this->groupIds())
					local_ids.insert(id);
				std::set<api::model::GroupId> updated_ids;
				for(auto id : agent->groupIds())
					updated_ids.insert(id);

				std::vector<api::model::GroupId> new_groups;
				for(auto id : updated_ids)
					if(local_ids.count(id) == 0)
						new_groups.push_back(id);

				std::vector<api::model::GroupId> obsolete_groups;
				for(auto id : local_ids)
					if(updated_ids.count(id) == 0)
						obsolete_groups.push_back(id);

				// Preserve the local agent set up
				for(auto group : this->groups())
					agent->addGroup(group);
				// Ok because the updated list of ids is saves in updated_ids
				for(auto id : agent->groupIds())
					agent->removeGroupId(id);
				for(auto id : this->groupIds())
					agent->addGroupId(id);

				for(auto task : this->tasks())
					agent->setTask(task.first, task.second);
				agent->setNode(this->node());
				agent->setModel(this->model());

				// Uses AgentType move assignment operator
				//
				// groupIds(), groups(), tasks(), node() and model() are
				// notably moved from agent to this, but this as no effect
				// since agent's fields was overriden above. In consequence,
				// those fields are properly preserved, as required by the
				// method.
				*dynamic_cast<AgentType*>(this) = std::move(*dynamic_cast<AgentType*>(agent));

				// Dynamically updates groups lists
				// If `agent` was added to / removed from group on a distant
				// process for example (assuming that this agent is DISTANT),
				// this agent representation groups are updated.
				for(auto id : new_groups)
					this->model()->getGroup(id).add(this);
				for(auto id : obsolete_groups)
					this->model()->getGroup(id).remove(this);
			}

			/**
			 * \copydoc fpmas::api::model::Agent::node
			 */
			api::model::AgentNode* node() override {return _node;}
			/**
			 * \copydoc fpmas::api::model::Agent::node
			 */
			const api::model::AgentNode* node() const override {return _node;}
			/**
			 * \copydoc fpmas::api::model::Agent::setNode
			 */
			void setNode(api::model::AgentNode* node) override {_node = node;}

			/**
			 * \copydoc fpmas::api::model::Agent::model
			 */
			api::model::Model* model() override {return _model;}
			/**
			 * \copydoc fpmas::api::model::Agent::model
			 */
			const api::model::Model* model() const override {return _model;}
			/**
			 * \copydoc fpmas::api::model::Agent::setModel
			 */
			void setModel(api::model::Model* model) override {_model = model;}

			/**
			 * \copydoc fpmas::api::model::Agent::task()
			 */
			api::model::AgentTask* task() override {
				return _tasks.at(this->groupId());
			}
			/**
			 * \copydoc fpmas::api::model::Agent::task()
			 */
			const api::model::AgentTask* task() const override {
				return _tasks.at(this->groupId());
			}
			/**
			 * \copydoc fpmas::api::model::Agent::setTask(api::model::AgentTask*)
			 */
			void setTask(api::model::AgentTask* task) override {
				_tasks[this->groupId()] = task;
			}

			/**
			 * \copydoc fpmas::api::model::Agent::task(GroupId)
			 */
			api::model::AgentTask* task(api::model::GroupId id) override {
				return _tasks.at(id);
			}
			/**
			 * \copydoc fpmas::api::model::Agent::task(GroupId)
			 */
			const api::model::AgentTask* task(api::model::GroupId id) const override {
				return _tasks.at(id);}

			/**
			 * \copydoc fpmas::api::model::Agent::setTask(GroupId, api::model::AgentTask*)
			 */
			void setTask(api::model::GroupId id, api::model::AgentTask* task) override {
				_tasks[id] = task;
			}

			/**
			 * \copydoc fpmas::api::model::Agent::tasks
			 */
			const std::unordered_map<api::model::GroupId, api::model::AgentTask*>&
				tasks() override { return _tasks;}

			
			/**
			 * By default, no behavior is associated to act().
			 *
			 * Implemented Agents can override the act() method to implement a
			 * default behavior.
			 */
			virtual void act() override {}

			virtual ~AgentBase() {}
	};
	/**
	 * Agent's TypeId.
	 *
	 * Equal to `typeid(AgentType)`.
	 */
	template<typename AgentType, typename TypeIdBase>
		const api::model::TypeId AgentBase<AgentType, TypeIdBase>::TYPE_ID = typeid(TypeIdBase);

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

	/**
	 * \Model fpmas::io::Breakpoint specialization.
	 */
	typedef io::Breakpoint<api::model::Model> Breakpoint;

	/**
	 * An utility class that can be used to easily schedule \Model breakpoints.
	 *
	 * A job() is built to automatically dump the specified model to a file
	 * when it is executed.
	 */
	class AutoBreakpoint {
		private:
			std::string file_format;
			std::fstream stream;
			fpmas::api::io::Breakpoint<api::model::Model>& breakpoint;
			api::model::Model& model;
			scheduler::detail::LambdaTask task {[this] () {
				stream.open(utils::format(
							file_format,
							model.getMpiCommunicator().getRank(),
							model.runtime().currentDate()
							), std::ios_base::out);
				breakpoint.dump(stream, model);
				stream.close();
			}};
			scheduler::Job _job {{task}};

		public:
			/**
			 * AutoBreakpoint constructor.
			 *
			 * The specified `file_format` is formatted using the
			 * fpmas::utils::format(std::string, int, fpmas::api::scheduler::TimeStep)
			 * method, so that `%r` and `%t` occurrences in the `file_format`
			 * are automatically replaced by the current process rank and the
			 * current date to generate file names.
			 *
			 * Since breakpoints can quickly grow in size, a good practice is
			 * to use a `file_format` such as `"breakpoint.%r.bin"`, that does
			 * not depend on `"%t"`. This way, the last dump always overrides the
			 * previous, what might save a consequent amount of memory, while
			 * still allowing to recover from the most recent breakpoint in
			 * case of crash.
			 *
			 * @param file_format File name format
			 * @param breakpoint Breakpoint instance used to perform dumps
			 * @param model Model on which breakpoints are performed
			 */
			AutoBreakpoint(
					std::string file_format,
					api::io::Breakpoint<api::model::Model>& breakpoint,
					api::model::Model& model)
				: file_format(file_format), breakpoint(breakpoint), model(model) {
				}

			/**
			 * A \Job that can be scheduled to automatically dump model breakpoints.
			 *
			 * @see fpmas::api::scheduler::Scheduler
			 */
			const api::scheduler::Job& job() const {return _job;}
	};
}}

namespace nlohmann {
	/**
	 * fpmas::api::model::AgentPtr JSON serialization rules **declaration**.
	 *
	 * `to_json` and `from_json` methods must be _defined_ by the user at compile time,
	 * depending on its own Agent types, using the FPMAS_JSON_SET_UP() macro,
	 * that must be called from a **source** file. Otherwise, linker errors
	 * will be thrown.
	 */
	template<>
		struct adl_serializer<fpmas::api::model::AgentPtr> {
			/**
			 * Serializes the \Agent represented by `pointer` to the specified
			 * JSON `j`.
			 *
			 * Notice that since \Agent is polymorphic, the actual `to_json`
			 * call that corresponds to the most derived type of the specified
			 * \Agent is resolved at runtime.
			 *
			 * @param j json output
			 * @param pointer pointer to polymorphic \Agent
			 */
			static void to_json(json& j, const fpmas::api::model::AgentPtr& pointer);

			/**
			 * Unserializes an \Agent from the specified JSON `j`.
			 *
			 * The built \Agent is dynamically allocated, but is automatically
			 * managed by the fpmas::api::model::AgentPtr wrapper.
			 *
			 * Since \Agent is polymorphic, the concrete type that should be
			 * instantiated from the input JSON `j` is determined at runtime.
			 *
			 * @param j input json
			 * @return dynamically allocated \Agent, unserialized from `j`,
			 * wrapped in an fpmas::api::model::AgentPtr instance
			 */
			static fpmas::api::model::AgentPtr from_json(const json& j);
		};

	/**
	 * \Model JSON serialization rules.
	 */
	template<>
		struct adl_serializer<fpmas::api::model::Model> {
			/**
			 * Serializes the specified `model` to the json `j`.
			 *
			 * The complete underlying \DistributedGraph is dumped in the json, as long as
			 * the current \Runtime status.
			 *
			 * @param j json output
			 * @param model \Model to serialize
			 *
			 * \see \ref nlohmann_adl_serializer_DistributedGraph_to_json "nlohmann::adl_serializer<fpmas::api::graph::DistributedGraph<T>>::to_json()"
			 */
			static void to_json(json& j, const fpmas::api::model::Model& model);

			/**
			 * Loads data from the input json to the specified `model`.
			 *
			 * The \DistributedGraph contained in `j` is loaded into
			 * `model.graph()`, and the `model.runtime()` current date is set
			 * to the value saved in the json.
			 *
			 * @param j input json
			 * @param model \Model in which data from `j` will be loaded
			 *
			 * \see \ref nlohmann_adl_serializer_DistributedGraph_from_json "nlohmann::adl_serializer<fpmas::api::graph::DistributedGraph<T>>::from_json()"
			 */
			static void from_json(const json& j, fpmas::api::model::Model& model);
		};
}

namespace fpmas { namespace io { namespace json {
	template<>
		struct light_serializer<fpmas::api::model::AgentPtr> {
			static void to_json(nlohmann::json& j, const fpmas::api::model::AgentPtr& pointer);
			static fpmas::api::model::AgentPtr from_json(const nlohmann::json& j);
		};
}}}
#endif
