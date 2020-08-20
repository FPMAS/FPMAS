#ifndef FPMAS_ID_API_H
#define FPMAS_ID_API_H

/** \file src/fpmas/api/graph/id.h
 * Id API
 */

#include <functional>
#include <string>

namespace fpmas { namespace api { namespace graph {

	/**
	 * Generic Id API.
	 */
	template<typename IdImpl>
	class Id {
		public:
			/**
			 * Increments the id.
			 *
			 * Successive calls to this operator are expected to yield a sequence
			 * of unique ids.
			 *
			 * @return reference to this
			 */
			virtual IdImpl& operator++() = 0;

			/**
			 * Checks if two ids are equal.
			 *
			 * @param id other id to compare
			 * @return true iff `id` is equal to this
			 */
			virtual bool operator==(const IdImpl& id) const = 0;

			/**
			 * Id hash function.
			 *
			 * @return id hash
			 */
			virtual std::size_t hash() const = 0;

			/**
			 * Id string conversion.
			 */
			virtual operator std::string() const = 0;

			virtual ~Id() {}
	};

	/**
	 * Functionnal object that can be used as an
	 * [Hash](https://en.cppreference.com/w/cpp/named_req/Hash) function in
	 * standard containers.
	 */
	template<typename IdImpl>
	struct IdHash {
		/**
		 * Returns id.hash().
		 *
		 * @param id id to hash
		 * @return id hash
		 *
		 * @see Id::hash()
		 */
		std::size_t operator()(const IdImpl& id) const {
			return id.hash();
		}
	};
}}}
#endif
