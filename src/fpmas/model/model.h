#ifndef MODEL_H
#define MODEL_H
#include "fpmas/api/model/model.h"
#include "fpmas/api/graph/distributed_graph.h"
#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/scheduler/scheduler.h"
#include "fpmas/runtime/runtime.h"
#include "fpmas/load_balancing/zoltan_load_balancing.h"
#include "fpmas/load_balancing/scheduled_load_balancing.h"

#include <random>

namespace fpmas { namespace model {

	using api::model::AgentNode;
	using api::model::AgentPtr;
	using api::scheduler::JID;

	/**
	 * Helper class to represent an Agent as the neighbor of an other.
	 *
	 * A Neighbor can be used through pointer like accesses, and the input
	 * Agent is automatically downcast to AgentType.
	 *
	 * @tparam AgentType Type of the input agent
	 */
	template<typename AgentType>
		class Neighbor {
			private:
				AgentPtr* agent;

			public:
				/**
				 * Neighbor constructor.
				 *
				 * @param agent pointer to a generic AgentPtr
				 */
				Neighbor(AgentPtr* agent)
					: agent(agent) {}

				/**
				 * Implicit conversion operator to `AgentType*`.
				 */
				operator AgentType*() const {
					return static_cast<AgentType*>(agent->get());
				}

				/**
				 * Member of pointer access operator.
				 *
				 * @return pointer to neighbor agent
				 */
				AgentType* operator->() const {
					return static_cast<AgentType*>(agent->get());
				}

				/**
				 * Indirection operator.
				 *
				 * @return reference to neighbor agent
				 */
				AgentType& operator*() const {
					return *static_cast<AgentType*>(agent->get());
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
	class AgentBase : public api::model::Agent {
		public:
			static const api::model::TypeId TYPE_ID;

		private:
			api::model::GroupId group_id;
			api::model::AgentGroup* _group;
			api::model::AgentTask* _task;
			api::model::AgentNode* _node;
			api::model::AgentGraph* _graph;
		public:
			api::model::GroupId groupId() const override {return group_id;}
			void setGroupId(api::model::GroupId group_id) override {this->group_id = group_id;}

			api::model::AgentGroup* group() override {return _group;}
			const api::model::AgentGroup* group() const override {return _group;}
			void setGroup(api::model::AgentGroup* group) override {this->_group = group;}

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
				std::vector<Neighbor<NeighborAgentType>> out;
				for(auto node : node()->outNeighbors()) {
					if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(node->data().get())) {
						out.push_back(&node->data());
					}
				}
				return out;
			}

			template<typename NeighborAgentType> Neighbors<NeighborAgentType> inNeighbors() const {
				std::vector<Neighbor<NeighborAgentType>> in;
				for(auto node : node()->inNeighbors()) {
					if(NeighborAgentType* neighbor = dynamic_cast<NeighborAgentType*>(node->data().get())) {
						in.push_back(&node->data());
					}
				}
				return in;
			}

			virtual ~AgentBase() {}
	};
	template<typename AgentType>
		const api::model::TypeId AgentBase<AgentType>::TYPE_ID = typeid(AgentType);

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
		private:
			api::model::AgentGraph& agent_graph;
		public:
			SynchronizeGraphTask(api::model::AgentGraph& agent_graph)
				: agent_graph(agent_graph) {}

			void run() override {
				agent_graph.synchronize();
			}
	};

	class EraseAgentCallback;
	class AgentGroup : public api::model::AgentGroup {
		friend EraseAgentCallback;
		public:
			typedef api::model::GroupId GroupId;
		private:
			GroupId id;
			api::model::AgentGraph& agent_graph;
			scheduler::Job _job;
			SynchronizeGraphTask sync_graph_task;
			std::vector<api::model::AgentPtr*> _agents;

		public:
			AgentGroup(GroupId group_id, api::model::AgentGraph& agent_graph, JID job_id);

			GroupId groupId() const override {return id;}

			void add(api::model::Agent*) override;
			void remove(api::model::Agent*) override;

			void insert(api::model::AgentPtr*) override;
			void erase(api::model::AgentPtr*) override;

			scheduler::Job& job() override {return _job;}
			const scheduler::Job& job() const override {return _job;}
			std::vector<api::model::AgentPtr*> agents() const override {return _agents;}

			template<typename AgentType>
				std::vector<AgentType*> agents() const {
					std::vector<AgentType*> out;
					for(auto agent : _agents)
						if(AgentType* agent_ptr = dynamic_cast<AgentType*>(agent))
							out.push_back(agent_ptr);

					return out;
				}
	};

	class LoadBalancingTask : public api::scheduler::Task {
		public:
			typedef api::load_balancing::LoadBalancing<AgentPtr>
				LoadBalancingAlgorithm;
			typedef api::load_balancing::NodeMap<AgentPtr> NodeMap;
			typedef typename api::load_balancing::PartitionMap PartitionMap;

		private:
			api::model::AgentGraph& agent_graph;
			LoadBalancingAlgorithm& load_balancing;
			api::scheduler::Scheduler& scheduler;
			api::runtime::Runtime& runtime;

		public:
			LoadBalancingTask(
					api::model::AgentGraph& agent_graph,
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
			api::model::AgentGraph& _graph;
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
				api::model::AgentGraph& graph,
				api::scheduler::Scheduler& scheduler,
				api::runtime::Runtime& runtime,
				LoadBalancingAlgorithm& load_balancing);
			Model(const Model&) = delete;
			Model(Model&&) = delete;
			Model& operator=(const Model&) = delete;
			Model& operator=(Model&&) = delete;

			api::model::AgentGraph& graph() override {return _graph;}
			api::scheduler::Scheduler& scheduler() override {return _scheduler;}
			api::runtime::Runtime& runtime() override {return _runtime;}

			const scheduler::Job& loadBalancingJob() const override {return _loadBalancingJob;}

			AgentGroup& buildGroup() override;
			AgentGroup& getGroup(GroupId) const override;
			const std::unordered_map<GroupId, api::model::AgentGroup*>& groups() const override {return _groups;}

			~Model();
	};


	template<template<typename> class SyncMode>
		using AgentGraph = graph::DistributedGraph<
		AgentPtr, SyncMode,
		graph::DistributedNode,
		graph::DistributedEdge,
		api::communication::MpiSetUp<communication::MpiCommunicator, communication::TypedMpi>,
		graph::LocationManager>;

	using AgentGraphApi = api::graph::DistributedGraph<AgentPtr>;

	typedef load_balancing::ZoltanLoadBalancing<AgentPtr> ZoltanLoadBalancing;
	typedef load_balancing::ScheduledLoadBalancing<AgentPtr> ScheduledLoadBalancing;
	typedef api::utils::Callback<AgentNode*> AgentNodeCallback;

	template<template<typename> class SyncMode>
	class DefaultModelConfig {
		protected:
			model::AgentGraph<SyncMode> __graph;
			scheduler::Scheduler __scheduler;
			runtime::Runtime __runtime {__scheduler};
			model::ZoltanLoadBalancing __zoltan_lb {__graph.getMpiCommunicator().getMpiComm()};
			model::ScheduledLoadBalancing __load_balancing {__zoltan_lb, __scheduler, __runtime};
	};

	template<template<typename> class SyncMode>
	class DefaultModel : private DefaultModelConfig<SyncMode>, public Model {
		public:
			DefaultModel()
				: Model(this->__graph, this->__scheduler, this->__runtime, this->__load_balancing) {}
	};
}}
#endif
