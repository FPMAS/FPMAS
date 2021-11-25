#ifndef FPMAS_MODEL_DATAPACK_SERIALIZER_H
#define FPMAS_MODEL_DATAPACK_SERIALIZER_H

#include "serializer_set_up.h"
#include "fpmas/api/model/model.h"
#include "fpmas/io/datapack.h"

/**
 * Defines the fpmas::io::datapack::Serializer specializations required to
 * handle the object pack serialization of the specified set of \Agent types.
 *
 * Notice that this is only the _definitions_ of the `to_datapack` and
 * `from_datapack` methods, that are declared (but not defined) in
 * src/fpmas/model/model.h.
 *
 * In consequence, this macro must be invoked exaclty once from a **source**
 * file, **at the global definition level**, in any C++ target using FPMAS.
 *
 * This macro does not define any rule for AgentPtr json serialization,
 * so using only this macro will produce compile time errors.
 *
 * Two solutions are available:
 * - calling FPMAS_BASE_JSON_SET_UP() and FPMAS_BASE_DATAPACK_SET_UP(). This
 *   enable both AgentPtr serialization techniques, but to_json() and
 *   from_json() methods must be specified for all agents, in addition to
 *   the to_datapack() and from_datapack() methods.
 * - just calling FPMAS_DATAPACK_SET_UP(), instead of
 *   FPMAS_BASE_DATAPACK_SET_UP().  This also enable both serialization
 *   techniques, but json serialization falls back to object pack
 *   serialization. This might be inefficient, but only requires to_datapack()
 *   and from_datapack() definitions.
 *
 * @see FPMAS_BASE_JSON_SET_UP()
 * @see FPMAS_DATAPACK_SET_UP()
 * @see FPMAS_JSON_SET_UP()
 */
#define FPMAS_BASE_DATAPACK_SET_UP(...)\
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

/**
 * Can be used instead of FPMAS_BASE_DATAPACK_SET_UP() to set up json serialization
 * without specifying any Agent type.
 */
#define FPMAS_BASE_DEFAULT_DATAPACK_SET_UP() \
	namespace fpmas { namespace io { namespace datapack {\
		void Serializer<fpmas::api::model::AgentPtr>::to_datapack(ObjectPack& o, const fpmas::api::model::AgentPtr& data) {\
			AgentPtrSerializer<ObjectPack, void>::to_datapack(o, data);\
		}\
		fpmas::api::model::AgentPtr Serializer<fpmas::api::model::AgentPtr>::from_datapack(const ObjectPack& o) {\
			return {AgentPtrSerializer<ObjectPack, void>::from_datapack(o)};\
		}\
		\
		void Serializer<fpmas::api::model::WeakAgentPtr>\
			::to_datapack(ObjectPack& o, const fpmas::api::model::WeakAgentPtr& data) {\
			AgentPtrSerializer<ObjectPack, void>::to_datapack(o, data);\
		}\
		fpmas::api::model::WeakAgentPtr Serializer<fpmas::api::model::WeakAgentPtr>\
			::from_datapack(const ObjectPack& o) {\
			return AgentPtrSerializer<ObjectPack, void>::from_datapack(o);\
		}\
		void LightSerializer<fpmas::api::model::AgentPtr>::to_datapack(LightObjectPack& o, const fpmas::api::model::AgentPtr& data) {\
			AgentPtrSerializer<LightObjectPack, void>::to_datapack(o, data);\
		}\
		fpmas::api::model::AgentPtr LightSerializer<fpmas::api::model::AgentPtr>::from_datapack(const LightObjectPack& o) {\
			return {AgentPtrSerializer<LightObjectPack, void>::from_datapack(o)};\
		}\
		\
		void LightSerializer<fpmas::api::model::WeakAgentPtr>\
			::to_datapack(LightObjectPack& o, const fpmas::api::model::WeakAgentPtr& data) {\
			AgentPtrSerializer<LightObjectPack, void>::to_datapack(o, data);\
		}\
		fpmas::api::model::WeakAgentPtr LightSerializer<fpmas::api::model::WeakAgentPtr>\
			::from_datapack(const LightObjectPack& o) {\
			return AgentPtrSerializer<LightObjectPack, void>::from_datapack(o);\
		}\
	}}}\

/**
 * Helper macro to easily define object pack serialization rules for
 * [DefaultConstructible](https://en.cppreference.com/w/cpp/named_req/DefaultConstructible)
 * \Agents.
 *
 * @param AGENT DefaultConstructible \Agent type
 */
