#ifndef DEFAULT_ID_H
#define DEFAULT_ID_H

#include <nlohmann/json.hpp>

namespace FPMAS::graph::base {
	class DefaultId {
		friend nlohmann::adl_serializer<DefaultId>;
		private:
			unsigned long id;

		public:
			DefaultId(unsigned long value) : id(value) {}

			DefaultId operator++(int) {
				return {id++};
			}

			unsigned long get() const {
				return id;
			}

			bool operator==(const DefaultId& id) const {
				return this->id == id.id;
			}
			operator std::string() const {
				return std::to_string(id);
			}
	};
}

using FPMAS::graph::base::DefaultId;

namespace nlohmann {
	template<>
		struct adl_serializer<DefaultId> {
			static void to_json(json& j, const DefaultId& value) {
				j = value.get();
			}

			static DefaultId from_json(const json& j) {
				return DefaultId(
						j.get<unsigned long>()
						);
			}
		};
	template<typename id>
		struct adl_serializer<std::array<id, 2>> {
			static void to_json(json& j, const std::array<id, 2>& value) {
				j = {value[0], value[1]};
			}

			static std::array<id, 2> from_json(const json& j) {
				return {
					j[0].get<id>(),
					j[1].get<id>()
				};
			}
		};
}
#endif
