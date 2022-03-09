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
#include "fpmas/graph/graph_builder.h"
#include "fpmas/graph/random_load_balancing.h"
#include <fstream>


namespace fpmas { namespace model {

	/**
	 * Checks if `agent` is currently in the AgentGroup represented by
	 * `group_id`.
	 */
	bool is_agent_in_group(api::model::Agent* agent, api::model::GroupId group_id);

	/**
	 * Checks if `agent` is currently in `group`.
	 */
	bool is_agent_in_group(api::model::Agent* agent, api::model::AgentGroup& group);

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
				HEDLEY_DEPRECATED_FOR(1.1, Neighbor(AgentPtr*, AgentEdge*))
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
					return static_cast<AgentType*>(_agent->get());
				}

				/**
				 * Member of pointer access operator.
				 *
				 * @return pointer to neighbor agent
				 */
				AgentType* operator->() const {
					return static_cast<AgentType*>(_agent->get());
				}

				/**
				 * Indirection operator.
				 *
				 * @return reference to neighbor agent
				 */
				AgentType& operator*() const {
					return *static_cast<AgentType*>(_agent->get()); }

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
					return static_cast<AgentType*>(_agent->get());
				}
		};

	/**
	 * Default Neighbor comparison function object.
	 *
	 * @tparam AgentType Type of neighbor agents
	 * @tparam Compare a comparator that can compare two `AgentType` instance.
	 * The comparator must defined a method with the following signature:
	 * ```cpp
	 * bool operator()(const AgentType&, const AgentType&) const;
	 * ```
	 * By default, `AgentTypes` are compared using the `operator<`, that can be
	 * user defined.
	 */
	template<typename AgentType, typename Compare = std::less<AgentType>>
		class CompareNeighbors {
			private:
				Compare compare;
			public:
				CompareNeighbors() = default;

				/**
				 * Compares agents using the specified function object.
				 */
				CompareNeighbors(const Compare& compare) : compare(compare) {
				}

				/**
				 * Compares neighbors' agents using the a Compare instance.
				 */
				bool operator()(
						const Neighbor<AgentType>& n1,
						const Neighbor<AgentType>& n2) const {
					return compare(*n1.agent(), *n2.agent());
				}
		};

	/**
	 * Default Neighbor comparison operator.
	 *
	 * This can only compare agents using the `operator<`, contrary to the
	 * generic CompareNeighbors implementation.
	 *
	 * However, this definition might be useful to use external sorting
	 * algorithm or any other method requiring a Neighbor comparison using the
	 * `operator<`.
	 *
	 * @par Example
	 * From a \SpatialAgent behavior:
	 * ```cpp
	 * auto mobility_field = this->mobilityField();
	 * std::sort(mobility_field.begin(), mobility_field.end());
	 * ```
	 * This is just for the example purpose, `mobility_field.sort()` can be
	 * used directly to produce the same behavior.
	 */
	template<typename AgentType>
		bool operator<(const Neighbor<AgentType>& n1, const Neighbor<AgentType>& n2) {
			return *n1.agent() < *n2.agent();
		}

	/**
	 * Helper class that defines a static random::DistributedGenerator used by
	 * the Neighbors class (for any `AgentType`).
	 *
	 * The random generator can be seeded by the user to control random
	 * neighbors selection and shuffling.
	 *
	 * @see Neighbors::shuffle()
	 * @see Neighbors::random()
	 */
	struct RandomNeighbors {
		/**
		 * Static random generator.
		 */
		static random::DistributedGenerator<> rd;

		/**
		 * Seeds the random generator.
		 *
		 * @param seed random seed
		 */
		static void seed(random::DistributedGenerator<>::result_type seed);
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

			public:
				Neighbors() = default;

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
				 * Checks if this neighbors list is empty.
				 *
				 * @return true iff there is no neighbor in this neighbor list
				 */
				bool empty() const {
					return neighbors.empty();
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
				 * [std::shuffle](https://en.cppreference.com/w/cpp/algorithm/random_shuffle)
				 * and the provided random number generator.
				 *
				 * Can be used to iterate randomly over the neighbors set.
				 *
				 * @return reference to this Neighbors instance
				 */
				template<typename Gen>
					Neighbors& shuffle(Gen& gen) {
						std::shuffle(neighbors.begin(), neighbors.end(), gen);
						return *this;
					}

				/**
				 * Shuffles agent using the distributed RandomNeighbors::rd
				 * random number generator.
				 *
				 * @see shuffle(Gen&)
				 *
				 * @return reference to this Neighbors instance
				 */
				Neighbors& shuffle() {
					return shuffle(RandomNeighbors::rd);
				}

				/**
				 * Sorts the neighbor list using the specified comparator.
				 *
				 * By default, the comparison is performed on neighbors agents
				 * using the `operator<` on `AgentType`, that can be user
				 * defined.
				 *
				 * In consequence, neighbors are sorted is ascending order by
				 * default. A custom comparator based on the `operator>` can be
				 * specified to sort neighbros in descending order.
				 *
				 * En efficient way of defining other comparison functions is
				 * the usage of lambda functions:
				 * ```cpp
				 * void agent_behavior() {
				 * 	auto neighbors = this->outNeighbors<UserAgent>();
				 * 	neighbors.sort([] (const UserAgent& a1, const UserAgent& a2) {
				 * 		return a1.getField() < a2.getField();
				 * 	});
				 * }
				 * ```
				 *
				 * More generally, `Compare` must be a type that defines a
				 * method with the following signature:
				 * ```
				 * bool operator()(const AgentType& a1, const AgentType& a2) const;
				 * ```
				 *
				 * @tparam Compare comparator type
				 * @param comp comparator instance
				 */
				template<typename Compare = std::less<AgentType>>
					Neighbors& sort(Compare comp = Compare()) {
						std::sort(
								neighbors.begin(), neighbors.end(),
								CompareNeighbors<AgentType, Compare>(comp)
								);
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
				 * Selects a random element of this neighbors list, using the
				 * specified random number generator.
				 *
				 * @return random neighbor
				 */
				template<typename Gen>
					Neighbor<AgentType> random(Gen& gen) {
						random::UniformIntDistribution<std::size_t> index(
								0, this->count()-1);
						return neighbors.at(index(gen));
					}

				/**
				 * Selects a ramdom element of this neighbors list, using the
				 * distributed RandomNeighbors::rd random number generator.
				 *
				 * @see random(Gen&)
				 *
				 * @return random neighbor
				 */
				Neighbor<AgentType> random() {
					return random(RandomNeighbors::rd);
				}
		};

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

				template<typename T, typename Enable=void>
					struct AutoAgentCast {
						static T* cast(api::model::Agent* agent) {
							return dynamic_cast<T*>(agent);
						}
					};

				template<typename T>
					struct AutoAgentCast<T, typename std::enable_if<
					std::is_base_of<api::model::Agent, T>::value
					>::type> {
						static T* cast(api::model::Agent* agent) {
							return static_cast<T*>(agent);
						}
					};

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
					for(auto behavior : this->_behaviors)
						(AutoAgentCast<AgentType>::cast(agent)->*behavior)();
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
	 * A \Behavior that does not execute anything on any \Agent.
	 */
	class IdleBehavior : public api::model::Behavior {
		void execute(api::model::Agent*) const override {
		}
	};

	namespace detail {
	/**
	 * Base implementation of the \Agent API.
	 *
	 * Users can use this class to easily define their own \Agents with custom
	 * behaviors.
	 *
	 * The `AgentInterface` template parameter specifies the API actually
	 * implemented by this AgentBase. It can be api::model::Agent, or any other
	 * api that extends the api::model::Agent interface.
	 *
	 * ```
	 *                               +--------------------------+
	 *                    ---------->| fpmas::api::model::Agent |<--------
	 *                   |           +--------------------------+         |
	 *                   |                                                |
	 *                   |                                                |
	 * +---------------------------------+                   +-------------------------+
	 * | fpmas::api::model::SpatialAgent |                   | fpmas::api::model::Cell |
	 * +---------------------------------+                   +-------------------------+
	 *           ^                                                        ^
	 *           |                                                        |
	 *  +------------------------------+                   +-----------------------------+
	 *  | fpmas::api::model::GridAgent |                   | fpmas::api::model::GridCell |
	 *  +------------------------------+                   +-----------------------------+
	 * ```
	 *
	 * - Implementing a regular Agent `UserAgent` will produce the following
	 *   hierarchy. In this case, `AgentInterface=api::model::Agent`.
	 *   ```
	 *               +--------------------------+
	 *               | fpmas::api::model::Agent |
	 *               +--------------------------+
	 *                             ^
	 *                             |
	 *   +---------------------------------------------------------+
	 *   | model::detail::AgentBase<fpmas::api::model::Agent, ...> | (alias: model::AgentBase<...>)
	 *   +---------------------------------------------------------+
	 *                             ^
	 *                             |
	 *                      +-----------+
	 *                      | UserAgent |
	 *                      +-----------+
	 *   ```
	 * - Implementing a GridAgent UserAgent produces the following
	 *   hierarchy. In this case, `AgentInterface=api::model::GridAgent`.
	 *   ```
	 *               +--------------------------+
	 *               | fpmas::api::model::Agent |
	 *               +--------------------------+
	 *                             ^
	 *                             |
	 *             +---------------------------------+
	 *             | fpmas::api::model::SpatialAgent |
	 *             +---------------------------------+
	 *                             ^
	 *                             |
	 *              +------------------------------+
	 *              | fpmas::api::model::GridAgent |
	 *              +------------------------------+
	 *                             ^
	 *                             |
	 *   +-------------------------------------------------------------+
	 *   | model::detail::AgentBase<fpmas::api::model::GridAgent, ...> |
	 *   +-------------------------------------------------------------+
	 *                             ^
	 *                             |
	 *   +-------------------------------------------------------------------+
	 *   | fpmas::model::SpatialAgentBase<fpmas::api::model::GridAgent, ...> |
	 *   +-------------------------------------------------------------------+
	 *                             ^
	 *                             |
	 *              +------------------------------+
	 *              | fpmas::model::GridAgent<...> |
	 *              +------------------------------+
	 *                             ^
	 *                             |
	 *                       +-----------+
	 *                       | UserAgent |
	 *                       +-----------+
	 *   ```
	 *
	 * In previous FPMAS versions, various Agent implementations was based on
	 * diamond inheritance and virtual base classes. However, such design
	 * revealed extremely costly in terms of `dynamic_cast` (downcasts using
	 * `static_cast` is not allowed with virtual bases). The new templated
	 * `AgentInterface` design produces vertical hierarchies that are much more
	 * efficient in case of `dynamic_cast`, and allow `static_cast`.
	 *
	 *
	 * @tparam AgentInterface api implemented by this AgentBase
	 * @tparam AgentType final Agent type, that must inherit from the current
	 * AgentBase.
	 * @tparam TypeIdBase type used to define the type id of the current
	 * AgentBase implementation.
	 */
	template<typename AgentInterface, typename AgentType, typename TypeIdBase = AgentType>
	class AgentBase : public AgentInterface {
		public:
			static const api::model::TypeId TYPE_ID;
			/**
			 * Type of the concrete, most derived \Agent type that extends this
			 * AgentBase.
			 *
			 * This type is the type actually instantiated and stored in a
			 * \DistributedNode for example.
			 */
			typedef AgentType FinalAgentType;
			
		private:
			std::vector<api::model::GroupId> group_ids;
			std::vector<api::model::AgentGroup*> _groups;
			std::unordered_map<
				api::model::GroupId, std::list<api::model::Agent*>::iterator
				> group_pos;
			std::unordered_map<api::model::GroupId, api::model::AgentTask*> _tasks;
			api::model::AgentNode* _node;
			api::model::Model* _model;

		public:
			/**
			 * Auto generated default constructor.
			 */
			AgentBase() = default;

			/**
			 * Auto generated copy assignment operator.
			 *
			 * If `AgentType` does not define any custom copy assignment
			 * operator, the `AgentType` copy assignment operator copies assign
			 * all `AgentType` members and all base classes (including this
			 * `AgentBase`) members.
			 *
			 * If a custom `AgentType` copy assignment operator is defined,
			 * this `AgentBase` copy assignment operator must be called
			 * explictly to ensure required members are properly copied.
			 *
			 * @see https://en.cppreference.com/w/cpp/language/copy_assignment
			 */
			AgentBase(const AgentBase& agent) = default;

			/**
			 * Auto generated copy constructor.
			 *
			 * If `AgentType` does not define any custom copy constructor , the
			 * `AgentType` copy constructor copies all `AgentType` members and
			 * all base classes (including this `AgentBase`) members using
			 * their copy constructors.
			 *
			 * If a custom `AgentType` copy constructor is defined, this
			 * `AgentBase` copy constructor must be called explictly to ensure
			 * required members are properly copied.
			 *
			 * @see https://en.cppreference.com/w/cpp/language/copy_constructor
			 */
			AgentBase& operator=(const AgentBase& agent) = default;

			// Move constructor and assignment preserve groupIds(), groups(),
			// group_pos, tasks(), node() and model()

			/**
			 * Autor generated move constructor.
			 *
			 * If `AgentType` does not define any custom move constructor , the
			 * `AgentType` move constructor moves all `AgentType` members and
			 * all base classes (including this `AgentBase`) members using
			 * their move constructors.
			 *
			 * If a custom `AgentType` move constructor is defined, this
			 * `AgentBase` copy constructor must be called explictly to ensure
			 * required members are properly copied.
			 *
			 * @see https://en.cppreference.com/w/cpp/language/move_constructor
			 */
			AgentBase(AgentBase&&) = default;

			/**
			 * Move assignment operator.
			 *
			 * groupIds(), groups(), getGroupPos() results, tasks(), node() and
			 * model() fields are preserved, as specified in the moveAssign()
			 * requirements, i.e. those fields are **not** moved from `other`
			 * to `this`.
			 *
			 * Since the current implementation does not do anything, there is
			 * no requirement to explicitly call this move constructor if a
			 * custom `AgentBase` move constructor is defined.
			 */
			AgentBase& operator=(AgentBase&&) {
				return *this;
			}

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
			 * \copydoc fpmas::api::model::Agent::setGroupPos
			 */
			void setGroupPos(
					api::model::GroupId gid,
					std::list<api::model::Agent*>::iterator pos
					) override {
				group_pos[gid] = pos;
			}

			/**
			 * \copydoc fpmas::api::model::Agent::getGroupPos
			 */
			std::list<api::model::Agent*>::iterator getGroupPos(api::model::GroupId gid) const override {
				return group_pos.find(gid)->second;
			}

			/**
			 * \copydoc fpmas::api::model::Agent::typeId
			 */
			api::model::TypeId typeId() const override {return TYPE_ID;}
			/**
			 * \copydoc fpmas::api::model::Agent::copy
			 */
			api::model::Agent* copy() const override {
				return new AgentType(*static_cast<const AgentType*>(this));
			}

			/**
			 * \copydoc fpmas::api::model::Agent::copyAssign
			 */
			void copyAssign(api::model::Agent* agent) override {
				// Uses AgentType copy assignment operator
				*static_cast<AgentType*>(this) = *static_cast<const AgentType*>(agent);
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

				// Uses AgentType move assignment operator
				//
				// groupIds(), groups(), tasks(), node() and model() are
				// notably moved from agent to this, but this as no effect
				// considering the move assignment operator defined above. In
				// consequence, those fields are properly preserved, as
				// required by the method.
				// If AgentType only explicitly defines a copy assignment
				// operator, it should not call the AgentBase copy assignment
				// so there should not be any problem.
				*static_cast<AgentType*>(this) = std::move(*static_cast<AgentType*>(agent));

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

			virtual ~AgentBase() {}
	};
	/**
	 * Agent's TypeId.
	 *
	 * Equal to `typeid(AgentType)`.
	 */
	template<typename AgentInterface, typename AgentType, typename TypeIdBase>
		const api::model::TypeId AgentBase<AgentInterface, AgentType, TypeIdBase>::TYPE_ID = typeid(TypeIdBase);
	}

	/**
	 * Basic AgentBase that can be extended by the user to define an Agent.
	 *
	 * @tparam AgentType final Agent type, that must inherit from the current
	 * AgentBase.
	 * @tparam TypeIdBase type used to define the type id of the current
	 * AgentBase implementation.
	 */
	template<typename AgentType, typename TypeIdBase = AgentType>
		using AgentBase = detail::AgentBase<api::model::Agent, AgentType, TypeIdBase>;

	/**
	 * Callback specialization that can be extended to define user callbacks.
	 */
	typedef api::utils::Callback<AgentNode*> AgentNodeCallback;
	
	/**
	 * An api::graph::NodeBuilder implementation that can be used to easily
	 * generate a random model, that can be provided to an
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
			api::model::AgentNode*
				buildNode(api::graph::DistributedGraph<AgentPtr>&) override;

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
	 * An api::graph::DistributedNodeBuilder implementation that can be used to
	 * easily generate a random model, that can be provided to an
	 * api::graph::DistributedGraphBuilder implementation.
	 *
	 * AgentNodes are built automatically, and can be linked according to the
	 * api::graph::DistributedGraphBuilder algorithm implementation.
	 */
	class DistributedAgentNodeBuilder : public graph::DistributedNodeBuilder<AgentPtr> {
		private:
			std::function<api::model::Agent*()> allocator;
			std::function<api::model::Agent*()> distant_allocator;
			api::model::AgentGroup& group;

		public:
			/**
			 * DistributedAgentNodeBuilder constructor.
			 *
			 * `allocator` is a function object used to dynamically allocate
			 * \Agents. When this version of the constructor is used, the
			 * `allocator` is used by both buildNode() and buildDistantNode()
			 * methods. A lambda function can be used to easily define an
			 * `allocator`.
			 *
			 * @par example
			 * ```cpp
			 * ...
			 * // Vector containing 1000 integers
			 * std::vector<int> some_agent_data(1000);
			 * // Initializes some_agent_data
			 * ...
			 * int index = 0;
			 * DistributedAgentNodeBuilder builder(
			 * 	user_group, 1000,
			 * 	[&index] () {
			 * 		return new UserAgent(some_agent_data[index++]);
			 * 		},
			 * 	model.getMpiCommunicator()
			 * 	);
			 * ```
			 * @note Notice that the build process defined in an
			 * fpmas::api::graph::DistributedGraphBuilder implementation is
			 * **distributed**. In consequence, the inialization of
			 * `some_agent_data` should in practice take distribution into
			 * account. For example, since 1000 is the **total** count of
			 * initialized \Agents, it is very likely that much less \Agents
			 * will be effectively initialized on the local process.
			 *
			 * @param group group to which agents will be added
			 * @param agent_count total count of \Agents to build
			 * @param allocator \Agent allocator
			 * @param comm MPI communicator
			 */
			DistributedAgentNodeBuilder(
					api::model::AgentGroup& group,
					std::size_t agent_count,
					std::function<api::model::Agent*()> allocator,
					api::communication::MpiCommunicator& comm);

			/**
			 * DistributedAgentNodeBuilder constructor.
			 *
			 * `allocator` and `distant_allocator` are function object used to
			 * dynamically allocate \Agents. When this version of the
			 * constructor is used, `allocator` is used by buildNode(), and
			 * `distant_allocator` is used by buildDistantNode(). Since
			 * buildDistantNode() is used to build temporary representations of
			 * \Agents, `distant_allocator` can be used to efficiently
			 * allocated default constructed \Agents. Lambda functions can be
			 * used to easily define an allocators.
			 *
			 * @par example
			 * ```cpp
			 * ...
			 * // Vector containing 1000 integers
			 * std::vector<int> some_agent_data(1000);
			 * // Initializes some_agent_data
			 * ...
			 * int index = 0;
			 * DistributedAgentNodeBuilder builder(
			 * 	user_group, 1000,
			 * 	[&index] () {
			 * 		// Allocates a local agent with data
			 * 		return new UserAgent(some_agent_data[index++]);
			 * 		},
			 * 	[] () {
			 * 		// Allocates a default temporary agent
			 * 		return new UserAgent;
			 * 		},
			 * 	model.getMpiCommunicator()
			 * 	);
			 * ```
			 * @note Same note as
			 * DistributedAgentNodeBuilder(fpmas::api::model::AgentGroup&, std::size_t, std::function<api::model::Agent*()>, fpmas::api::communication::MpiCommunicator&)
			 *
			 * @param group group to which agents will be added
			 * @param agent_count total count of \Agents to build
			 * @param allocator \Agent allocator
			 * @param distant_allocator temporary \Agent allocator
			 * @param comm MPI communicator
			 */
			DistributedAgentNodeBuilder(
					api::model::AgentGroup& group,
					std::size_t agent_count,
					std::function<api::model::Agent*()> allocator,
					std::function<api::model::Agent*()> distant_allocator,
					api::communication::MpiCommunicator& comm);


			/**
			 * \copydoc fpmas::api::graph::DistributedNodeBuilder::buildNode()
			 */
			api::model::AgentNode*
				buildNode(api::graph::DistributedGraph<AgentPtr>& graph) override;

			/**
			 * \copydoc fpmas::api::graph::DistributedNodeBuilder::buildDistantNode()
			 */
			api::model::AgentNode*
				buildDistantNode(
						api::graph::DistributedId id,
						int location,
						api::graph::DistributedGraph<AgentPtr>& graph) override;
	};

	/**
	 * FPMAS default Model.
	 */
	template<template<typename> class SyncMode>
		using Model = detail::DefaultModel<SyncMode>;

	using api::model::AgentGroup;

	/**
	 * graph::DistributedUniformGraphBuilder AgentPtr specialization.
	 */
	using DistributedUniformGraphBuilder
		= graph::DistributedUniformGraphBuilder<AgentPtr>;
	/**
	 * graph::DistributedClusteredGraphBuilder AgentPtr specialization.
	 */
	using DistributedClusteredGraphBuilder
		= graph::DistributedClusteredGraphBuilder<AgentPtr>;

	/**
	 * graph::RingGraphBuilder AgentPtr specialization.
	 */
	using RingGraphBuilder
		= graph::RingGraphBuilder<AgentPtr>;

	/**
	 * graph::ZoltanLoadBalancing AgentPtr specialization.
	 */
	using ZoltanLoadBalancing
		= graph::ZoltanLoadBalancing<AgentPtr>;
	/**
	 * graph::ScheduledLoadBalancing AgentPtr specialization.
	 */
	using ScheduledLoadBalancing
		= graph::ScheduledLoadBalancing<AgentPtr>;

	/**
	 * graph::RandomLoadBalancing AgentPtr specialization.
	 */
	using RandomLoadBalancing
		= graph::RandomLoadBalancing<AgentPtr>;

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

namespace fpmas { namespace io { namespace json {

	/**
	 * fpmas::api::model::AgentPtr fpmas::io::json::light_json serialization
	 * rules **declaration**.
	 *
	 * `to_json` and `from_json` methods must be _defined_ by the user at
	 * compile time, depending on its own Agent types, using the
	 * FPMAS_JSON_SET_UP() macro, that must be called from a **source** file.
	 * Otherwise, linker errors will be thrown.
	 */
	template<>
		struct light_serializer<fpmas::api::model::AgentPtr> {
			/**
			 * Serializes the \Agent represented by `pointer` to the specified
			 * \light_json `j`.
			 *
			 * Notice that since \Agent is polymorphic, the actual `to_json`
			 * call that corresponds to the most derived type of the specified
			 * \Agent is resolved at runtime.
			 *
			 * The \light_json version of the to_json() method attempts to
			 * serialize the agent with the less data possible, i.e. possibly
			 * nothing if the \Agent is default constructible, and no user
			 * defined \light_json serialization rules have been provided.
			 *
			 * If \Agent is not default constructible, the classic
			 * `nlohmann::adl_serializer<PtrWrapper<AgentType>>` is used, what
			 * might be inefficient.
			 *
			 * @see \ref default_constructible_Agent_light_serializer_to_json
			 * "light_serializer<PtrWrapper<AgentType>>::to_json() (default constructible AgentType)"
			 * @see \ref not_default_constructible_Agent_light_serializer_to_json
			 * "light_serializer<PtrWrapper<AgentType>>::to_json() (not default constructible AgentType)"
			 *
			 * @param j json output
			 * @param pointer pointer to polymorphic \Agent
			 */
			static void to_json(light_json& j, const
					fpmas::api::model::AgentPtr& pointer);

			/**
			 * Unserializes an \Agent from the specified \light_json `j`.
			 *
			 * The built \Agent is dynamically allocated, but is automatically
			 * managed by the fpmas::api::model::AgentPtr wrapper.
			 *
			 * Since \Agent is polymorphic, the concrete type that should be
			 * instantiated from the input \light_json `j` is determined at runtime.
			 *
			 * The \light_json version of the from_json() method attempts to
			 * build the \Agent using a default constructor, to bypass the
			 * classical `json` serialization process.
			 *
			 * If \Agent is not default constructible, the classic
			 * `nlohmann::adl_serializer<PtrWrapper<AgentType>>` is used, what
			 * might be inefficient.
			 *
			 * @see \ref default_constructible_Agent_light_serializer_from_json
			 * "light_serializer<PtrWrapper<AgentType>>::from_json() (default constructible AgentType)"
			 * @see \ref not_default_constructible_Agent_light_serializer_from_json
			 * "light_serializer<PtrWrapper<AgentType>>::from_json() (not default constructible AgentType)"
			 *
			 * @param j input json
			 * @return dynamically allocated \Agent, unserialized from `j`,
			 * wrapped in an fpmas::api::model::AgentPtr instance
			 */
			static fpmas::api::model::AgentPtr from_json(const light_json& j);
		};

	/**
	 * fpmas::api::model::WeakAgentPtr fpmas::io::json::light_json serialization
	 * rules **declaration**.
	 *
	 * `to_json` and `from_json` methods must be _defined_ by the user at
	 * compile time, depending on its own Agent types, using the
	 * FPMAS_JSON_SET_UP() macro, that must be called from a **source** file.
	 * Otherwise, linker errors will be thrown.
	 */
	template<>
		struct light_serializer<fpmas::api::model::WeakAgentPtr> {
			/**
			 * Equivalent to fpmas::io::json::light_serializer<fpmas::api::model::AgentPtr>::to_json(light_json&, const fpmas::api::model::AgentPtr&).
			 */
			static void to_json(light_json& j, const
					fpmas::api::model::WeakAgentPtr& pointer);

			
			/** 
			 * Equivalent to
			 * fpmas::io::json::light_serializer<fpmas::api::model::AgentPtr>::from_json(const light_json&),
			 * but the result is wrapped in an  fpmas::api::model::WeakAgentPtr
			 * instance.
			 */
			static fpmas::api::model::WeakAgentPtr from_json(const light_json& j);
		};
}}}

namespace fpmas { namespace io { namespace datapack {
	using api::model::AgentPtr;
	using api::model::WeakAgentPtr;

	/**
	 * fpmas::api::model::AgentPtr fpmas::io::datapack::Serializer
	 * serialization rules **declaration**.
	 *
	 * `to_datapack` and `from_datapack` methods must be _defined_ by the user
	 * at compile time, depending on its own Agent types, using the
	 * FPMAS_DATAPACK_SET_UP() macro, that must be called from a **source** file.
	 * Otherwise, linker errors will be thrown.
	 */
	template<>
		struct Serializer<AgentPtr> {
			/**
			 * Returns the buffer size required to serialize the polymorphic
			 * \Agent pointed by `ptr` to `p`.
			 */
			static std::size_t size(const ObjectPack& p, const AgentPtr& ptr);

			/**
			 * Serializes the polymorphic \Agent pointed by `pointer` to the
			 * specified ObjectPack.
			 *
			 * @param pack destination ObjectPack
			 * @param pointer pointer to polymorphic \Agent
			 */
			static void to_datapack(ObjectPack& pack, const AgentPtr& pointer);

			/**
			 * Unserializes an \Agent from the specified ObjectPack.
			 *
			 * Since \Agent is polymorphic, the concrete type that should be
			 * instantiated from the input ObjectPack is determined at runtime.
			 *
			 * The built \Agent is dynamically allocated, but is automatically
			 * managed by the fpmas::api::model::AgentPtr wrapper.
			 *
			 * @param pack source ObjectPack
			 * @return dynamically allocated \Agent, unserialized from `pack`,
			 * wrapped in an fpmas::api::model::AgentPtr instance
			 */
			static AgentPtr from_datapack(const ObjectPack& pack);
		};

	/**
	 * fpmas::api::model::WeakAgentPtr fpmas::io::datapack::Serializer
	 * serialization rules **declaration**.
	 *
	 * `to_datapack` and `from_datapack` methods must be _defined_ by the user
	 * at compile time, depending on its own Agent types, using the
	 * FPMAS_DATAPACK_SET_UP() macro, that must be called from a **source** file.
	 * Otherwise, linker errors will be thrown.
	 *
	 * The behavior of the specified methods are equivalent to the
	 * corresponding fpmas::io::datapack::Serializer<AgentPtr> definitions, but
	 * \Agents are wrapped in WeakAgentPtr instance, so that no automatic
	 * memory management is performed.
	 */
	template<>
		struct Serializer<WeakAgentPtr> {
			/**
			 * Equivalent to
			 * fpmas::io::datapack::Serializer<AgentPtr>>:size(const ObjectPack&, const AgentPtr&).
			 */
			static std::size_t size(const ObjectPack& pack, const WeakAgentPtr& ptr);

			/**
			 * Equivalent to fpmas::io::datapack::Serializer<AgentPtr>::to_datapack(ObjectPack& o, const AgentPtr&).
			 */
			static void to_datapack(ObjectPack& pack, const WeakAgentPtr& ptr);

			/**
			 * Equivalent to fpmas::io::datapack::Serializer<AgentPtr>::from_datapack(const ObjectPack& o),
			 * but the result is wrapped in an fpmas::api::model::WeakAgentPtr
			 * instance.
			 */
			static WeakAgentPtr from_datapack(const ObjectPack& pack);
		};

	/**
	 * fpmas::api::model::AgentPtr fpmas::io::datapack::LightSerializer
	 * serialization rules **declaration**.
	 *
	 * `to_datapack` and `from_datapack` methods must be _defined_ by the user
	 * at compile time, depending on its own Agent types, using the
	 * FPMAS_DATAPACK_SET_UP() macro, that must be called from a **source** file.
	 * Otherwise, linker errors will be thrown.
	 */
	template<>
		struct LightSerializer<AgentPtr> {
			/**
			 * Returns the buffer size required to serialize the polymorphic
			 * \Agent pointed by `ptr` into `p`.
			 *
			 * @see \ref default_constructible_Agent_light_serializer_size
			 * "LightSerializer<PtrWrapper<AgentType>>::size() (default constructible AgentType)"
			 * @see \ref
			 * not_default_constructible_Agent_light_serializer_size
			 * "LightSerializer<PtrWrapper<AgentType>>::size() (not default constructible AgentType)"
			 */
			static std::size_t size(const LightObjectPack& pack, const AgentPtr& ptr);

			/**
			 * Serializes the \Agent represented by `pointer` to the specified
			 * LightObjectPack.
			 *
			 * The LightObjectPack version of the to_datapack() method attempts to
			 * serialize the agent with the less data possible, i.e. possibly
			 * nothing if the \Agent is default constructible, and no user
			 * defined LightObjectPack serialization rules have been provided.
			 *
			 * If \Agent is not default constructible, the classic
			 * `io::datapack::Serializer<PtrWrapper<AgentType>>` is used, what
			 * might be inefficient.
			 *
			 * @see \ref default_constructible_Agent_light_serializer_to_datapack
			 * "LightSerializer<PtrWrapper<AgentType>>::to_datapack() (default constructible AgentType)"
			 * @see \ref
			 * not_default_constructible_Agent_light_serializer_to_datapack
			 * "LightSerializer<PtrWrapper<AgentType>>::to_datapack() (not default constructible AgentType)"
			 *
			 * @param pack destination LightObjectPack
			 * @param pointer pointer to polymorphic \Agent
			 */
			static void to_datapack(LightObjectPack& pack, const AgentPtr& pointer);

			/**
			 * Unserializes an \Agent from the specified LightObjectPack.
			 *
			 * The built \Agent is dynamically allocated, but is automatically
			 * managed by the fpmas::api::model::AgentPtr wrapper.
			 *
			 * Since \Agent is polymorphic, the concrete type that should be
			 * instantiated from the input LightObjectPack is determined at
			 * runtime.
			 *
			 * The LightObjectPack version of the from_datapack() method
			 * attempts to build the \Agent using a default constructor, to
			 * bypass the classical ObjectPack serialization process.
			 *
			 * If \Agent is not default constructible, the classic
			 * `io::datapack::Serializer<PtrWrapper<AgentType>>` is used, what
			 * might be inefficient.
			 *
			 * @see \ref
			 * default_constructible_Agent_light_serializer_from_datapack
			 * "light_serializer<PtrWrapper<AgentType>>::from_datapack() (default constructible AgentType)"
			 * @see \ref
			 * not_default_constructible_Agent_light_serializer_from_datapack
			 * "light_serializer<PtrWrapper<AgentType>>::from_datapack() (not default constructible AgentType)"
			 *
			 * @param pack source LightObjectPack
			 * @return dynamically allocated \Agent, unserialized from `o`,
			 * wrapped in an fpmas::api::model::AgentPtr instance
			 */
			static AgentPtr from_datapack(const LightObjectPack& pack);
		};

	/**
	 * fpmas::api::model::WeakAgentPtr fpmas::io::datapack::LightSerializer
	 * serialization rules **declaration**.
	 *
	 * `to_datapack` and `from_datapack` methods must be _defined_ by the user
	 * at compile time, depending on its own Agent types, using the
	 * FPMAS_DATAPACK_SET_UP() macro, that must be called from a **source** file.
	 * Otherwise, linker errors will be thrown.
	 *
	 * The behavior of the specified methods are equivalent to the corresponding
	 * fpmas::io::datapack::LightSerializer<AgentPtr> definitions, but \Agents
	 * are wrapped in WeakAgentPtr instance, so that no automatic memory
	 * management is performed.
	 */
	template<>
		struct LightSerializer<WeakAgentPtr> {
			/**
			 * Equivalent to
			 * fpmas::io::datapack::LightSerializer<AgentPtr>::size(const LightObjectPack&, const AgentPtr&).
			 */
			static std::size_t size(const LightObjectPack& pack, const WeakAgentPtr& ptr);

			/**
			 * Equivalent to fpmas::io::datapack::LightSerializer<AgentPtr>::to_datapack(LightObjectPack&, const AgentPtr&).
			 */
			static void to_datapack(LightObjectPack& pack, const WeakAgentPtr& ptr);

			/** 
			 * Equivalent to
			 * fpmas::io::datapack::LightSerializer<AgentPtr>::from_datapack(const LightObjectPack&),
			 * but the result is wrapped in an  fpmas::api::model::WeakAgentPtr
			 * instance.
			 */
			static WeakAgentPtr from_datapack(const LightObjectPack& pack);
		};
}}}

namespace nlohmann {
	/**
	 * fpmas::api::model::AgentPtr `nlohmann::json` serialization rules **declaration**.
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
			 * json `j`.
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
			 * instantiated from the input json `j` is determined at runtime.
			 *
			 * @param j input json
			 * @return dynamically allocated \Agent, unserialized from `j`,
			 * wrapped in an fpmas::api::model::AgentPtr instance
			 */
			static fpmas::api::model::AgentPtr from_json(const json& j);
		};

	/**
	 * fpmas::api::model::WeakAgentPtr `nlohmann::json`
	 * serialization rules **declaration**.
	 *
	 * `to_json` and `from_json` methods must be _defined_ by the user at compile time,
	 * depending on its own Agent types, using the FPMAS_JSON_SET_UP() macro,
	 * that must be called from a **source** file. Otherwise, linker errors
	 * will be thrown.
	 */
	template<>
		struct adl_serializer<fpmas::api::model::WeakAgentPtr> {
			
			/**
			 * Equivalent to nlohmann::adl_serializer<fpmas::api::model::AgentPtr>::to_json(json& j, const fpmas::api::model::AgentPtr&).
			 */
			static void to_json(json& j, const fpmas::api::model::WeakAgentPtr& pointer);

			/**
			 * Equivalent to nlohmann::adl_serializer<fpmas::api::model::AgentPtr>::from_json(const json& j),
			 * but the result is wrapped in an fpmas::api::model::WeakAgentPtr
			 * instance.
			 */
			static fpmas::api::model::WeakAgentPtr from_json(const json& j);
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
#endif
