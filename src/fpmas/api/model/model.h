#ifndef MODEL_API_H
#define MODEL_API_H

#include "fpmas/api/graph/parallel/distributed_graph.h"
#include "fpmas/api/scheduler/scheduler.h"
#include "fpmas/api/runtime/runtime.h"
#include "fpmas/api/utils/ptr_wrapper.h"

namespace fpmas::api::model {
	class Agent;
	class AgentTask;
	class AgentPtrWrapper;
	typedef AgentPtrWrapper AgentPtr;
	typedef api::graph::parallel::DistributedNode<AgentPtr> AgentNode;
	typedef api::graph::parallel::DistributedGraph<AgentPtr> AgentGraph;
	typedef int GroupId;
	typedef int TypeId;

	class AgentPtrWrapper : public api::utils::VirtualPtrWrapper<api::model::Agent> {
		public:
			AgentPtrWrapper() : VirtualPtrWrapper() {}
			AgentPtrWrapper(api::model::Agent* agent)
				: VirtualPtrWrapper(agent) {}

			AgentPtrWrapper(AgentPtrWrapper&&);
			AgentPtrWrapper(const AgentPtrWrapper&);
			AgentPtrWrapper& operator=(AgentPtrWrapper&&);
			AgentPtrWrapper& operator=(const AgentPtrWrapper&);

			~AgentPtrWrapper();
	};

	class Agent {
		public:
			virtual GroupId groupId() const = 0;
			virtual void setGroupId(GroupId) = 0;
			virtual TypeId typeId() const = 0;
			virtual Agent* copy() const = 0;

			virtual AgentNode* node() = 0;
			virtual const AgentNode* node() const = 0;
			virtual void setNode(AgentNode*) = 0;

			virtual AgentGraph* graph() = 0;
			virtual const AgentGraph* graph() const = 0;
			virtual void setGraph(AgentGraph*) = 0;

			virtual AgentTask* task() = 0; 
			virtual const AgentTask* task() const = 0; 
			virtual void setTask(AgentTask* task) = 0;

			virtual void act() = 0;

			virtual ~Agent(){}
	};

	class AgentTask : public api::scheduler::NodeTask<AgentPtr> {
		public:
			virtual const Agent* agent() const = 0;
			virtual void setAgent(Agent*) = 0;
			virtual ~AgentTask(){}
	};

	class AgentGroup {
		public:
			virtual GroupId groupId() const = 0;

			virtual void add(Agent*) = 0;
			virtual void remove(Agent*) = 0;
			virtual api::scheduler::Job& job() = 0;
			virtual const api::scheduler::Job& job() const = 0;

			virtual ~AgentGroup(){}
	};

	class Model {
		public:
			typedef api::graph::parallel::DistributedGraph<AgentPtr> AgentGraph;

			virtual AgentGraph& graph() = 0;
			virtual api::scheduler::Scheduler& scheduler() = 0;
			virtual api::runtime::Runtime& runtime() = 0;

			virtual const api::scheduler::Job& loadBalancingJob() const = 0;

			virtual AgentGroup& buildGroup() = 0;
			virtual AgentGroup& getGroup(GroupId) = 0;
			virtual const std::unordered_map<GroupId, AgentGroup*>& groups() const = 0;

			virtual ~Model(){}
	};
}
#endif
