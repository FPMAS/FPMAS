#ifndef FPMAS_MODEL_H
#define FPMAS_MODEL_H

/** \file src/fpmas/model/model.h
 * Model implementation.
 */

#include "fpmas/api/model/model.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/runtime/runtime.h"
#include "fpmas/graph/zoltan_load_balancing.h"
#include "fpmas/graph/scheduled_load_balancing.h"

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
					return *static_cast<AgentType*>(agent->get()); }

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
			api::model::Model* _model;
		public:
			api::model::GroupId groupId() const override {return group_id;}
			void setGroupId(api::model::GroupId id) override {this->group_id = id;}

			api::model::AgentGroup* group() override {return _group;}
			const api::model::AgentGroup* group() const override {return _group;}
			void setGroup(api::model::AgentGroup* group) override {this->_group = group;}

			api::model::TypeId typeId() const override {return TYPE_ID;}
			api::model::Agent* copy() const override {return new AgentType(*static_cast<const AgentType*>(this));}

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
			 * @see fpmas::api::graph::node::outNeighbors()
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
			 * @see fpmas::api::graph::node::outNeighbors()
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
	 * api::model::AgentTask implementation.
	 */
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

	/**
	 * Graph synchronization task.
	 *
	 * This task is set as the end task of each \AgentGroup's job.
	 *
	 * Concretely, this means that the simulation Graph is synchronized at the
	 * end of each \AgentGroup execution.
	 */
	class SynchronizeGraphTask : public api::scheduler::Task {
		private:
			api::model::AgentGraph& agent_graph;
		public:
			/**
			 * SynchronizeGraphTask constructor.
			 *
			 * @param agent_graph Agent graph to synchronize
			 */
			SynchronizeGraphTask(api::model::AgentGraph& agent_graph)
				: agent_graph(agent_graph) {}

			/**
			 * Calls api::model::AgentGraph::synchronize().
			 */
			void run() override {
				agent_graph.synchronize();
			}
	};

	class EraseAgentCallback;

	/**
	 * api::model::AgentGroup implementation.
	 */
	class AgentGroup : public api::model::AgentGroup {
		friend EraseAgentCallback;
		private:
		api::model::GroupId id;
		api::model::AgentGraph& agent_graph;
		scheduler::Job _job;
		SynchronizeGraphTask sync_graph_task;
		std::vector<api::model::AgentPtr*> _agents;

		public:
		/**
		 * AgentGroup constructor.
		 *
		 * @param group_id unique id of the group
		 * @param agent_graph associated agent graph
		 * @param job_id unique id of the associated job
		 */
		AgentGroup(api::model::GroupId group_id, api::model::AgentGraph& agent_graph, JID job_id);

		api::model::GroupId groupId() const override {return id;}

		void add(api::model::Agent*) override;
		void remove(api::model::Agent*) override;

		void insert(api::model::AgentPtr*) override;
		void erase(api::model::AgentPtr*) override;

		scheduler::Job& job() override {return _job;}
		const scheduler::Job& job() const override {return _job;}
		std::vector<api::model::AgentPtr*> agents() const override {return _agents;}
	};

	/**
	 * Load balancing task.
	 *
	 * This task is actually the unique task of the \Job defined by
	 * Model::loadBalancingJob().
	 *
	 * @see api::model::AgentGraph::balance()
	 */
	class LoadBalancingTask : public api::scheduler::Task {
		public:
			/**
			 * LoadBalancingAlgorithm type.
			 */
			typedef api::graph::LoadBalancing<AgentPtr>
				LoadBalancingAlgorithm;
			/**
			 * Agent node map.
			 */
			typedef api::graph::NodeMap<AgentPtr> NodeMap;
			/**
			 * Partition map.
			 */
			typedef typename api::graph::PartitionMap PartitionMap;

		private:
			api::model::AgentGraph& agent_graph;
			LoadBalancingAlgorithm& load_balancing;

		public:
			/**
			 * LoadBalancingTask constructor.
			 *
			 * @param agent_graph associated agent graph on which load
			 * balancing will be performed
			 * @param load_balancing load balancing algorithme used to compute
			 * a balanced partition
			 */
			LoadBalancingTask(
					api::model::AgentGraph& agent_graph,
					LoadBalancingAlgorithm& load_balancing
					) : agent_graph(agent_graph), load_balancing(load_balancing) {}

			void run() override;
	};

	/**
	 * Callback triggered when an \AgentNode is _inserted_ into the simulation
	 * graph (i.e. when an \AgentNode is created, or when it's imported from an
	 * other process).
	 */
	class InsertAgentNodeCallback 
		: public api::utils::Callback<AgentNode*> {
			  private:
				  api::model::Model& model;
			  public:
				  /**
				   * InsertAgentNodeCallback constructor.
				   *
				   * @param model current model
				   */
				  InsertAgentNodeCallback(api::model::Model& model) : model(model) {}

				  /**
				   * When called, the argument is assumed to contain a new
				   * uninitialized \Agent (accessible through node->data()).
				   * The \Agent GroupId is also assumed to be initialized.
				   *
				   * Then this \Agent is bound to :
				   * - its corresponding group (api::model::Agent::setGroup(),
				   *   api::model::AgentGroup::insert()). The group is
				   *   retrieved from the current model using
				   *   api::model::Model::getGroup().
				   * - the argument node (api::model::Agent::setNode)
				   * - the current simulation graph
				   *   (api::model::Agent::setGraph())
				   * - a new AgentTask (api::model::Agent::setTask())
				   *
				   * All the corresponding fields are valid once this callback
				   * has been triggered.
				   *
				   * @param node \AgentNode inserted in the graph
				   */
				  void call(api::model::AgentNode* node) override;
		  };

	/**
	 * Callback triggered when an \AgentNode is _erased_ from the simulation
	 * graph (i.e. when an \AgentNode is created, or when its imported from an
	 * other process).
	 */
	class EraseAgentNodeCallback 
		: public api::utils::Callback<AgentNode*> {
			private:
				api::model::Model& model;
			public:
				/**
				 * EraseAgentNodeCallback constructor.
				 *
				 * @param model current model
				 */
				EraseAgentNodeCallback(api::model::Model& model) : model(model) {}

				/**
				 * Removes the node's \Agent (i.e. node->data()) from its
				 * \AgentGroup.
				 *
				 * If the node was still \LOCAL, the agent's task is also
				 * unscheduled.
				 *
				 * @param node \AgentNode to erase from the graph
				 */
				void call(api::model::AgentNode* node) override;
		};

	/**
	 * Callback triggered when an \AgentNode is set \LOCAL.
	 *
	 * This happens when a previously \DISTANT node is imported into the local
	 * graph and becomes \LOCAL, or when a new \AgentNode is inserted in the
	 * graph (the node is implicitly set \LOCAL in this case).
	 */
	class SetAgentLocalCallback
		: public api::utils::Callback<AgentNode*> {
			private:
				api::model::Model& model;
			public:
				/**
				 * SetAgentLocalCallback constructor.
				 *
				 * @param model current model
				 */
				SetAgentLocalCallback(api::model::Model& model) : model(model) {}

				/**
				 * Schedules the associated agent's task to be executed within
				 * its \AgentGroup's job.
				 *
				 * @param node \AgentNode to set \LOCAL
				 *
				 * @see \AgentTask
				 * @see fpmas::api::model::Agent::task()
				 * @see fpmas::api::model::AgentGroup::job()
				 */
				void call(api::model::AgentNode* node) override;
		};

	/**
	 * Callback triggered when an \AgentNode is set \DISTANT.
	 *
	 * This happens when a previously \LOCAL node is exported to an other
	 * process, or when a \DISTANT node is inserted into the graph upon edge
	 * import.
	 */
	class SetAgentDistantCallback
		: public api::utils::Callback<AgentNode*> {
			private:
				api::model::Model& model;
			public:
				/**
				 * SetAgentDistantCallback constructor.
				 *
				 * @param model current model
				 */
				SetAgentDistantCallback(api::model::Model& model) : model(model) {}

				/**
				 * Unschedules the associated agent's task.
				 *
				 * Notice that when a node goes \DISTANT, it is still contained
				 * in the simulation graph as any other node. However, it
				 * **must not** be executed, since only \LOCAL agents are
				 * assumed to be executed by the local process.
				 *
				 * @param node \AgentNode to set \DISTANT
				 *
				 * @see \AgentTask
				 * @see fpmas::api::model::Agent::task()
				 * @see fpmas::api::model::AgentGroup::job()
				 */
				void call(api::model::AgentNode* node) override;
		};

	/**
	 * api::model::Model implementation.
	 */
	class Model : public api::model::Model {
		public:
			/**
			 * LoadBalancingAlgorithm type.
			 */
			typedef typename LoadBalancingTask::LoadBalancingAlgorithm LoadBalancingAlgorithm;

		private:
			api::model::AgentGraph& _graph;
			api::scheduler::Scheduler& _scheduler;
			api::runtime::Runtime& _runtime;
			scheduler::Job _loadBalancingJob;
			LoadBalancingTask load_balancing_task;
			InsertAgentNodeCallback* insert_node_callback = new InsertAgentNodeCallback(*this);
			EraseAgentNodeCallback* erase_node_callback = new EraseAgentNodeCallback(*this);
			SetAgentLocalCallback* set_local_callback = new SetAgentLocalCallback(*this);
			SetAgentDistantCallback* set_distant_callback = new SetAgentDistantCallback(*this);

			std::unordered_map<api::model::GroupId, api::model::AgentGroup*> _groups;
			
			JID job_id = LB_JID + 1;
			

		public:
			/**
			 * JID used for the loadBalancingJob()
			 */
			static const JID LB_JID;
			/**
			 * Model constructor.
			 *
			 * @param graph simulation graph
			 * @param scheduler scheduler
			 * @param runtime runtime
			 * @param load_balancing load balancing algorithm
			 */
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

			AgentGroup& buildGroup(api::model::GroupId id) override;
			AgentGroup& getGroup(api::model::GroupId id) const override;
			const std::unordered_map<api::model::GroupId, api::model::AgentGroup*>& groups() const override {return _groups;}

			~Model();
	};


	/**
	 * Partial graph::DistributedGraph specialization used as the Agent
	 * simulation graph.
	 */
	template<template<typename> class SyncMode>
		using AgentGraph = graph::DistributedGraph<AgentPtr, SyncMode>;

	/**
	 * ZoltanLoadBalancing specialization.
	 */
	typedef graph::ZoltanLoadBalancing<AgentPtr> ZoltanLoadBalancing;
	/**
	 * ScheduledLoadBalancing specialization.
	 */
	typedef graph::ScheduledLoadBalancing<AgentPtr> ScheduledLoadBalancing;
	/**
	 * Callback specialization that can be extended to define user callbacks.
	 */
	typedef api::utils::Callback<AgentNode*> AgentNodeCallback;

	/**
	 * A default Model configuration that instantiate default components
	 * implementations.
	 */
	template<template<typename> class SyncMode>
	class DefaultModelConfig {
		protected:
			/**
			 * Default MpiCommunicator.
			 */
			communication::MpiCommunicator comm;
			/**
			 * Default AgentGraph.
			 */
			model::AgentGraph<SyncMode> __graph {comm};
			/**
			 * Default Scheduler.
			 */
			scheduler::Scheduler __scheduler;
			/**
			 * Default Runtime.
			 */
			runtime::Runtime __runtime {__scheduler};
			/**
			 * Default load balancing algorithm.
			 */
			model::ZoltanLoadBalancing __zoltan_lb {comm};
			/**
			 * Default scheduled load balancing algorithm.
			 */
			model::ScheduledLoadBalancing __load_balancing {__zoltan_lb, __scheduler, __runtime};
	};

	/**
	 * Model extension based on the DefaultModelConfig.
	 *
	 * It is recommended to use this helper class to instantiate a Model.
	 *
	 * @par Example
	 * ```cpp
	 * // main.cpp
	 * #include "fpmas.h"
	 *
	 * using namespace fpmas;
	 *
	 * class UserAgent1 : public model::AgentBase<UserAgent1> {
	 * 	public:
	 * 	void act() override {
	 * 		FPMAS_LOG_INFO(UserAgent1, "Execute agent %s",
	 * 		FPMAS_C_STR(this->node()->getId()))
	 * 	}
	 * };
	 *
	 * class UserAgent2 : public model::AgentBase<UserAgent2> {
	 * 	public:
	 * 	void act() override {
	 * 		FPMAS_LOG_INFO(UserAgent2, "Execute agent %s",
	 * 		FPMAS_C_STR(this->node()->getId()))
	 * 	}
	 * };
	 *
	 * #define USER_AGENTS UserAgent1, UserAgent2
	 *
	 * FPMAS_DEFAULT_JSON(UserAgent1)
	 * FPMAS_DEFAULT_JSON(UserAgent2)
	 *
	 * FPMAS_JSON_SET_UP(USER_AGENTS)
	 *
	 *
	 * int main(int argc, char** argv) {
	 * 	fpmas::init(argc, argv);
	 * 	FPMAS_REGISTER_AGENT_TYPES(USER_AGENTS);
	 *
	 * 	{
	 * 		model::DefaultModel model;
	 *
	 * 		model::AgentGroup& group_1 = model.buildGroup();
	 * 		group_1.add(new UserAgent1);
	 * 		group_1.add(new UserAgent2);
	 *
	 * 		model.scheduler().schedule(0, 1, group_1.job());
	 * 		model.scheduler().schedule(0, 50, model.loadBalancingJob());
	 *
	 * 		model.runtime().run(1000);
	 * 	}
	 * 	fpmas::finalize();
	 * }
	 * ```
	 */
	template<template<typename> class SyncMode>
	class DefaultModel : private DefaultModelConfig<SyncMode>, public Model {
		public:
			DefaultModel() :
				DefaultModelConfig<SyncMode>(),
				Model(this->__graph, this->__scheduler, this->__runtime, this->__load_balancing) {}

			/**
			 * Returns a reference to the internal
			 * communication::MpiCommunicator.
			 *
			 * @return reference to internal MPI communicator
			 */
			communication::MpiCommunicator& getMpiCommunicator() {
				return this->comm;
			}

			/**
			 * \copydoc getMpiCommunicator
			 */
			const communication::MpiCommunicator& getMpiCommunicator() const {
				return this->comm;
			}
	};
}}
#endif
