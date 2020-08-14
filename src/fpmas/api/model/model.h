#ifndef MODEL_API_H
#define MODEL_API_H

#include "fpmas/api/graph/distributed_graph.h"
#include "fpmas/api/scheduler/scheduler.h"
#include "fpmas/api/runtime/runtime.h"
#include "fpmas/api/utils/ptr_wrapper.h"

namespace fpmas { namespace api {namespace model {
	class Agent;
	class AgentTask;
	class AgentPtr;

	/**
	 * AgentNode type.
	 */
	typedef api::graph::DistributedNode<AgentPtr> AgentNode;
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
	typedef int TypeId;

	/**
	 * A smart pointer used to manage an Agent pointer.
	 *
	 * This type can be used as a datatype T within an api::graph::DistributedGraph.
	 *
	 * @see AgentNode
	 * @see AgentGraph
	 */
	class AgentPtr : public api::utils::VirtualPtrWrapper<api::model::Agent> {
		public:
			/**
			 * Default AgentPtr constructor.
			 */
			AgentPtr() : VirtualPtrWrapper() {}
			/**
			 * AgentPtr constructor.
			 *
			 * @param agent agent pointer to manage
			 */
			AgentPtr(api::model::Agent* agent)
				: VirtualPtrWrapper(agent) {}

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
			 * Moves other's agent data _into_ this agent pointer.
			 *
			 * The old agent pointer is replaced by other's agent according to
			 * the following rules :
			 * - other's agent pointer group, task, node and graph fields are
			 *   replaced by **this current agent pointer values**.
			 * - this current agent pointer task is rebound to other's agent
			 *   pointer
			 * - this agent pointer is replaced by other's agent pointer
			 *
			 * In other terms, this agent pointer wrapper instance can be
			 * considered as an "host" for an agent pointer. When other is
			 * moved into this instance, any internal agent data is moved from
			 * other to this, preserving former links to task, node and graph
			 * so that those objects do not notify that the internal pointer
			 * has changed.
			 *
			 * This mechanism is used when Agent updates are received in a
			 * synchro::DataUpdatePack for example, depending on the
			 * api::synchro::SyncMode used. This AgentPtr instance is supposed
			 * to be already contained in the local graph, as a DISTANT node
			 * data, and updates, received in the other AgentPtr, are _moved
			 * into_ this AgentPtr.
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

	class AgentGroup;

	/**
	 * Generic Agent API.
	 *
	 * \Agents is a simulation are assumed to take advantage of polymorphism : a
	 * single type is used to store \Agents in an api::graph::DistributedGraph
	 * (more precisely, AgentPtr is used), but pointed \Agents might have
	 * different types and implement different the act() functions to produce
	 * different behaviors. Such considerations implies a few constraints :
	 * - each agent type must be associated to an unique TypeId, so that the
	 *   underlying type of each Agent can be deduced thanks to the method
	 *   typeId().
	 * - a copy() method must be implemented to dynamically allocate copies of
	 *   \Agents, with the same underlying type, but directly from this generic
	 *   API.
	 */
	class Agent {
		public:
			/**
			 * Returns the ID of the AgentGroup to which this Agent belong.
			 *
			 * @return group id
			 */
			virtual GroupId groupId() const = 0;
			/**
			 * Sets this Agent group ID.
			 *
			 * @param agent group id 
			 */
			virtual void setGroupId(GroupId id) = 0;


			/**
			 * Returns a pointer to the AgentGroup to which this Agent belong.
			 *
			 * @return pointer to agent group
			 */
			virtual AgentGroup* group() = 0;
			/**
			 * \copydoc group()
			 */
			virtual const AgentGroup* group() const = 0;
			/**
			 * Sets the internal pointer of the AgentGroup to which this Agent
			 * belong.
			 *
			 * @param group agent group
			 */
			virtual void setGroup(AgentGroup* group) = 0;

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
			 * Returns a pointer to the \AgentGraph that contains this Agent's
			 * \AgentNode.
			 *
			 * @return pointer to \AgentGraph
			 */
			virtual AgentGraph* graph() = 0;
			/**
			 * \copydoc graph()
			 */
			virtual const AgentGraph* graph() const = 0;
			/**
			 * Sets the internal pointer of the \AgentGraph that contains this
			 * Agent's \AgentNode.
			 *
			 * @param graph pointer to \AgentGraph
			 */
			virtual void setGraph(AgentGraph* graph) = 0;

			/**
			 * Returns a pointer to the AgentTask associated to this Agent,
			 * that is used to execute the Agent (thanks to the act()
			 * function).
			 *
			 * @return agent task
			 */
			virtual AgentTask* task() = 0; 
			/**
			 * \copydoc task()
			 */
			virtual const AgentTask* task() const = 0; 
			/**
			 * Sets the AgentTask associated to this Agent.
			 *
			 * @param task agent task
			 */
			virtual void setTask(AgentTask* task) = 0;

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
			virtual const Agent* agent() const = 0;
			/**
			 * Sets the internal pointer of the associated Agent.
			 *
			 * @param agent task's agent
			 */
			virtual void setAgent(Agent*) = 0;

			virtual ~AgentTask(){}
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
			 * Returns the ID of this group.
			 *
			 * @return group id
			 */
			virtual GroupId groupId() const = 0;

			/**
			 * Adds a new Agent to this group.
			 *
			 * Also builds a new \AgentNode into the current \AgentGraph and
			 * associates it to the Agent.
			 *
			 * @param agent agent to add
			 */
			virtual void add(Agent* agent) = 0;
			/**
			 * Removes an Agent from this group.
			 *
			 * The Agent is completely removed from the simulation, so the
			 * associated \AgentNode is also globally removed from the
			 * \AgentGraph.
			 *
			 * @param agent agent to remove
			 */
			virtual void remove(Agent* agent) = 0;

			/**
			 * Inserts an Agent into this AgentGroup.
			 *
			 * This function is only used during the node migration process,
			 * and is not supposed to be used by the final user.
			 *
			 * Contrary to add(), this method assumes that an AgentNode has
			 * already been built for the Agent.
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
			 * Agent (and its associated AgentNode) from the simulation.
			 *
			 * @param agent agent to erase from this group
			 */
			virtual void erase(AgentPtr* agent) = 0;

			/**
			 * Returns the list of all \Agents currently in this AgentGroup.
			 *
			 * @return \Agents in this group
			 */
			virtual std::vector<AgentPtr*> agents() const = 0;

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
			 */
			virtual api::scheduler::Job& job() = 0;
			/**
			 * \copydoc job()
			 */
			virtual const api::scheduler::Job& job() const = 0;

			virtual ~AgentGroup(){}
	};

	class Model {
		public:
			typedef api::graph::DistributedGraph<AgentPtr> AgentGraph;

			virtual AgentGraph& graph() = 0;
			virtual api::scheduler::Scheduler& scheduler() = 0;
			virtual api::runtime::Runtime& runtime() = 0;

			virtual const api::scheduler::Job& loadBalancingJob() const = 0;

			virtual AgentGroup& buildGroup() = 0;
			virtual AgentGroup& getGroup(GroupId) const = 0;
			virtual const std::unordered_map<GroupId, AgentGroup*>& groups() const = 0;

			virtual ~Model(){}
	};
}}}
#endif