#define FPMAS_DEFAULT_DATAPACK(AGENT) \
	namespace fpmas { namespace io { namespace datapack {\
		template<>\
			/**\
			 * Default AGENT Serializer specialization.
			 */\
			struct Serializer<fpmas::api::utils::PtrWrapper<AGENT>> {\
				/**\
				 * No effect: o is left empty.
				 *
				 * @param o output ObjectPack
				 */\
				template<typename PackType>\
				static void to_datapack(PackType& o, const fpmas::api::utils::PtrWrapper<AGENT>&) {\
				}\
\
				/**\
				 * Unserializes a default AGENT, dynamically allocated with `new AGENT`.
				 *
				 * @param ptr unserialized AGENT pointer
				 */\
				template<typename PackType>\
				static fpmas::api::utils::PtrWrapper<AGENT> from_datapack(const PackType&) {\
					return {new AGENT};\
				}\
			};\
	}}}

namespace fpmas { namespace io { namespace datapack {

	using api::model::AgentPtr;
	using api::model::WeakAgentPtr;

	/**
	 * A Serializer specialization to allow ObjectPack serialization of
	 * [std:type_index](https://en.cppreference.com/w/cpp/types/type_index),
	 * that is used as \Agents TypeId.
	 *
	 * Each registered std::type_index is automatically mapped to a unique
	 * std::size_t integer to allow serialization.
	 */
	template<>
		struct Serializer<std::type_index> {
			private:
				static std::size_t id;
				static std::unordered_map<std::size_t, std::type_index> id_to_type;
				static std::unordered_map<std::type_index, std::size_t> type_to_id;

			public:
				/**
				 * Register an std::type_index instance so that it can be
				 * serialized.
				 *
				 * A unique integer id is internally associated to the type and
				 * used for serialization.
				 *
				 * The fpmas::register_types() function template and the
				 * FPMAS_REGISTER_AGENT_TYPES(...) macro can be used to easily register
				 * types.
				 *
				 * @param type type to register
				 */
				static std::size_t register_type(const std::type_index& type) {
					std::size_t new_id = id++;
					type_to_id.insert({type, new_id});
					id_to_type.insert({new_id, type});
					return new_id;
				}

				/**
				 * Serializes an std::type_index instance into the specified
				 * ObjectPack.
				 *
				 * @param o object pack output
				 * @param index type index to serialize
				 * @throw fpmas::exceptions::BadTypeException if the type was not
				 * register using register_type()
				 */
				template<typename PackType>
					static void to_datapack(PackType& o, const std::type_index& index) {
						auto _id = type_to_id.find(index);
						if(_id != type_to_id.end())
							o = _id->second;
						else
							throw fpmas::exceptions::BadTypeException(index);
					}

				/**
				 * Unserializes an std::type_index instance from the specified
				 * ObjectPack.
				 *
				 * @param o object pack input
				 * @throw fpmas::exceptions::BadIdException if the id does not
				 * correspond to a type registered using register_type()
				 */
				template<typename PackType>
					static std::type_index from_datapack(const PackType& o) {
						std::size_t type_id = o.template get<std::size_t>();
						auto _type = id_to_type.find(type_id);
						if(_type != id_to_type.end())
							return _type->second;
						throw fpmas::exceptions::BadIdException(type_id);
					}
		};

