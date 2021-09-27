#ifndef FPMAS_MODEL_API_H
#define FPMAS_MODEL_API_H

/** \file src/fpmas/api/model/model.h
 * Model API
 */

#include <typeindex>

#include "fpmas/api/graph/distributed_graph.h"
#include "fpmas/api/runtime/runtime.h"
#include "fpmas/api/utils/ptr_wrapper.h"

namespace fpmas { namespace api {namespace model {
	using api::graph::DistributedId;

	class Agent;
	class AgentTask;
	class AgentPtr;

	/**
	 * AgentNode type.
	 */
	typedef api::graph::DistributedNode<AgentPtr> AgentNode;
	/**
	 * AgentEdge type.
	 */
	typedef api::graph::DistributedEdge<AgentPtr> AgentEdge;

	/**
	 * AgentGraph type.
	 */
	typedef api::graph::DistributedGraph<AgentPtr> AgentGraph;
	/**
	 * Group ID type.
	 */
	typedef int GroupId;
	/**
	 * Agent type ID type.
	 */
	typedef std::type_index TypeId;

	/**
	 * A smart pointer used to manage an Agent pointer.
	 *
	 * This type can be used as a datatype T within an api::graph::DistributedGraph.
	 *
	 * @see AgentNode
	 * @see AgentGraph
	 */
	class AgentPtr : public api::utils::PtrWrapper<api::model::Agent> {
		public:
			/**
			 * Default AgentPtr constructor.
			 */
			AgentPtr() : PtrWrapper() {}
			/**
			 * AgentPtr constructor.
			 *
			 * @param agent agent pointer to manage
			 */
			AgentPtr(api::model::Agent* agent)
				: PtrWrapper(agent) {}

			/**
			 * Moves the internal Agent pointer from other to this.
			 *
			 * @param other other pointer wrapper to move from
			 */
			AgentPtr(AgentPtr&& other);

			/**
			 * Copies the other internal agent pointer data to this, using
			 * Agent::copy().
			 * 
			 * **Must be used only for serialization purpose.** (for example
			 * when data is copied to an MPI buffer through a synchro::DataUpdatePack
			 * instance)
			 *
			 * Only the Agent data and its GroupId is copied, but task, node or
			 * graph fields of the copied agent is left undefined, and so the
			 * original task associated to other's agent is **not** rebound to
			 * the copied agent.
			 *
			 * @param other other pointer wrapper to copy from
			 */
			AgentPtr(const AgentPtr& other);

			/**
			 * Moves other's agent data _into_ this agent pointer, using
			 * Agent::moveAssign().
			 *
			 * @param other other AgentPtr to move into this
			 */
			AgentPtr& operator=(AgentPtr&& other);

			/**
			 * Copies the other internal agent pointer data to this, following
			 * the same rules as AgentPtr(const AgentPtr&).
			 *
			 * Also deletes the old agent pointer if it was not null.
			 *
			 * @param other other pointer wrapper to copy from
			 */
			AgentPtr& operator=(const AgentPtr&);

			/**
			 * Deletes the internal agent pointer if it's not null.
			 */
			~AgentPtr();
	};

	class Model;
	class AgentGroup;

	/**
	 * Generic Agent API.
	 *
	 * \Agents in a simulation are assumed to take advantage of polymorphism : a
	 * single type is used to store \Agents in an api::graph::DistributedGraph
	 * (more precisely, AgentPtr is used), but pointed \Agents might have
	 * different types and implement different the act() functions to produce
	 * different behaviors.
	 */
	class Agent {
		public:
			/**
			 * @deprecated
			 * Returns the ID of the AgentGroup to which this Agent belong.
			 *
			 * This method is deprecated and will be removed in a next release.
			 * Use groupIds() instead.
			 *
			 * @return group id
			 */
			[[deprecated]]
			virtual GroupId groupId() const = 0;

			/**
			 * Returns ids of AgentGroups to which the Agent belong.
			 *
			 * @return list of group ids
			 */
			virtual std::vector<GroupId> groupIds() const = 0;

