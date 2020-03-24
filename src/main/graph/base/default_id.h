#ifndef DEFAULT_ID_H
#define DEFAULT_ID_H

#include <nlohmann/json.hpp>

namespace FPMAS::graph::base {
	/**
	 * A default `IdType` implementation, used by default in a Graph instance.
	 *
	 * The id is simply represented by an incrementing `unsigned long`.
	 */
	class DefaultId {
		friend nlohmann::adl_serializer<DefaultId>;
		private:
			unsigned long id;

		public:
			/**
			 * Initialized the DefaultId instance with the specified value.
			 */
			DefaultId(unsigned long value) : id(value) {}

			/**
			 * Increments the internal id.
			 */
			DefaultId operator++(int) {
				return {id++};
			}

			/**
			 * Gets the current internal id value.
			 *
			 * @return internal id
			 */
			unsigned long get() const {
				return id;
			}

			/**
			 * Two DefaultId instances are equal iff their internal ids are
			 * equal.
			 *
			 * @param id other DefaultId to compare
			 * @return comparison result
			 */
			bool operator==(const DefaultId& id) const {
				return this->id == id.id;
			}

			/**
			 * std::string conversion.
			 */
			operator std::string() const {
				return std::to_string(id);
			}
	};
}

using FPMAS::graph::base::DefaultId;

namespace std {
	/**
	 * DefaultId hash function object.
	 */
	template<> struct hash<DefaultId> {
		/**
		 * Returns the id hash.
		 *
		 * @return hash of the internal id (unsigned long)
		 */
		std::size_t operator()(DefaultId const& id) const noexcept
		{
			return std::hash<unsigned long>{}(id.get());
		}
	};
}

namespace nlohmann {
	template<>
		/**
		 * DefaultId json serializer / unserializer.
		 */
		struct adl_serializer<DefaultId> {
			/**
			 * Serializes the internal id.
			 *
			 * @param j current json
			 * @param value DefaultId to serialize
			 */
			static void to_json(json& j, const DefaultId& value) {
				j = value.get();
			}

			/**
			 * Builds a DefaultId instance from the unserialized internal id.
			 *
			 * @param j json to unserialize
			 * @return unserialized DefaultId instance
			 */
			static DefaultId from_json(const json& j) {
				return DefaultId(
						j.get<unsigned long>()
						);
			}
		};
}
#endif
