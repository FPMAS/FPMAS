#ifndef MODEL_H
#define MODEL_H
#include "api/model/model.h"
#include "api/graph/parallel/distributed_graph.h"
#include "api/utils/ptr_wrapper.h"
#include "graph/parallel/distributed_graph.h"
#include "scheduler/scheduler.h"
#include "runtime/runtime.h"
#include "load_balancing/zoltan_load_balancing.h"
#include "load_balancing/scheduled_load_balancing.h"

namespace FPMAS::model {

	using api::model::AgentNode;
	using api::model::AgentPtr;

	template<api::model::TypeId _TYPE_ID>
	class AgentBase : public api::model::Agent {
		public:
			inline static const api::model::TypeId TYPE_ID = _TYPE_ID;

		private:
			api::model::AgentTask* _task;
			api::model::AgentNode* _node;
			api::model::GroupId group_id;
		public:
			api::model::GroupId groupId() const override {return group_id;}
			void setGroupId(api::model::GroupId group_id) override {this->group_id = group_id;}

			api::model::TypeId typeId() const override {return TYPE_ID;}

			api::model::AgentNode* node() override {return _node;}
			const api::model::AgentNode* node() const override {return _node;}
			void setNode(api::model::AgentNode* node) override {_node = node;}

			api::model::AgentTask* task() override {return _task;}
			const api::model::AgentTask* task() const override {return _task;}
			void setTask(api::model::AgentTask* task) override {_task = task;}

			virtual ~AgentBase() {}
	};

	class AgentTask : public api::model::AgentTask {
		private:
			api::model::Agent& _agent;
		public:
			AgentTask(api::model::Agent& agent)
				: _agent(agent) {}

			const api::model::Agent& agent() const override {return _agent;}

			AgentNode* node() override
				{return _agent.node();}


			void run() override {
				_agent.act();
			}
	};

	class SynchronizeGraphTask : public api::scheduler::Task {
		public:
			typedef typename api::model::Model::AgentGraph AgentGraph;
		private:
			AgentGraph& agent_graph;
		public:
			SynchronizeGraphTask(AgentGraph& agent_graph)
				: agent_graph(agent_graph) {}

			void run() override {
				agent_graph.synchronize();
			}
	};

	class AgentGroup : public api::model::AgentGroup {
		public:
			typedef api::model::GroupId GroupId;
			typedef typename api::model::Model::AgentGraph AgentGraph;
		private:
			GroupId id;
			AgentGraph& agent_graph;
			scheduler::Job _job;
			SynchronizeGraphTask sync_graph_task;

		public:
			AgentGroup(GroupId group_id, AgentGraph& agent_graph, JID job_id);
			//AgentGroup(const AgentGroup&) = delete;
			//AgentGroup(AgentGroup&&) = delete;
			//AgentGroup& operator=(const AgentGroup&) = delete;
			//AgentGroup& operator=(AgentGroup&&) = delete;

			GroupId groupId() const override {return id;}

			void add(api::model::Agent*) override;
			void remove(api::model::Agent*) override;
			scheduler::Job& job() override {return _job;}
			const scheduler::Job& job() const override {return _job;}

			//~AgentGroup();
	};

	class LoadBalancingTask : public api::scheduler::Task {
		public:
			typedef typename api::model::Model::AgentGraph AgentGraph;
			typedef api::load_balancing::LoadBalancing<AgentPtr>
				LoadBalancingAlgorithm;
			typedef typename LoadBalancingAlgorithm::ConstNodeMap ConstNodeMap;
			typedef typename LoadBalancingAlgorithm::PartitionMap PartitionMap;

		private:
			AgentGraph& agent_graph;
			LoadBalancingAlgorithm& load_balancing;
			api::scheduler::Scheduler& scheduler;
			api::runtime::Runtime& runtime;

		public:
			LoadBalancingTask(
					AgentGraph& agent_graph,
					LoadBalancingAlgorithm& load_balancing,
					api::scheduler::Scheduler& scheduler,
					api::runtime::Runtime& runtime
					) : agent_graph(agent_graph), load_balancing(load_balancing), scheduler(scheduler), runtime(runtime) {}

			void run() override;
	};

	class InsertNodeCallback 
		: public api::utils::Callback<AgentNode*> {
			  private:
				  api::model::Model& model;
			  public:
				  InsertNodeCallback(api::model::Model& model) : model(model) {}

				  void call(AgentNode* node) override;
		  };

	class EraseNodeCallback 
		: public api::utils::Callback<AgentNode*> {
			private:
				api::model::Model& model;
			public:
				EraseNodeCallback(api::model::Model& model) : model(model) {}

				void call(AgentNode* node) override;
		};

	class SetLocalNodeCallback
		: public api::utils::Callback<AgentNode*> {
			private:
				api::model::Model& model;
			public:
				SetLocalNodeCallback(api::model::Model& model) : model(model) {}

				void call(AgentNode* node) override;
		};

	class SetDistantNodeCallback
		: public api::utils::Callback<AgentNode*> {
			private:
				api::model::Model& model;
			public:
				SetDistantNodeCallback(api::model::Model& model) : model(model) {}

				void call(AgentNode* node) override;
		};

	class Model : public api::model::Model {
		public:
			typedef api::model::GroupId GroupId;
			typedef typename LoadBalancingTask::LoadBalancingAlgorithm LoadBalancingAlgorithm;
		private:
			GroupId gid;
			AgentGraph& _graph;
			api::scheduler::Scheduler& _scheduler;
			api::runtime::Runtime& _runtime;
			scheduler::Job _loadBalancingJob;
			LoadBalancingTask load_balancing_task;
			InsertNodeCallback* insert_node_callback = new InsertNodeCallback(*this);
			EraseNodeCallback* erase_node_callback = new EraseNodeCallback(*this);
			SetLocalNodeCallback* set_local_callback = new SetLocalNodeCallback(*this);
			SetDistantNodeCallback* set_distant_callback = new SetDistantNodeCallback(*this);

			std::unordered_map<GroupId, api::model::AgentGroup*> _groups;
			
			JID job_id = LB_JID + 1;
			

		public:
			static const JID LB_JID;
			Model(
				AgentGraph& graph,
				api::scheduler::Scheduler& scheduler,
				api::runtime::Runtime& runtime,
				LoadBalancingAlgorithm& load_balancing);
			Model(const Model&) = delete;
			Model(Model&&) = delete;
			Model& operator=(const Model&) = delete;
			Model& operator=(Model&&) = delete;

			AgentGraph& graph() override {return _graph;}
			api::scheduler::Scheduler& scheduler() override {return _scheduler;}
			api::runtime::Runtime& runtime() override {return _runtime;}

			const scheduler::Job& loadBalancingJob() const override {return _loadBalancingJob;}

			api::model::AgentGroup& buildGroup() override;
			api::model::AgentGroup& getGroup(GroupId) override;
			const std::unordered_map<GroupId, api::model::AgentGroup*>& groups() const override {return _groups;}

			~Model();
	};


	template<typename SyncMode>
		using AgentGraph = graph::parallel::DistributedGraph<
		AgentPtr, SyncMode,
		graph::parallel::DistributedNode,
		graph::parallel::DistributedArc,
		api::communication::MpiSetUp<communication::MpiCommunicator, communication::TypedMpi>,
		graph::parallel::LocationManager>;

	typedef load_balancing::ZoltanLoadBalancing<AgentPtr> ZoltanLoadBalancing;
	typedef load_balancing::ScheduledLoadBalancing<AgentPtr> ScheduledLoadBalancing;
	typedef api::utils::Callback<AgentNode*> AgentNodeCallback;
}
#endif