			/**
			 * @deprecated
			 * Sets this Agent group ID.
			 *
			 * This method is deprecated and will be removed in a next release.
			 * Use addGroupId() instead.
			 *
			 * @param id agent group id 
			 */
			[[deprecated]]
			virtual void setGroupId(GroupId id) = 0;

			/**
			 * Adds `id` to this Agent's group ids.
			 *
			 * @param id agent group id 
			 */
			virtual void addGroupId(GroupId id) = 0;

			/**
			 * Removes `id` from this Agent's group ids.
			 *
			 * @param id agent group id
			 */
			virtual void removeGroupId(GroupId id) = 0;


			/**
			 * @deprecated
			 * Returns a pointer to the AgentGroup to which this Agent belong.
			 *
			 * This method is deprecated and will be removed in a next release.
			 * Use groups() instead.
			 *
			 * @return pointer to agent group
			 */
			[[deprecated]]
			virtual AgentGroup* group() = 0;
			/**
			 * \copydoc group()
			 */
			[[deprecated]]
			virtual const AgentGroup* group() const = 0;

			/**
			 * Returns a list of \AgentGroups to which this Agent belong.
			 *
			 * @return agent group list
			 */
			virtual std::vector<AgentGroup*> groups() = 0;

			/**
			 * \copydoc groups()
			 */
			virtual std::vector<const AgentGroup*> groups() const = 0;

			/**
			 * @deprecated
			 * Sets the internal pointer of the AgentGroup to which this Agent
			 * belong.
			 *
			 * This method is deprecated and will be removed in a next release.
			 * Use addGroup() instead.
			 *
			 * @param group agent group
			 */
			[[deprecated]]
			virtual void setGroup(AgentGroup* group) = 0;

			/**
			 * Adds `group` to the list of \AgentGroups to which this Agent
			 * belong.
			 *
			 * @param group agent group
			 */
			virtual void addGroup(AgentGroup* group) = 0;

			/**
			 * Removes `group` from the list of \AgentGroups to wich this Agent
			 * belong.
			 *
			 * Morever, the entry corresponding to this `group`'s id is removed
			 * from tasks().
			 *
			 * @param group agent group
			 */
			virtual void removeGroup(AgentGroup* group) = 0;

			/**
			 * Returns the ID of the type of this Agent.
			 */
			virtual TypeId typeId() const = 0;
			/**
			 * Allocates a new Agent pointer, considered as a copy of the
			 * current Agent.
			 *
			 * @return pointer to a new allocated Agent, copied from this Agent
			 */
			virtual Agent* copy() const = 0;

			/**
			 * Copies the specified agent to this.
			 *
			 * @param agent agent to copy from
			 */
			virtual void copyAssign(Agent* agent) = 0;

			/**
			 * Moves the specified agent into this.
			 *
			 * The specified `agent` is assumed to be an updated version of
			 * this agent.
			 *
			 * In the move process, the following fields of "this" host agent
			 * keep unchanged, and so are not moved from `agent` to this :
			 * - node()
			 * - model()
			 *
			 * groups() and tasks() fields are particularly handled such as the
			 * `agent`s groupIds() is assumed to be up to date and synchronized
			 * with this groupIds().
			 *
			 * More particularly:
			 * - this agent is added to groups corresponding to ids contained in
			 *   `agent->groupIds()` but not in `this->groupIds()` (see
			 *   AgentGroup::add())
			 * - this agent is removed from groups corresponding to ids
			 *   contained in `this->groupIds()` but not in `agent->groupIds()`
			 *   (see AgentGroup::remove())
			 * - other groups are left unchanged.
			 *
			 * `this->groupIds()` is also updated according to the previous
			 * rules.
			 *
			 * tasks() are assumed to stay consistent with the rules expressed
			 * above, since tasks are bound to agent groups to which the agent
			 * belong. Those tasks are updated automatically by
			 * AgentGroup::add() and AgentGroup::remove().
			 *
			 * Other fields are moved according to implementation defined
			 * rules.
			 *
			 * @param agent agent to move
			 */
			virtual void moveAssign(Agent* agent) = 0;

