#ifndef AGENT_SERIALIZER_H
#define AGENT_SERIALIZER_H

#include <nlohmann/json.hpp>
#include <tuple>

namespace FPMAS::agent {
	template<typename...> class Agent;
}

namespace nlohmann {
	using FPMAS::agent::Agent;

	/**
	 * TypeInfoRef, Hasher and EqualTo come from the cppreference example at
	 * https://en.cppreference.com/w/cpp/types/type_info/hash_code
	 */
	namespace type_hash {
		// Reference wrapper to type_info (allows to store reference to type_info
		// in maps)
		using TypeInfoRef = std::reference_wrapper<const std::type_info>;

		// type_info Hash function
		struct Hasher {
			std::size_t operator()(TypeInfoRef code) const
			{
				return code.get().hash_code();
			}
		};

		// type_info EqualTo
		struct EqualTo {
			bool operator()(TypeInfoRef lhs, TypeInfoRef rhs) const
			{
				return lhs.get() == rhs.get();
			}
		};
	}
	using type_hash::TypeInfoRef;
	using type_hash::Hasher;
	using type_hash::EqualTo;

	template<typename... Types>
    struct adl_serializer<std::unique_ptr<Agent<Types...>>> {
		static const std::unordered_map<TypeInfoRef, unsigned long, Hasher, EqualTo> type_id_map;
		static const std::unordered_map<unsigned long, TypeInfoRef> id_type_map;

		static std::unordered_map<TypeInfoRef, unsigned long, Hasher, EqualTo> init_type_id_map() {
			int i = 0;
			std::unordered_map<TypeInfoRef, unsigned long, Hasher, EqualTo> map;
			(map.insert(std::make_pair((TypeInfoRef) typeid(Types), i++)), ...);
			return map;
		}

		static std::unordered_map<unsigned long, TypeInfoRef> init_id_type_map() {
			int i = 0;
			std::unordered_map<unsigned long, TypeInfoRef> map;
			(map.insert(std::make_pair(i++, (TypeInfoRef) typeid(Types))), ...);
			return map;
		}

		static void to_json(json& j, const std::unique_ptr<Agent<Types...>>& data) {
			Agent<Types...>::to_json_t(j, data, Types()...);
		}

		static void from_json(const json& j, std::unique_ptr<Agent<Types...>>& agent_ptr) {
			Agent<Types...>::from_json_t(j, agent_ptr, j.at("type").get<unsigned long>(), Types()...);
			}
		};
	template<typename... Types> const std::unordered_map<TypeInfoRef, unsigned long, Hasher, EqualTo> 
		adl_serializer<std::unique_ptr<Agent<Types...>>>::type_id_map = init_type_id_map();
	template<typename... Types> const std::unordered_map<unsigned long, TypeInfoRef> 
		adl_serializer<std::unique_ptr<Agent<Types...>>>::id_type_map = init_id_type_map();

}
#endif