	/**
	 * Generic \Agent pointer serialization.
	 *
	 * This partial specialization can be used to easily define serialization
	 * rules directly from the \Agent implementation, using the following
	 * static method definitions :
	 * ```cpp
	 * class UserDefinedAgent : fpmas::model::AgentBase<UserDefinedAgent> {
	 * 	...
	 * 	public:
	 * 		...
	 * 		static void to_datapack(
	 * 			fpmas::io::datapack::ObjectPack& o,
	 * 			const UserDefinedAgent* agent
	 * 			);
	 * 		static UserDefinedAgent* from_datapack(
	 * 			const fpmas::io::datapack::ObjectPack& o
	 * 			);
	 * 		...
	 * 	};
	 * 	```
	 * The from_datapack method is assumed to return an **heap allocated** agent
	 * (initialized with a `new` statement).
	 *
	 * \par Example
	 * ```cpp
	 * class Agent1 : public fpmas::model::AgentBase<Agent1> {
	 * 	private:
	 * 		int count;
	 * 		std::string message;
	 * 	public:
	 * 		Agent1(int count, std::string message)
	 * 			: count(count), message(message) {}
	 *
	 * 		static void to_datapack(fpmas::io::datapack::ObjectPack& o, const Agent1* agent) {
	 * 			using namespace fpmas::io::datapack;
	 *
	 * 			o.allocate(pack_size<int>() + pack_size(agent->message));
	 * 			o.write(agent->count);
	 * 			o.write(agent->message);
	 * 		}
	 *
	 * 		static Agent1* from_datapack(const fpmas::io::datapack::ObjectPack& o) {
	 * 			using namespace fpmas::io::datapack;
	 *
	 * 			int count;
	 * 			std::string message;
	 * 			o.read(count);
	 * 			o.read(message);
	 * 			return new Agent1(count, message);
	 * 		}
	 * 	};
	 * ```
	 *
	 * \note
	 * Notice that its still possible to define adl_serializer specialization
	 * without using the internal to_json / from_json methods.
	 *
	 * \par Example
	 * ```cpp
	 * class Agent1 : public fpmas::model::AgentBase<Agent1> {
	 * 	private:
	 * 		int count;
	 * 		std::string message;
	 * 	public:
	 * 		Agent1(int count, std::string message)
	 * 			: count(count), message(message) {}
	 *
	 * 		int getCount() const {return count;}
	 * 		std::string getMessage() const {return message;}
	 * 	};
	 *
	 * 	namespace fpmas { namespace io { namespace datapack {
	 * 		template<>
	 * 			struct Serializer<fpmas::api::utils::PtrWrapper<Agent1>> {
	 * 				static void to_datapack(
	 * 					ObjectPack& o,
	 * 					fpmas::api::utils::PtrWrapper<Agent1>& agent_ptr
	 * 				) {
	 * 					o.allocate(pack_size<int>() + pack_size(agent->message));
	 * 					o.write(agent->count);
	 * 					o.write(agent->message);
	 * 				}
	 * 				
	 * 				static fpmas::api::utils::PtrWrapper<Agent1> from_datapack(
	 * 					ObjectPack& j
	 * 				) {
	 * 					int count;
	 * 					std::string message;
	 * 					o.read(count);
	 * 					o.read(message);
	 * 					return new Agent1(count, message);
	 * 				}
	 * 		};
	 * 	}}}
	 * 	```
	 */
	 template<typename AgentType>
	 struct Serializer<api::utils::PtrWrapper<AgentType>> {
		 /**
		  * Returns a PtrWrapper initialized with `AgentType::from_datapack(o)`.
		  *
		  * @param o object pack output
		  * @return unserialized agent pointer
		  */
		 template<typename PackType>
			 static api::utils::PtrWrapper<AgentType> from_datapack(const PackType& o) {
				 return AgentType::from_datapack(o);
			 }

		 /**
		  * Calls `AgentType::to_datapack(o, agent_ptr.get())`.
		  *
		  * @param o object pack input
		  * @param agent_ptr agent pointer to serialized
		  */
		 template<typename PackType>
			 static void to_datapack(PackType& o, const api::utils::PtrWrapper<AgentType>& agent_ptr) {
				 AgentType::to_datapack(o, agent_ptr.get());
			 }
	 };