			/**
			 * Returns a pointer to the \AgentNode that contains this Agent.
			 *
			 * @return pointer to wrapping \AgentNode
			 */
			virtual AgentNode* node() = 0;
			/**
			 * \copydoc node()
			 */
			virtual const AgentNode* node() const = 0;
			/**
			 * Sets the internal pointer of the \AgentNode that contains this Agent.
			 *
			 * @param node pointer to wrapping \AgentNode
			 */
			virtual void setNode(AgentNode* node) = 0;

			/**
			 * Returns a pointer to the Model that contains this Agent.
			 *
			 * @return pointer to Model
			 */
			virtual Model* model() = 0;
			/**
			 * \copydoc model()
			 */
			virtual const Model* model() const = 0;
			/**
			 * Sets the internal pointer of the Model that contains this
			 * Agent.
			 *
			 * @param model pointer to Model
			 */
			virtual void setModel(Model* model) = 0;

			/**
			 * Returns a pointer to the AgentTask associated to this Agent,
			 * that is used to execute the Agent.
			 *
			 * @return agent task
			 */
			[[deprecated]]
			virtual AgentTask* task() = 0; 
			/**
			 * \copydoc task()
			 */
			[[deprecated]]
			virtual const AgentTask* task() const = 0; 
			/**
			 * Sets the AgentTask associated to this Agent.
			 *
			 * @param task agent task
			 */
			[[deprecated]]
			virtual void setTask(AgentTask* task) = 0;

			/**
			 * Returns the AgentTask associated to this Agent in the AgentGroup
			 * corresponding to `id`.
			 *
			 * Behavior is undefined if the Agent does not belong to an
			 * AgentGroup with the specified `id`.
			 *
			 * @param id group id
			 */
			virtual AgentTask* task(GroupId id) = 0;
			/**
			 * \copydoc task(int)
			 */
			virtual const AgentTask* task(GroupId id) const = 0; 

			/**
			 * Sets the AgentTask associated to this Agent in the AgentGroup
			 * corresponding to `id`.
			 *
			 * Behavior is undefined if the Agent does not belong to an
			 * AgentGroup with the specified `id`.
			 *
			 * @param id group id
			 * @param task agent task
			 */
			virtual void setTask(GroupId id, AgentTask* task) = 0;

			/**
			 * Returns a map containing all the tasks associated to this Agent
			 * in the \AgentGroups to which it belongs.
			 *
			 * Exactly one task is associated to the agent in each group.
			 *
			 * @return tasks associated to this agent
			 */
			virtual const std::unordered_map<GroupId, AgentTask*>& tasks() = 0;

			/**
			 * Executes the behavior of the Agent.
			 *
			 * Notice that the behavior of an Agent can involve graph and node
			 * operations, such as linking or unlinking, thanks to the Agent
			 * methods graph() and node().
			 */
			virtual void act() = 0;

			virtual ~Agent(){}
	};

	/**
	 * AgentTask API.
	 *
	 * An AgentTask is a api::scheduler::NodeTask responsible for the execution
	 * of an Agent, thanks to the Agent::act() function.
	 */
	class AgentTask : public api::scheduler::NodeTask<AgentPtr> {
		public:
			/**
			 * Returns a pointer to the associated Agent.
			 *
			 * @return task's agent
			 */
			virtual const AgentPtr& agent() const = 0;
			
			virtual ~AgentTask(){}
	};

	/**
	 * Agent Behavior API.
	 *
	 * The purpose of a Behavior is to be _executed_ on \Agents, for example
	 * calling a user specified method on the Agent to execute.
	 *
	 * A single Behavior instance might be executed on many \Agents: in
	 * practice, each AgentGroup is bound to a Behavior.
	 *
	 * @see Model::buildGroup(GroupId, const Behavior&)
	 */
	class Behavior {
		public:
			/**
			 * Executes this behavior on the specified `agent`.
			 *
			 * @param agent on which this behavior must be executed.
			 */
			virtual void execute(Agent* agent) const = 0;

			virtual ~Behavior(){}
	};

