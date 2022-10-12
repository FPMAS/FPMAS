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
	 * FPMAS reserved api::model::GroupId used by the CellNetworkBuilder.
	 */
	static const api::model::GroupId GRAPH_CELL_GROUP_ID = -2;

	/**
	 * An fpmas::api::model::CellNetworkBuilder implementation.
	 *
	 * The algorithm takes as input a generic
	 * fpmas::api::graph::DistributedGraphBuilder instance, and builds a \Cell
	 * network from it with appropriate \AgentGroups and CELL_SUCCESSOR links.
	 *
	 * @tparam CellType type of cells to build
	 */
	template<typename CellType>
		class CellNetworkBuilder
		: public api::model::CellNetworkBuilder<CellType>{
			private:
				fpmas::api::graph::DistributedGraphBuilder<fpmas::model::AgentPtr>&
					graph_builder;
				std::size_t cell_count;

				std::function<CellType*()> cell_allocator;
				std::function<CellType*()> distant_cell_allocator;

			public:
				/**
				 * CellNetworkBuilder constructor.
				 *
				 * @param graph_builder DistributedGraphBuilder used to build
				 * the Cell graph
				 * @param cell_count size of the Cell network to build. The
				 * global network size should be provided on all processes,
				 * independently from the distribution.
				 * @param cell_allocator function used to dynamically allocate
				 * \LOCAL CellType instances
				 * @param distant_cell_allocator function used to dynamically
				 * allocate \DISTANT CellType instances
				 */
				CellNetworkBuilder(
						api::graph::DistributedGraphBuilder<model::AgentPtr>& graph_builder,
						std::size_t cell_count,
						std::function<CellType*()> cell_allocator,
						std::function<CellType*()> distant_cell_allocator) :
					graph_builder(graph_builder), cell_count(cell_count),
					cell_allocator(cell_allocator),
					distant_cell_allocator(distant_cell_allocator) {
					}

				/**
				 * CellNetworkBuilder constructor.
				 *
				 * @param graph_builder DistributedGraphBuilder used to build
				 * the Cell graph
				 * @param cell_count size of the Cell network to build. The
				 * global network size should be provided on all processes,
				 * independently from the distribution.
				 * @param cell_allocator function used to dynamically allocate
				 * \LOCAL and \DISTANT CellType instances
				 */
				CellNetworkBuilder(
						api::graph::DistributedGraphBuilder<model::AgentPtr>& graph_builder,
						std::size_t cell_count,
						std::function<CellType*()> cell_allocator)
					: CellNetworkBuilder(
							graph_builder, cell_count, cell_allocator, cell_allocator) {
					}

				/**
				 * CellNetworkBuilder constructor.
				 *
				 * \LOCAL and \DISTANT CellType instances are allocated using
				 * the `new CellType` instruction.
				 *
				 * @param graph_builder DistributedGraphBuilder used to build
				 * the Cell graph
				 * @param cell_count size of the Cell network to build. The
				 * global network size should be provided on all processes,
				 * independently from the distribution.
				 */
				CellNetworkBuilder(
						api::graph::DistributedGraphBuilder<model::AgentPtr>& graph_builder,
						std::size_t cell_count)
					: CellNetworkBuilder(graph_builder, cell_count, [] () {return new CellType;}) {
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
		std::vector<CellType*> CellNetworkBuilder<CellType>::build(
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
