#include "model.h"

namespace fpmas { namespace model {
	
	bool is_agent_in_group(api::model::Agent* agent, api::model::GroupId group_id) {
		std::vector<fpmas::api::model::GroupId> group_ids = agent->groupIds();
		auto result = std::find(group_ids.begin(), group_ids.end(), group_id);
		return result != group_ids.end();
	}

	bool is_agent_in_group(api::model::Agent* agent, api::model::AgentGroup& group) {
		return is_agent_in_group(agent, group.groupId());
	}

	random::DistributedGenerator<> RandomNeighbors::rd;

	void RandomNeighbors::seed(random::DistributedGenerator<>::result_type seed) {
		RandomNeighbors::rd.seed(seed);
	}

	api::model::AgentNode*
		AgentNodeBuilder::buildNode(api::graph::DistributedGraph<AgentPtr>&) {
			auto* agent = agents.back();
			agents.pop_back();

			// Implicitly builds and insert agent node into the graph
			group.add(agent);

			return agent->node();
		}

	DistributedAgentNodeBuilder::DistributedAgentNodeBuilder(
			const api::model::GroupList& groups,
			std::size_t agent_count,
			std::function<api::model::Agent*()> allocator,
			api::communication::MpiCommunicator& comm
			) :
		graph::DistributedNodeBuilder<AgentPtr>(agent_count, comm),
		allocator(allocator),
		distant_allocator(allocator),
		groups(groups) {
	}

	DistributedAgentNodeBuilder::DistributedAgentNodeBuilder(
			const api::model::GroupList& groups,
			std::size_t agent_count,
			std::function<api::model::Agent*()> allocator,
			std::function<api::model::Agent*()> distant_allocator,
			api::communication::MpiCommunicator& comm
			) :
		graph::DistributedNodeBuilder<AgentPtr>(agent_count, comm),
		allocator(allocator),
		distant_allocator(distant_allocator),
		groups(groups) {
	}

	api::model::AgentNode* DistributedAgentNodeBuilder::buildNode(api::graph::DistributedGraph<AgentPtr> &) {
		auto* agent = allocator();

		// Implicitly builds and insert agent node into the graph
		for(auto& group : groups)
			group.get().add(agent);

		local_node_count--;
		return agent->node();
	}

	api::model::AgentNode* DistributedAgentNodeBuilder::buildDistantNode(
			api::graph::DistributedId id, int location,
			api::graph::DistributedGraph<AgentPtr> & graph) {
		auto* agent = distant_allocator();
		// Registers the id of the group to which the agent should be added
		for(auto& group : groups)
			agent->addGroupId(group.get().groupId());

		fpmas::model::AgentNode* node
			= new fpmas::graph::DistributedNode<AgentPtr>(id, agent);
		// No insertion is performed if a distant node with id was already
		// contained in the graph. In this case, `agent` is ignored and deleted,
		// and the agent already contained in the graph is assumed to have been
		// built by this DistributedAgentNodeBuilder and so to be already
		// contained in `group`.
		//
		// If the node was not contained in the graph, the agent is
		// automatically inserted into the group when InsertAgentNodeCallback is
		// triggered
		node = graph.insertDistant(node);

		// In any case, the location of the node is updated, even if it was
		// already contained in the graph
		node->setLocation(location);

		return node;
	}
}}

namespace nlohmann {
	void adl_serializer<fpmas::api::model::Model>::to_json(json& j, const fpmas::api::model::Model& model) {
		j["graph"] = model.graph();
		j["runtime"] = model.runtime().currentDate();
	}

	void adl_serializer<fpmas::api::model::Model>::from_json(const json& j, fpmas::api::model::Model& model) {
		json::json_serializer<fpmas::api::model::AgentGraph, void>
			::from_json(j["graph"], model.graph());
		fpmas::api::scheduler::Date current_date
			= j["runtime"].get<fpmas::api::scheduler::Date>();
		model.runtime().setCurrentDate(++current_date);
	}
}