	/**
	 * AgentGroup API.
	 *
	 * An AgentGroup describes a group of \Agents that can be executed
	 * independently in any order, potentially in parallel.
	 *
	 * More precisely, the AgentGroup is associated to an api::scheduler::Job
	 * that contains all the tasks corresponding to \Agents added in the group.
	 *
	 * The AgentGroup's Job can then be scheduled thanks to the
	 * api::scheduler::Scheduler API to define when the AgentGroup should be
	 * executed.
	 *
	 * A Model can be used to conveniently build \AgentGroups.
	 */
	class AgentGroup {
		public:
			/**
			 * Events that can be emitted by the AgentGroup interface.
			 */
			enum Event {
				/**
				 * Sent after a node has been inserted in the AgentGroup.
				 */
				INSERT,
				/**
				 * Sent before an Agent is erased from the AgentGroup.
				 */
				ERASE,
				/**
				 * Sent after a node has been added to the AgentGroup.
				 */
				ADD,
				/**
				 * Sent before an Agent is removed from the AgentGroup.
				 */
				REMOVE
			};
			/**
			 * Returns the ID of this group.
			 *
			 * @return group id
			 */
			virtual GroupId groupId() const = 0;

			/**
			 * Returns the Behavior associated to this group.
			 *
			 * @return group's behavior
			 */
			virtual const Behavior& behavior() = 0;

			/**
			 * Adds a new Agent to this group.
			 *
			 * \AgentGroups to which an `agent` is added take ownership of the
			 * `agent`, that must be dynamically allocated (using a `new`
			 * statement). The `agent` lives while it belongs to at least one
			 * group, and is deleted when it is removed from the last group.
			 *
			 * add() builds a new \AgentNode into the current \AgentGraph and
			 * associates it to the Agent if the `agent` was not contained in
			 * any group yet.
			 * 
			 * This group's groupId() is added to `agent`s Agent::groupIds().
			 *
			 * A task is also bound to the `agent` and Agent::groups() is
			 * updated as defined in insert().
			 *
			 * Moreover, if the `agent` is \LOCAL, the previous task is added
			 * to agentExecutionJob().
			 *
			 * In consequence, it is possible to dynamically add() \LOCAL
			 * agents to groups.
			 *
			 * Notice that when the `agent` is not bound to a node yet, i.e.
			 * this is the first group the agent is inserted into so that a new
			 * node is built, the `agent` automatically becomes \LOCAL, since
			 * the new node is built locally.
			 *
			 * If a \DISTANT `agent` is added to the group, it is just inserted
			 * in the group.  However, this is a mechanic that must be used
			 * only internally, since adding a \DISTANT agent to the group is
			 * not guaranteed to report the operation on other processes.
			 *
			 * @param agent agent to add
			 */
			virtual void add(Agent* agent) = 0;

			/**
			 * Removes an Agent from this group.
			 *
			 * If this was the last group containing the Agent, it is
			 * completely removed from the simulation, so the associated
			 * \AgentNode is also globally removed from the \AgentGraph.
			 *
			 * Else, the agent is simply removed from this group as defined in
			 * erase().
			 *
			 * If the `agent` is \LOCAL, the task associated to this group's
			 * behavior() is removed from agentExecutionJob().
			 *
			 * @param agent agent to remove
			 */
			virtual void remove(Agent* agent) = 0;

			/**
			 * Removes all local agents from this group.
			 */
			virtual void clear() = 0;

			/**
			 * Inserts an Agent into this AgentGroup.
			 *
			 * This function is only used during the node migration process,
			 * and is not supposed to be used by the final user.
			 *
			 * Contrary to add(), this method assumes that an \AgentNode has
			 * already been built for the Agent.
			 *
			 * When this method is called, it is assumed that `agent`'s
			 * Agent::groupIds() list already contains this groupId(), so the
			 * `agent` should be inserted in the group.
			 *
			 * Upon return, `agent`'s Agent::groups() must be updated, and a
			 * task that execute behavior() on this agent must be bound to the
			 * agent. The corresponding task is then accessible from
			 * Agent::task(GroupId) and Agent::tasks().
			 *
			 * @param agent agent to insert into this group
			 */
			virtual void insert(AgentPtr* agent) = 0;
			/**
			 * Erases an Agent from this AgentGroup.
			 *
			 * This function is only used during the node migration process,
			 * and is not supposed to be used by the final user.
			 *
			 * Contrary to remove(), this method does not globally removes the
			 * Agent (and its associated \AgentNode) from the simulation.
			 *
			 * Upon return, `agent`'s Agent::groupIds() and Agent::groups()
			 * must be updated, and the task previously bound to the agent by
			 * insert() must be unbound.
			 *
			 * @param agent agent to erase from this group
			 */
			virtual void erase(AgentPtr* agent) = 0;

