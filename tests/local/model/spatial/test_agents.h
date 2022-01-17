#ifndef FPMAS_TEST_AGENTS_H
#define FPMAS_TEST_AGENTS_H

#include "../../../mocks/model/mock_model.h"
#include "../../../mocks/model/mock_spatial_model.h"
#include "fpmas/model/spatial/grid.h"
#include "fpmas/model/spatial/graph.h"
#include "../../../mocks/model/mock_grid.h"

namespace fpmas { namespace model {
	// Methods to compare default cells additional data.
	// Since there is no additional data for default cells, true is always
	// returned.
	bool operator==(const GridCell&, const GridCell&);
	bool operator==(const GraphCell&, const GraphCell&);
}}

namespace model { namespace test {
	// Default cells alias
	typedef fpmas::model::GridCell GridCell;
	typedef fpmas::model::GraphCell GraphCell;

	template<typename CellType>
	class CustomCell {
		protected:
			int data;

		public:
			CustomCell() : data(72) {};

			CustomCell(int data) : data(data) {
			}

			static void to_json(nlohmann::json& j, const CellType* cell) {
				j = cell->data;
			}
			static CellType* from_json(const nlohmann::json& j) {
				return new CellType(j.get<int>());
			}

			static std::size_t size(
					const fpmas::io::datapack::ObjectPack& pack,
					const CellType*) {
				return pack.size<int>();
			}

			static void to_datapack(
					fpmas::io::datapack::ObjectPack& pack,
					const CellType* cell) {
				pack.put(cell->data);
			}
			static CellType* from_datapack(
					const fpmas::io::datapack::ObjectPack& pack) {
				return new CellType(pack.get<int>());
			}

			bool operator==(const CellType& cell) const {
				return data == cell.data;
			}
	};

	class CustomGridCell :
		public CustomCell<CustomGridCell>,
		public fpmas::model::GridCellBase<CustomGridCell> {
		public:
			using fpmas::model::GridCellBase<CustomGridCell>::GridCellBase;
			using CustomCell<CustomGridCell>::CustomCell;
	};

	class CustomGraphCell :
		public CustomCell<CustomGraphCell>,
		public fpmas::model::GraphCellBase<CustomGraphCell> {
		public:
			using fpmas::model::GraphCellBase<CustomGraphCell>::GraphCellBase;
			using CustomCell<CustomGraphCell>::CustomCell;
	};

	template<typename AgentType>
		class SpatialAgentBase : public fpmas::model::SpatialAgent<AgentType> {
			public:
				testing::NiceMock<MockRange<fpmas::api::model::Cell>> mobility_range;
				testing::NiceMock<MockRange<fpmas::api::model::Cell>> perception_range;

				SpatialAgentBase() {}
				SpatialAgentBase(const SpatialAgentBase&)
					: SpatialAgentBase() {}
				SpatialAgentBase(SpatialAgentBase&&)
					: SpatialAgentBase() {}
				SpatialAgentBase& operator=(const SpatialAgentBase&) {return *this;}
				SpatialAgentBase& operator=(SpatialAgentBase&&) {return *this;}

				FPMAS_MOBILITY_RANGE(mobility_range);
				FPMAS_PERCEPTION_RANGE(perception_range);
		};

	class SpatialAgent : public SpatialAgentBase<SpatialAgent> {};

	bool operator==(const SpatialAgent&, const SpatialAgent&);


	class SpatialAgentWithData : public SpatialAgentBase<SpatialAgentWithData> {
		public:
			int data;

			SpatialAgentWithData() : data(44) {
			}
			SpatialAgentWithData(int data) : data(data) {}

			static void to_json(nlohmann::json& j, const SpatialAgentWithData* agent) {
				j = agent->data;
			}
			static SpatialAgentWithData* from_json(const nlohmann::json& j) {
				auto agent = new SpatialAgentWithData(j.get<int>());;
				return agent;
			}

			static std::size_t size(
					const fpmas::io::datapack::ObjectPack& pack,
					const SpatialAgentWithData*) {
				return pack.size<int>();
			}

			static void to_datapack(
					fpmas::io::datapack::ObjectPack& pack,
					const SpatialAgentWithData* agent) {
				pack.put(agent->data);
			}
			static SpatialAgentWithData* from_datapack(
					const fpmas::io::datapack::ObjectPack& pack) {
				auto agent = new SpatialAgentWithData(pack.get<int>());;
				return agent;
			}

			bool operator==(const SpatialAgentWithData& agent) const {
				return data == agent.data;
			}
	};

	template<typename GridAgentType>
		class GridAgentBase : public fpmas::model::GridAgent<GridAgentType, MockGridCell> {
			public:
				static MockRange<MockGridCell>* mobility_range;
				static MockRange<MockGridCell>* perception_range;

				FPMAS_MOBILITY_RANGE(*mobility_range);
				FPMAS_PERCEPTION_RANGE(*perception_range);
		};

	template<typename GridAgentType>
	MockRange<MockGridCell>* GridAgentBase<GridAgentType>::mobility_range;
	template<typename GridAgentType>
	MockRange<MockGridCell>* GridAgentBase<GridAgentType>::perception_range;

	class GridAgent : public GridAgentBase<GridAgent> {};

	bool operator==(const GridAgent&, const GridAgent&);

	class GridAgentWithData : public GridAgentBase<GridAgentWithData> {
		public:
			int data;

			GridAgentWithData() : data(0) {}
			GridAgentWithData(int data)
				: data(data) {}

			static GridAgentWithData* from_json(const nlohmann::json& j) {
				return new GridAgentWithData(j.get<int>());
			}
			static void to_json(nlohmann::json& j, const GridAgentWithData* agent) {
				j = agent->data;
			}

			static std::size_t size(
					const fpmas::io::datapack::ObjectPack& pack,
					const GridAgentWithData*) {
				return pack.size<int>();
			}

			static void to_datapack(
					fpmas::io::datapack::ObjectPack& pack,
					const GridAgentWithData* agent) {
				pack.put(agent->data);
			}

			static GridAgentWithData* from_datapack(
					const fpmas::io::datapack::ObjectPack& pack) {
				return new GridAgentWithData(pack.get<int>());
			}

			bool operator==(const GridAgentWithData& agent) const {
				return data == agent.data;
			}
	};
}}

FPMAS_DEFAULT_JSON(model::test::SpatialAgent);
FPMAS_DEFAULT_DATAPACK(::model::test::SpatialAgent);

FPMAS_DEFAULT_JSON(model::test::GridAgent);
FPMAS_DEFAULT_DATAPACK(::model::test::GridAgent);

#endif
