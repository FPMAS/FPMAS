#ifndef FPMAS_MODEL_DATAPACK_SERIALIZER_H
#define FPMAS_MODEL_DATAPACK_SERIALIZER_H

#include "serializer_set_up.h"
#include "fpmas/api/model/model.h"
#include "fpmas/io/datapack.h"

#define FPMAS_DATAPACK_SET_UP(...)\
	namespace fpmas { namespace io { namespace datapack {\
		void Serializer<api::model::AgentPtr>\
			::to_datapack(ObjectPack& p, const api::model::AgentPtr& data) {\
			AgentPtrSerializer<ObjectPack, __VA_ARGS__ , void>::to_datapack(p, data);\
		}\
		api::model::AgentPtr Serializer<api::model::AgentPtr>\
			::from_datapack(const ObjectPack& p) {\
			return {AgentPtrSerializer<ObjectPack, __VA_ARGS__ , void>::from_datapack(p)};\
		}\
		\
		void Serializer<api::model::WeakAgentPtr>\
			::to_datapack(ObjectPack& p, const api::model::WeakAgentPtr& data) {\
			AgentPtrSerializer<ObjectPack, __VA_ARGS__ , void>::to_datapack(p, data);\
		}\
		api::model::WeakAgentPtr Serializer<api::model::WeakAgentPtr>\
			::from_datapack(const ObjectPack& p) {\
			return AgentPtrSerializer<ObjectPack, __VA_ARGS__ , void>::from_datapack(p);\
		}\
		\
		void LightSerializer<api::model::AgentPtr>\
			::to_datapack(LightObjectPack& p, const api::model::AgentPtr& data) {\
			AgentPtrSerializer<LightObjectPack, __VA_ARGS__ , void>::to_datapack(p, data);\
		}\
		api::model::AgentPtr LightSerializer<api::model::AgentPtr>::from_datapack(const LightObjectPack& p) {\
			return {AgentPtrSerializer<LightObjectPack, __VA_ARGS__ , void>::from_datapack(p)};\
		}\
		\
		void LightSerializer<api::model::WeakAgentPtr>\
			::to_datapack(LightObjectPack& p, const api::model::WeakAgentPtr& data) {\
			AgentPtrSerializer<LightObjectPack, __VA_ARGS__ , void>::to_datapack(p, data);\
		}\
		api::model::WeakAgentPtr LightSerializer<api::model::WeakAgentPtr>\
			::from_datapack(const LightObjectPack& p) {\
			return AgentPtrSerializer<LightObjectPack, __VA_ARGS__ , void>::from_datapack(p);\
		}\
	}}}


namespace fpmas { namespace io { namespace datapack {
	using api::model::AgentPtr;
	using api::model::WeakAgentPtr;

	template<typename JsonType, typename Type, typename... AgentTypes> 
		struct AgentPtrSerializer;

	/**
	 * AgentPtrSerializer recursion base case.
	 */
	template<typename PackType> 
		struct AgentPtrSerializer<PackType, void> {
			/**
			 * to_datapack recursion base case.
			 *
			 * Reaching this case is erroneous and throws a exceptions::BadTypeException.
			 *
			 * @throw exceptions::BadTypeException
			 */
			static void to_datapack(PackType&, const WeakAgentPtr& ptr); 

			/**
			 * from_datapack recursion base case.
			 *
			 * Reaching this case is erroneous and throws a exceptions::BadIdException.
			 *
			 * @throw exceptions::BadIdException
			 */
			static WeakAgentPtr from_datapack(const PackType& p);
		};

	template<typename PackType>
		void AgentPtrSerializer<PackType, void>::to_datapack(
				PackType &, const WeakAgentPtr& ptr) {
			FPMAS_LOGE(-1, "AGENT_SERIALIZER",
					"Invalid agent type : %s. Make sure to properly register "
					"the Agent type with FPMAS_DATAPACK_SET_UP and FPMAS_REGISTER_AGENT_TYPES.",
					ptr->typeId().name());
			throw exceptions::BadTypeException(ptr->typeId());
		}