			/**
			 * Returns the list of all \Agents currently in this AgentGroup,
			 * including `DISTANT` representations of Agents not executed on
			 * the current process.
			 *
			 * The returned list is **invalidated** by the following operations:
			 * - fpmas::api::graph::DistributedGraph::synchronize()
			 * - fpmas::api::synchro::Mutex::read() or
			 *   fpmas::api::synchro::Mutex::acquire() on any Agent of the
			 *   list.
			 *
			 * @return \Agents in this group
			 */
			virtual std::vector<Agent*> agents() const = 0;


			/**
			 * Returns the list of \LOCAL Agents currently in this AgentGroup,
			 * that are executed on the current process.
			 *
			 * The returned list is **invalidated** by the following operations:
			 * - fpmas::api::graph::DistributedGraph::synchronize()
			 * - fpmas::api::synchro::Mutex::read() or
			 *   fpmas::api::synchro::Mutex::acquire() on any Agent of the
			 *   list.
			 *
			 * @return \LOCAL \Agents in this group
			 */
			virtual std::vector<Agent*> localAgents() const = 0;

			/**
			 * Returns the list of \DISTANT Agents currently in this AgentGroup,
			 * represented to preserve \LOCAL agents neighbors but not executed
			 * on this process.
			 *
			 * The process on which a \DISTANT `agent` is executed can be
			 * retrieved with `agent->node()->location()`.
			 *
			 * The returned list is **invalidated** by the following operations:
			 * - fpmas::api::graph::DistributedGraph::synchronize()
			 * - fpmas::api::synchro::Mutex::read() or
			 *   fpmas::api::synchro::Mutex::acquire() on any Agent of the
			 *   list.
			 *
			 * @return \DISTANT \Agents in this group
			 */
			virtual std::vector<Agent*> distantAgents() const = 0;

			/**
			 * Returns a reference to the \Job associated to this AgentGroup.
			 *
			 * This \Job contains all the \AgentTasks associated to all \Agents
			 * in this group.
			 *
			 * This \Job can be scheduled to define when this AgentGroup must
			 * be executed.
			 *
			 * @par Example
			 * ```cpp
			 * // Schedules agent_group to be executed from iteration 10 with a
			 * // period of 2
			 * scheduler.schedule(10, 2, agent_group.job());
			 * ```
			 *
			 * See the complete api::scheduler::Scheduler interface for more
			 * scheduling options.
			 *
			 * This method as been deprecated and will be removed in a next
			 * release. Use agentExecutionJob() or jobs() instead, depending on
			 * use case.
			 */
			[[deprecated]]
			virtual api::scheduler::Job& job() = 0;
			/**
			 * \copydoc job()
			 */
			[[deprecated]]
			virtual const api::scheduler::Job& job() const = 0;

			/**
			 * Returns a reference to the internal \Job specifically used to
			 * execute behavior() on the agents() of the group.
			 *
			 * Any necessary graph synchronization is included at the end of
			 * the \Job, after all agents execution.
			 *
			 * However this job is not intended to be scheduled directly: use
			 * jobs() instead.
			 *
			 * @return agent execution job
			 * @see jobs()
			 */
			virtual api::scheduler::Job& agentExecutionJob() = 0;

			/**
			 * \copydoc agentExecutionJob()
			 */
			virtual const api::scheduler::Job& agentExecutionJob() const = 0;

