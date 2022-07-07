#ifndef FPMAS_SPATIAL_GRAPH_BUILDER_H
#define FPMAS_SPATIAL_GRAPH_BUILDER_H

/** \file src/fpmas/model/spatial/graph_builder.h
 * Defines classes to build spatial environments with a graph shape, based on
 * src/fpmas/graph/graph_builder.h.
 */

#include "fpmas/api/model/model.h"
#include "fpmas/api/model/spatial/spatial_model.h"
#include "fpmas/model/model.h"

namespace fpmas { namespace model {

	/**
	 * FPMAS reserved api::model::GroupId used by the SpatialGraphBuilder.
	 */
	static const api::model::GroupId GRAPH_CELL_GROUP_ID = -2;

	template<typename CellType>
		class SpatialGraphBuilder
		: public api::model::CellNetworkBuilder<CellType>{
			private:
				fpmas::api::graph::DistributedGraphBuilder<fpmas::model::AgentPtr>&
					graph_builder;
				std::size_t cell_count;

				std::function<CellType*()> cell_allocator;
				std::function<CellType*()> distant_cell_allocator;

			public:
				SpatialGraphBuilder(
						api::graph::DistributedGraphBuilder<model::AgentPtr>& graph_builder,
						std::size_t cell_count,
						std::function<CellType*()> cell_allocator,
						std::function<CellType*()> distant_cell_allocator) :
					graph_builder(graph_builder), cell_count(cell_count),
					cell_allocator(cell_allocator),
					distant_cell_allocator(distant_cell_allocator) {
					}

				SpatialGraphBuilder(
						api::graph::DistributedGraphBuilder<model::AgentPtr>& graph_builder,
						std::size_t cell_count,
						std::function<CellType*()> cell_allocator)
					: SpatialGraphBuilder(
							graph_builder, cell_count, cell_allocator, cell_allocator) {
					}

				SpatialGraphBuilder(
						api::graph::DistributedGraphBuilder<model::AgentPtr>& graph_builder,
						std::size_t cell_count)
					: SpatialGraphBuilder(graph_builder, cell_count, [] () {return new CellType;}) {
					}

				std::vector<CellType*> build(
						api::model::SpatialModel<CellType>& model) const override {
					return this->build(model, {});
				}

				std::vector<CellType*> build(
						api::model::SpatialModel<CellType>& model,
						api::model::GroupList groups) const override;
		};

	template<typename CellType>
		std::vector<CellType*> SpatialGraphBuilder<CellType>::build(
				api::model::SpatialModel<CellType>& model,
				api::model::GroupList groups) const {
			groups.push_back(model.cellGroup());
			model::DistributedAgentNodeBuilder node_builder(
					groups, cell_count, cell_allocator, distant_cell_allocator,
					model.getMpiCommunicator()
					);

			graph_builder.build(
					node_builder, api::model::CELL_SUCCESSOR, model.graph()
					);

			return model.cells();
		}
}}

#endif
