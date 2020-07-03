#ifndef MODEL_H
#define MODEL_H
#include "fpmas/api/model/model.h"
#include "fpmas/api/graph/parallel/distributed_graph.h"
#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/graph/parallel/distributed_graph.h"
#include "fpmas/scheduler/scheduler.h"
#include "fpmas/runtime/runtime.h"
#include "fpmas/load_balancing/zoltan_load_balancing.h"
#include "fpmas/load_balancing/scheduled_load_balancing.h"

#include <random>

namespace fpmas::model {

	using api::model::AgentNode;
	using api::model::AgentPtr;

	template<typename AgentType>
		class Neighbors {
			private:
				typedef typename std::vector<AgentType*>::iterator iterator;
				typedef typename std::vector<AgentType*>::const_iterator const_iterator;
				std::vector<AgentType*> neighbors;
				static std::mt19937 rd;

			public:
				Neighbors(const std::vector<AgentType*>& neighbors)
					: neighbors(neighbors) {}

				operator std::vector<AgentType*>() {return neighbors;}

				iterator begin() {
					return neighbors.begin();
				}
				const_iterator begin() const {
					return neighbors.begin();
				}

				iterator end() {
					return neighbors.end();
				}
				const_iterator end() const {
					return neighbors.end();
				}

				Neighbors& shuffle() {
					std::shuffle(neighbors.begin(), neighbors.end(), rd);
					return *this;
				}
		};
	template<typename AgentType> std::mt19937 Neighbors<AgentType>::rd;

	template<typename AgentType, api::model::TypeId _TYPE_ID>
	class AgentBase : public api::model::Agent {
		public:
			inline static const api::model::TypeId TYPE_ID = _TYPE_ID;

		private:
			api::model::AgentTask* _task;
			api::model::AgentNode* _node;
			api::model::AgentGraph* _graph;
			api::model::GroupId group_id;
		public:
			api::model::GroupId groupId() const override {return group_id;}
			void setGroupId(api::model::GroupId group_id) override {this->group_id = group_id;}

			api::model::TypeId typeId() const override {return TYPE_ID;}
			api::model::Agent* copy() const override {return new AgentType(*static_cast<const AgentType*>(this));}

			api::model::AgentNode* node() override {return _node;}
			const api::model::AgentNode* node() const override {return _node;}
			void setNode(api::model::AgentNode* node) override {_node = node;}

			api::model::AgentGraph* graph() override {return _graph;}
			const api::model::AgentGraph* graph() const override {return _graph;}
			void setGraph(api::model::AgentGraph* graph) override {_graph = graph;}

			api::model::AgentTask* task() override {return _task;}
			const api::model::AgentTask* task() const override {return _task;}
			void setTask(api::model::AgentTask* task) override {_task = task;}

			template<typename NeighborAgentType> Neighbors<NeighborAgentType> outNeighbors() const {
				std::vector<NeighborAgentType*> out;
				for(auto node : node()->outNeighbors()) {
					if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(node->data().get())) {
						out.push_back(neighbor);
					}
				}
				return out;
			}

			template<typename NeighborAgentType> Neighbors<NeighborAgentType> inNeighbors() const {
				std::vector<NeighborAgentType*> in;
				for(auto node : node()->inNeighbors()) {
					if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(node->data().get())) {
						in.push_back(neighbor);
					}
				}
				return in;
			}

			virtual ~AgentBase() {}
	};

	class AgentTask : public api::model::AgentTask {
		private:
			api::model::Agent* _agent;
		public:
			const api::model::Agent* agent() const override {return _agent;}
			void setAgent(api::model::Agent* agent) override {_agent=agent;}

			AgentNode* node() override
				{return _agent->node();}

			void run() override {
				_agent->act();
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

			GroupId groupId() const override {return id;}

			void add(api::model::Agent*) override;
			void remove(api::model::Agent*) override;
			scheduler::Job& job() override {return _job;}
			const scheduler::Job& job() const override {return _job;}
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

	class InsertAgentCallback 
		: public api::utils::Callback<AgentNode*> {
			  private:
				  api::model::Model& model;
			  public:
				  InsertAgentCallback(api::model::Model& model) : model(model) {}

				  void call(AgentNode* node) override;
		  };

	class EraseAgentCallback 
		: public api::utils::Callback<AgentNode*> {
			private:
				api::model::Model& model;
			public:
				EraseAgentCallback(api::model::Model& model) : model(model) {}

				void call(AgentNode* node) override;
		};

	class SetAgentLocalCallback
		: public api::utils::Callback<AgentNode*> {
			private:
				api::model::Model& model;
			public:
				SetAgentLocalCallback(api::model::Model& model) : model(model) {}

				void call(AgentNode* node) override;
		};

	class SetAgentDistantCallback
		: public api::utils::Callback<AgentNode*> {
			private:
				api::model::Model& model;
			public:
				SetAgentDistantCallback(api::model::Model& model) : model(model) {}

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
			InsertAgentCallback* insert_node_callback = new InsertAgentCallback(*this);
			EraseAgentCallback* erase_node_callback = new EraseAgentCallback(*this);
			SetAgentLocalCallback* set_local_callback = new SetAgentLocalCallback(*this);
			SetAgentDistantCallback* set_distant_callback = new SetAgentDistantCallback(*this);

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

	using AgentGraphApi = api::graph::parallel::DistributedGraph<AgentPtr>;

	typedef load_balancing::ZoltanLoadBalancing<AgentPtr> ZoltanLoadBalancing;
	typedef load_balancing::ScheduledLoadBalancing<AgentPtr> ScheduledLoadBalancing;
	typedef api::utils::Callback<AgentNode*> AgentNodeCallback;
}
#endif