			/**
			 * Returns the list of \Jobs associated to this group.
			 *
			 * This list must at least contain the agentExecutionJob(). Extra
			 * processes might be added internally to ensure the proper
			 * execution of \Agents: in consequence, this list of job can be
			 * safely scheduled to plan agents execution.
			 *
			 * @par Example
			 * ```cpp
			 * // Schedules agent_group to be executed from iteration 10 with a
			 * // period of 2
			 * scheduler.schedule(10, 2, agent_group.jobs());
			 * ```
			 *
			 * See the complete api::scheduler::Scheduler interface for more
			 * scheduling options.
			 *
			 * @return list of jobs associated to this group
			 */
			virtual api::scheduler::JobList jobs() const = 0;

			/**
			 * Registers a callback function to be called when the specified
			 * Event is emitted.
			 *
			 * @param event event that triggers the callback
			 * @param callback callback called when the event occurs
			 */
			virtual void addEventHandler(
					Event event, api::utils::Callback<Agent*>* callback
					) = 0;
			/**
			 * Unregisters a callback function previously registered for the
			 * spespecified event.
			 *
			 * The behavior is undefined if the callback was not previously
			 * registered using addEventHandler().
			 *
			 * @param event event that triggers the callback
			 * @param callback callback called when the event occurs
			 */
			virtual void removeEventHandler(
					Event event, api::utils::Callback<Agent*>* callback
					) = 0;

			virtual ~AgentGroup(){}
	};

	/**
	 * A type used to represent lists of \AgentGroups.
	 */
	typedef std::vector<std::reference_wrapper<AgentGroup>> GroupList;

	/**
	 * Model API.
	 *
	 * A Model defines and instantiates all the components required to run an
	 * FPMAS Multi-Agent simulation.
	 */
	class Model {
		protected:
			/**
			 * Inserts an AgentGroup into this model.
			 *
			 * @param id id of the group
			 * @param group agent group to insert
			 */
			virtual void insert(GroupId id, AgentGroup* group) = 0;

		public:
			/**
			 * Model's \AgentGraph.
			 *
			 * @return agent graph
			 */
			virtual AgentGraph& graph() = 0;

			/**
			 * \copydoc graph()
			 */
			virtual const AgentGraph& graph() const = 0;

			/**
			 * Model's \Scheduler.
			 *
			 * @return scheduler
			 */
			virtual api::scheduler::Scheduler& scheduler() = 0;

			/**
			 * \copydoc scheduler()
			 */
			virtual const api::scheduler::Scheduler& scheduler() const = 0;

			/**
			 * Model's \Runtime, used to execute the scheduler().
			 *
			 * @return runtime
			 */
			virtual api::runtime::Runtime& runtime() = 0;

			/**
			 * \copydoc runtime()
			 */
			virtual const api::runtime::Runtime& runtime() const = 0;

			/**
			 * A predefined LoadBalancing \Job.
			 *
			 * This \Job can be scheduled to define when and at which frequency
			 * the LoadBalancing algorithm (and the graph distribution) will be
			 * performed.
			 *
			 * \par Example
			 * ```cpp
			 * // Schedules the LoadBalancing algorithm to be applied every 10
			 * // iterations.
			 * model.scheduler().schedule(0, 10, model.loadBalancingJob());
			 * ```
			 *
			 * @return reference to the predefined load balancing job
			 */
			virtual const api::scheduler::Job& loadBalancingJob() const = 0;

