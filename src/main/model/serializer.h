#ifndef MODEL_SERIALIZER_H
#define MODEL_SERIALIZER_H

#include "api/utils/ptr_wrapper.h"
#include "api/model/model.h"

namespace FPMAS::model {
	typedef api::utils::VirtualPtrWrapper<api::model::Agent> AgentPtr;
	template<typename T>
		using TypedAgentPtr = api::utils::VirtualPtrWrapper<T>;

	template<typename Type, typename... AgentTypes> 
		void to_json(nlohmann::json& j, const AgentPtr& ptr);

	template<> 
		void to_json<void>(nlohmann::json& j, const AgentPtr& ptr) {
		}

	template<typename Type, typename... AgentTypes> 
		void to_json(nlohmann::json& j, const AgentPtr& ptr) {
			if(ptr->typeId() == Type::TYPE_ID) {
				j["type"] = Type::TYPE_ID;
				j["gid"] = ptr->groupId();
				j["agent"] = TypedAgentPtr<Type>(const_cast<Type*>(static_cast<const Type*>(ptr.get())));
			} else {
				to_json<AgentTypes...>(j, ptr);
			}
		}

	template<typename Type, typename... AgentTypes> 
		void from_json(const nlohmann::json& j, AgentPtr& ptr);

	template<> 
		void from_json<void>(const nlohmann::json& j, AgentPtr& ptr) {
		}

	template<typename Type, typename... AgentTypes> 
		void from_json(const nlohmann::json& j, AgentPtr& ptr) {
			FPMAS::api::model::TypeId id = j.at("type").get<FPMAS::api::model::TypeId>();
			if(id == Type::TYPE_ID) {
				ptr = AgentPtr(j.at("agent").get<TypedAgentPtr<Type>>());
				ptr->setGroupId(j.at("gid").get<FPMAS::api::model::GroupId>());
			} else {
				from_json<AgentTypes...>(j, ptr);
			}
		}
}
	

#define FPMAS_JSON_SERIALIZE_AGENT(...) \
	namespace nlohmann {\
		typedef FPMAS::api::utils::VirtualPtrWrapper<FPMAS::api::model::Agent> AgentPtr;\
		template<>\
		struct adl_serializer<AgentPtr> {\
			static void to_json(json& j, const AgentPtr& data) {\
				FPMAS::model::to_json<__VA_ARGS__, void>(j, data);\
			}\
			static void from_json(const json& j, AgentPtr& ptr) {\
				FPMAS::model::from_json<__VA_ARGS__, void>(j, ptr);\
			}\
		};\
	}\

/*
 *namespace FPMAS::model {
 *    typedef FPMAS::api::utils::VirtualPtrWrapper<FPMAS::api::model::Agent> AgentPtr;\
 *        template<typename T>
 *        using TypedAgentPtr = FPMAS::api::utils::VirtualPtrWrapper<T>;
 *
 *    template<typename Type, typename... AgentTypes> 
 *        void to_json(nlohmann::json& j, const AgentPtr& ptr);
 *
 *    template<> 
 *        void to_json<void>(nlohmann::json& j, const AgentPtr& ptr) {
 *        }
 *
 *    template<typename Type, typename... AgentTypes> 
 *        void to_json(nlohmann::json& j, const AgentPtr& ptr) {
 *            if(ptr->typeId() == Type::TYPE_ID) {
 *                j["type"] = Type::TYPE_ID;
 *                j["agent"] = TypedAgentPtr<Type>(const_cast<Type*>(static_cast<const Type*>(ptr.get())));
 *            } else {
 *                to_json<AgentTypes...>(j, ptr);
 *            }
 *        }
 *
 *    template<typename Type, typename... AgentTypes> 
 *        void from_json(const nlohmann::json& j, AgentPtr& ptr);
 *
 *    template<> 
 *        void from_json<void>(const nlohmann::json& j, AgentPtr& ptr) {
 *        }
 *
 *    template<typename Type, typename... AgentTypes> 
 *        void from_json(const nlohmann::json& j, AgentPtr& ptr) {
 *            FPMAS::api::model::TypeId id = j.at("type").get<FPMAS::api::model::TypeId>();
 *            if(id == Type::TYPE_ID) {
 *                ptr = AgentPtr(j.at("agent").get<TypedAgentPtr<Type>>());
 *            } else {
 *                from_json<AgentTypes...>(j, ptr);
 *            }
 *        }
 *}
 */


//namespace nlohmann {
	////using FPMAS::agent::AgentType;
	////using FPMAS::agent::Agent;
	//typedef FPMAS::utils::VirtualPtrWrapper<FPMAS::api::model::Agent*> AgentPtr;

	//template<>
    //struct adl_serializer<AgentPtr> {
/*
 *        static const std::unordered_map<std::type_index, unsigned long> type_id_map;
 *        static const std::unordered_map<unsigned long, std::type_index> id_type_map;
 *
 *        static std::unordered_map<std::type_index, unsigned long> init_type_id_map() {
 *            int i = 0;
 *            std::unordered_map<std::type_index, unsigned long> map;
 *            (map.insert(std::make_pair(std::type_index(typeid(Types)), i++)), ...);
 *            return map;
 *        }
 *
 *        static std::unordered_map<unsigned long, std::type_index> init_id_type_map() {
 *            int i = 0;
 *            std::unordered_map<unsigned long, std::type_index> map;
 *            (map.insert(std::make_pair(i++, std::type_index(typeid(Types)))), ...);
 *            return map;
 *        }
 */

		//static void to_json(json& j, const AgentPtr& data) {
			//switch(data->typeId()) {
				//case AGENT_1 :
					//Agent1& converted_agent = static_cast<Agent1&>(*data);
					//j = converted_agent;
					//break;
				//case AGENT_2 :
					//Agent2& converted_agent = static_cast<Agent2&>(*data);
					//j = converted_agent;
					//break;
			//}
			////Agent<S, Types...>::to_json_t(j, data, AgentType<Types>()...);
		//}

		//static void from_json(const json& j, AgentPtr& agent_ptr) {
			////Agent<S, Types...>::from_json_t(j, agent_ptr, j.at("type").get<unsigned long>(), AgentType<Types>()...);
			//TypeId id = j.at("type").get<TypeId>();
			//switch(data->typeId()) {
				//case AGENT_1 :
					//Agent1& converted_agent = static_cast<Agent1&>(*data);
					//j = converted_agent;
					//auto agent = j.at("agent").get<FPMAS::utils::VirtualPtrWrapper<Agent1>>();
					//agent_ptr.set(agent);
					//break;
				//case AGENT_2 :
					//Agent2& converted_agent = static_cast<Agent2&>(*data);
					//j = converted_agent;
					//auto agent = j.at("agent").get<FPMAS::utils::VirtualPtrWrapper<Agent2>>();
					//agent_ptr.set(agent);
					//break;
			//}
		//}
		//};
	//template<SYNC_MODE, typename... Types>
		//const std::unordered_map<std::type_index, unsigned long> 
		//adl_serializer<std::unique_ptr<Agent<S, Types...>>>::type_id_map
			//= init_type_id_map();
	//template<SYNC_MODE, typename... Types>
		//const std::unordered_map<unsigned long, std::type_index> 
		//adl_serializer<std::unique_ptr<Agent<S, Types...>>>::id_type_map
			//= init_id_type_map();

/*}*/
#endif