	/**
	 * \anchor not_default_constructible_Agent_datapack_light_serializer
	 *
	 * A default LightSerializer specialization for \Agent types, when no
	 * default constructor is available. In this case, to_datapack() and
	 * from_datapack() methods falls back to the classic
	 * `io::datapack::Serializer` serialization rules, what might be
	 * inefficient. A warning is also printed at runtime.
	 *
	 * To avoid this inefficient behaviors, two things can be performed:
	 * - Defining a default constructor for `AgentType`. Notice that the
	 *   default constructed `AgentType` will **never** be used by FPMAS (since
	 *   `AgentType` transmitted in a LightObjectPack are completely passive, a
	 *   complete \DistributedNode transfer is always performed when the \Agent
	 *   data is required).
	 * - Specializing the LightSerializer with `AgentType`:
	 *   ```cpp
	 *   namespace fpmas { namespace io { namespace datapack {
	 *   	template<>
	 *   		struct LightSerializer<fpmas::api::utils::PtrWrapper<CustomAgentType>> {
	 *   			static void to_datapack(LightObjectPack& o, const fpmas::api::utils::PtrWrapper<CustomAgentType>& ptr) {
	 *   				// LightObjectPack serialization rules.
	 *					// Might be empty.
	 *   				...
	 *   			}
	 *
	 *   			static fpmas::api::utils::PtrWrapper<CustomAgentType> from_datapack(LightObjectPack& o) {
	 *   				// LightObjectPack unserialization rules
	 *   				...
	 *
	 *   				return new CustomAgentType(...);
	 *   			}
	 *   		};
	 *   }}}
	 *   ```
	 *
	 * @tparam AgentType concrete type of \Agent to serialize
	 */
	template<typename AgentType>
		struct LightSerializer<
		fpmas::api::utils::PtrWrapper<AgentType>,
		typename std::enable_if<
			std::is_base_of<fpmas::api::model::Agent, AgentType>::value
			&& !std::is_default_constructible<AgentType>::value
			&& std::is_same<AgentType, typename AgentType::FinalAgentType>::value>::type
			>{
				public:
					/**
					 * \anchor not_default_constructible_Agent_light_serializer_to_datapack
					 *
					 * Calls
					 * `Serializer<fpmas::api::utils::PtrWrapper<AgentType>>::%to_datapack()`.
					 *
					 * @param o object pack output
					 * @param agent agent to serialize
					 */
					static void to_datapack(LightObjectPack& o, const fpmas::api::utils::PtrWrapper<AgentType>& agent) {
						static bool warn_print = false;
						if(!warn_print) {
							warn_print = true;
							FPMAS_LOGW(0, "LightSerializer",
									"Type %s does not define a default constructor or "
									"a LightSerializer specialization."
									"This might be uneficient.", AGENT_TYPE_STR(AgentType)
									);
						}

						ObjectPack _p = agent;
						o = LightObjectPack::parse(_p.dump());
					}

					/**
					 * \anchor not_default_constructible_Agent_light_serializer_from_datapack
					 *
					 * Calls
					 * `Serializer<fpmas::api::utils::PtrWrapper<AgentType>>::%from_datapack()`.
					 *
					 * @param o object pack input
					 * @return dynamically allocated `AgentType`
					 */
					static fpmas::api::utils::PtrWrapper<AgentType> from_datapack(const LightObjectPack& o) {
						ObjectPack _p = ObjectPack::parse(o.data());
						return _p.get<fpmas::api::utils::PtrWrapper<AgentType>>();
					}
			};

	/**
	 * \anchor default_constructible_Agent_datapack_light_serializer
	 *
	 * A default LightSerializer specialization for default constructible
	 * \Agent types.
	 *
	 * When a default constructor is available for `AgentType`, this
	 * specialization is automatically selected.
	 *
	 * The usage of this specialization is recommended, since its optimal:
	 * **no** data is required to serialize / unserialize a LightObjectPack
	 * representing such an `AgentType`. Moreover, its usage is perfectly safe
	 * since AgentType instanciated this way are never used directly within
	 * FPMAS (at least a complete ObjectPack import is realized before
	 * accessing AgentType data, using a ReadGuard for example).
	 *
	 * @tparam AgentType concrete type of \Agent to serialize
	 */
	template<typename AgentType>
		struct LightSerializer<
		fpmas::api::utils::PtrWrapper<AgentType>,
		typename std::enable_if<
			std::is_base_of<api::model::Agent, AgentType>::value
			&& std::is_default_constructible<AgentType>::value
			&& std::is_same<AgentType, typename AgentType::FinalAgentType>::value>::type
			> {

				/**
				 * \anchor default_constructible_Agent_light_serializer_to_datapack
				 *
				 * LightObjectPack to_datapack() implementation for the default
				 * constructible `AgentType`: nothing is serialized.
				 */
				static void to_datapack(
						LightObjectPack&, const api::utils::PtrWrapper<AgentType>&) {
				}

				/**
				 * \anchor default_constructible_Agent_light_serializer_from_datapack
				 *
				 * LightObjectPack from_datapack() implementation for the default
				 * constructible `AgentType`: a dynamically allocated, default
				 * constructed `AgentType` is returned.
				 *
				 * @return dynamically allocated `AgentType`
				 */
				static api::utils::PtrWrapper<AgentType> from_datapack(
						const LightObjectPack&) {
					return new AgentType;
				}
			};

	template<typename PackType, typename Type, typename... AgentTypes> 
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
			std::size_t id = p.template read<std::size_t>();
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
					PackType type_id = Type::TYPE_ID;
					PackType agent = TypedAgentPtr<Type>(
							const_cast<Type*>(dynamic_cast<const Type*>(ptr.get()))
							);
					p.allocate(
							pack_size(type_id) +
							pack_size(agent) +
							pack_size(ptr->groupIds())
							);
					p.write(type_id);
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
				fpmas::api::model::TypeId id = p
					.template read<PackType>()
					.template get<fpmas::api::model::TypeId>();
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
					return AgentPtrSerializer<PackType, AgentTypes...>::from_datapack(p);
				}
			}
		};

}}}

#endif