			/**
			 * Builds a new AgentGroup associated to this Model.
			 *
			 * The user can add its own \Agents to the built group, and
			 * schedule it so that \Agents will be executed. More precisely,
			 * the default behavior defined by this group calls the
			 * Agent::act() method, that do nothing by default but can be
			 * overriden by the user. Other behaviors can be specified using
			 * the addGroup(GroupId, const Behavior&) method.
			 *
			 * Notice that each AgentGroup is expected to be associated to a
			 * unique GroupId. The FPMAS_DEFINE_GROUPS() macro can be used to
			 * easily generate unique GroupIds.
			 *
			 *
			 * \par Example
			 * Assuming `Agent1` and `Agent2` are predefined user implemented \Agents :
			 * ```cpp
			 * using fpmas::api::model::AgentGroup;
			 *
			 * enum Groups : fpmas::api::model::GroupId {
			 * 	G_1, G_2
			 * };
			 *
			 * AgentGroup& group_1 = model.buildGroup(G_1);
			 * AgentGroup& group_2 = model.buildGroup(G_2);
			 *
			 * group_1.add(new Agent1);
			 * group_1.add(new Agent1);
			 *
			 * group_2.add(new Agent2);
			 * // Different Agent types can be added
			 * // to each group
			 * group_2.add(new Agent1);
			 *
			 * // Schedules Agents of group_1 to be executed each iteration
			 * model.scheduler().schedule(0, 1, group_1.job());
			 * // Schedules Agents of group_2 to be executed each 2 iterations
			 * model.scheduler().schedule(0, 2, group_2.job());
			 * ```
			 *
			 * @param id unique group id
			 */
			virtual AgentGroup& buildGroup(GroupId id) = 0;

			/**
			 * Builds a new AgentGroup associated to this Model.
			 *
			 * The user can add its own \Agents to the built group, and
			 * schedule it so that \Agents will be executed.
			 *
			 * Jobs generated by the build AgentGroup will execute the
			 * specified `behavior` on agents contained in the group.
			 *
			 * The `behavior`s storage duration must be greater or equal to
			 * this AgentGroup storage duration.
			 *
			 * Each AgentGroup is expected to be associated to a unique
			 * GroupId. The FPMAS_DEFINE_GROUPS() macro can be used to easily
			 * generate unique GroupIds.
			 *
			 * @param id unique group id
			 * @param behavior behavior to execute on agents of the group
			 */
			virtual AgentGroup& buildGroup(GroupId id, const Behavior& behavior) = 0;

			/**
			 * Removes the specified group from the model.
			 *
			 * All local agents are removed from group, and the group is
			 * deleted.
			 *
			 * @param group group to remove from the model
			 */
			virtual void removeGroup(AgentGroup& group) = 0;

			/**
			 * Gets a reference to an AgentGroup previously created with
			 * buildGroup().
			 *
			 * Behavior is undefined if the specified ID does not correspond to
			 * a previously built group.
			 *
			 * @param id group id
			 * @return associated agent group
			 */
			virtual AgentGroup& getGroup(GroupId id) const = 0;

			/**
			 * Returns the map of \AgentGroups currently defined in the Model.
			 *
			 * @return agent groups
			 */
			virtual const std::unordered_map<GroupId, AgentGroup*>& groups() const = 0;

			/**
			 * Defines a new relation from `src_agent` to `tgt_agent` on the
			 * given `layer`, that can be interpreted as a relation type.
			 *
			 * The FPMAS_DEFINE_LAYERS() macro can be used to easily generate
			 * `LayerIds`.
			 *
			 * @param src_agent source agent
			 * @param tgt_agent target agent
			 * @param layer layer id
			 */
			virtual AgentEdge* link(Agent* src_agent, Agent* tgt_agent, api::graph::LayerId layer) = 0;
			/**
			 * Delete a previously defined agent relation, unlinking the
			 * corresponding edge.
			 *
			 * @param edge edge to unlink
			 */
			virtual void unlink(AgentEdge* edge) = 0;

			/**
			 * Returns the MPI communicator used by this model.
			 *
			 * @return MPI communicator
			 */
			virtual api::communication::MpiCommunicator& getMpiCommunicator() = 0;

			/**
			 * \copydoc getMpiCommunicator()
			 */
			virtual const api::communication::MpiCommunicator& getMpiCommunicator() const = 0;

			/**
			 * Erases all agents from the Model.
			 *
			 * This is assumed to be performed by all the processes in the same
			 * epoch, even if the operation itself does not necessarily
			 * requires communications (since it is assumed that all agents are
			 * erased from any process anyway).
			 */
			virtual void clear() = 0;
		public:
			virtual ~Model(){}
	};

	/**
	 * AgentPtr LoadBalancing specialization.
	 */
	using LoadBalancing = fpmas::api::graph::LoadBalancing<AgentPtr>;
}}}
#endif
