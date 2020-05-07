#ifndef MOCK_DISTRIBUTED_GRAPH_H
#define MOCK_DISTRIBUTED_GRAPH_H
#include "gmock/gmock.h"

#include "../base/mock_graph_base.h"
#include "api/graph/parallel/distributed_graph.h"

template<typename DistNode, typename DistArc>
class MockDistributedGraph :
	public MockGraphBase<DistNode, DistArc>,
	public FPMAS::api::graph::parallel::DistributedGraph<DistNode, DistArc> {

		typedef FPMAS::api::graph::parallel::DistributedGraph<DistNode, DistArc>
			graph_base;
		public:
		using typename graph_base::node_type;
		using typename graph_base::node_map;
		using typename graph_base::arc_type;
		using typename graph_base::arc_map;
		using typename graph_base::partition_type;

		MOCK_METHOD(
				const FPMAS::api::graph::parallel::LocationManager<DistNode>&,
				getLocationManager, (), (const, override));

		MOCK_METHOD(node_type*, importNode, (const node_type&), (override));
		MOCK_METHOD(arc_type*, importArc, (const arc_type&), (override));
		MOCK_METHOD(void, clear, (arc_type*), (override));
		MOCK_METHOD(void, clear, (node_type*), (override));

		MOCK_METHOD(void, balance, (), (override));
		MOCK_METHOD(void, distribute, (partition_type), (override));
		MOCK_METHOD(void, synchronize, (), (override));
	};

#endif
