#ifndef FPMAS_MODEL_SERIALIZER_SET_UP_H
#define FPMAS_MODEL_SERIALIZER_SET_UP_H

#include <stdexcept>
#include <typeindex>
#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/utils/log.h"

#define AGENT_TYPE_STR(AgentType) typeid(AgentType).name()

#ifndef FPMAS_TYPE_INDEX
	/**
	 * Type used to serialize
	 * [std::type_index](https://en.cppreference.com/w/cpp/types/type_index)
	 * instances, that is notably used by the polymorphic \Agent serialization
	 * process.
	 *
	 * More precisely, the specified unsigned integer type must be able to
	 * represent the count of \Agent passed to the FPMAS_REGISTER_AGENT_TYPES()
	 * macro.
	 *
	 * In consequence, the default
	 * [std::uint_fast8_t](https://en.cppreference.com/w/cpp/types/integer)
	 * type should be able to represent 256 agent types while maximizing
	 * performances.
	 *
	 * But an other unsigned integer types can be specified at compile time
	 * using
	 * `cmake -DFPMAS_TYPE_INDEX=<unsigned integer type> ..`
	 */
	#define FPMAS_TYPE_INDEX std::uint_fast8_t
#endif

namespace fpmas {
	/**
	 * Typed \Agent pointer.
	 *
	 * Contrary to api::model::AgentPtr, that represents a polymorphic
	 * \Agent (i.e. api::utils::PtrWrapper<api::model::Agent>), this
	 * pointer is used to wrap a concrete \Agent type (e.g.
	 * api::utils::PtrWrapper<UserAgent1>).
	 *
	 * This type is notably used to define json serialization rules for
	 * user defined \Agent types.
	 *
	 * @tparam Agent concrete agent type
	 */
	template<typename Agent>
		using TypedAgentPtr = api::utils::PtrWrapper<Agent>;


	namespace exceptions {
		/**
		 * Exception raised when unserializing an std::type_index if the provided
		 * type id does not correspond to any registered type.
		 *
		 * @see register_types
		 * @see nlohmann::adl_serializer<std::type_index>
		 */
		class BadIdException : public std::exception {
			private:
				std::string message;

			public:
				/**
				 * BadIdException constructor.
				 *
				 * @param bad_id unregistered id
				 */
				BadIdException(std::size_t bad_id)
					: message("Unknown type id : " + std::to_string(bad_id)) {}

				/**
				 * Returns the explanatory string.
				 *
				 * @see https://en.cppreference.com/w/cpp/error/exception/what
				 */
				const char* what() const noexcept override {
					return message.c_str();
				}
		};

		/**
		 * Exception raised when serializing an std::type_index if the provided
		 * type index not correspond to any registered type.
		 *
		 * @see register_types
		 * @see nlohmann::adl_serializer<std::type_index>
		 */
		class BadTypeException : public std::exception {
			private:
				std::string message;

			public:
				/**
				 * BadTypeException constructor.
				 *
				 * @param bad_type unregistered std::type_index
				 */
				BadTypeException(const std::type_index& bad_type)
					: message("Unknown type index : " + std::string(bad_type.name())) {}

				/**
				 * Returns the explanatory string.
				 *
				 * @see https://en.cppreference.com/w/cpp/error/exception/what
				 */
				const char* what() const noexcept override {
					return message.c_str();
				}
		};
	}
}

#endif