	template<typename PackType>
		WeakAgentPtr AgentPtrSerializer<PackType, void>::from_datapack(const PackType &p) {
			std::size_t id = p.template read<std::size_t>("type");
			FPMAS_LOGE(-1, "AGENT_SERIALIZER",
					"Invalid agent type id : %lu. Make sure to properly register "
					"the Agent type with FPMAS_DATAPACK_SET_UP and FPMAS_REGISTER_AGENT_TYPES.",
					id);
			throw exceptions::BadIdException(id);
		}

	/**
	 * Generic AgentPtr serializer, that recursively deduces the final and
	 * concrete types of polymorphic AgentPtr instances.
	 *
	 * @tparam PackType type of ObjectPack used for serialization
	 * @tparam Type type currently inspected
	 * @tparam AgentTypes agent types not inspected yet (must end with
	 * `void`)
	 */
	template<typename PackType, typename Type, typename... AgentTypes> 
		struct AgentPtrSerializer {
			/**
			 * Recursive to_datapack method used to serialize polymorphic \Agent
			 * pointers as ObjectPacks.
			 *
			 * `Type` corresponds to the currently examined \Agent type. If this
			 * type id (i.e. Type::TYPE_ID) corresponds to the `ptr` \Agent type
			 * (i.e.  `ptr->typedId()`), the underlying agent is serialized
			 * using the specified PackType.
			 *
			 * Else, attempts to recursively serialize `ptr` with
			 * `to_datapack<AgentTypes...>(p, ptr)`. Notice that the last
			 * value of `AgentTypes` **must** be void to reach the
			 * recursion base case (see to_datapack<void>).
			 *
			 * @param p destination ObjectPack
			 * @param ptr polymorphic agent pointer to serialize
			 */
			static void to_datapack(
					PackType& p, const WeakAgentPtr& ptr) {
				if(ptr->typeId() == Type::TYPE_ID) {
					PackType agent = TypedAgentPtr<Type>(
							const_cast<Type*>(dynamic_cast<const Type*>(ptr.get()))
							);
					p.allocate(
							pack_size<Type::TYPE_ID>() +
							pack_size(agent) +
							pack_size(ptr->groupIds())
							);
					p.write(Type::TYPE_ID);
					p.write(agent);
					p.write(ptr->groupIds());
				} else {
					AgentPtrSerializer<PackType, AgentTypes...>::to_datapack(p, ptr);
				}
			}

			/**
			 * Recursive from_datapack method used to unserialize polymorphic \Agent
			 * pointers from ObjectPacks.
			 *
			 * `Type` corresponds to the currently examined \Agent type. If
			 * this type id (i.e. Type::TYPE_ID) corresponds to the
			 * ObjectPack type id value , the ObjectPack is unserialized
			 * using the specified PackType.
			 *
			 * Else, attempts to recursively unserialize the ObjectPack
			 * with `from_datapack<AgentTypes...>(p, ptr)`. Notice that the
			 * last value of `AgentTypes` **must** be void to reach the
			 * recursion base case (see from_datapack<void>).
			 *
			 * @param p source ObjectPack
			 * @return ptr unserialized polymorphic agent pointer
			 */
			static WeakAgentPtr from_datapack(const PackType& p) {
				std::size_t pos = p.readPos();
				fpmas::api::model::TypeId id = p.template read<api::model::TypeId>();
				if(id == Type::TYPE_ID) {
					auto agent = p
						.template read<PackType>()
						.template get<TypedAgentPtr<Type>>();
					for(auto gid : 
							p.template read<std::vector<api::model::GroupId>>())
						agent->addGroupId(gid);
					return {agent};
				} else {
					p.seekRead(pos);
					return AgentPtrSerializer<PackType, AgentTypes...>::from_json(p);
				}
			}
		};

}}}

#endif
