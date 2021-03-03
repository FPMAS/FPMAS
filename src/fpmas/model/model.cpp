#include "model.h"

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
