#ifndef FPMAS_DISTRIBUTED_ID_H
#define FPMAS_DISTRIBUTED_ID_H

#include "hedley/hedley.h"

/** \file src/fpmas/api/graph/distributed_id.h
 * DistributedId implementation.
 */

#ifndef FPMAS_ID_TYPE
	/**
	 * Type used to represent the "id" part of an fpmas::api::graph::DistributedId.
	 *
	 * This type defines the maximum count of Nodes and Edges that might be created
	 * during a single simulation. (e.g. 65535 for a 16 bit unsigned integer)
	 *
	 * It can be user defined at compile time using
	 * `cmake -DFPMAS_ID_TYPE=<unsigned integer type> ..`
	 *
	 * By default, the `std::uint_fast32_t` type is used, that can be 32 or 64 bit
	 * depending on the fastest unsigned integer type of at least 32 bits that
	 * can be processed by the current system.
	 *
	 * Any unsigned integer type can be specified. The std::uintN_t like
	 * types defined in the [C++
	 * standard](https://en.cppreference.com/w/cpp/types/integer) can notably
	 * be used to fix the \FPMAS_ID_TYPE size independently of the underlying
	 * system.
	 *
	 * FPMAS currently supports 16, 32 and 64 unsigned integer types.
	 */
	#define FPMAS_ID_TYPE std::uint_fast32_t
#endif

#include <functional>
#include <nlohmann/json.hpp>
#include <mpi.h>
#include <climits>
#include <type_traits>

static_assert(
		std::is_integral<FPMAS_ID_TYPE>::value,
		"FPMAS_ID_TYPE must be an unsigned integer."
		);
static_assert(
		std::is_unsigned<FPMAS_ID_TYPE>::value,
		"FPMAS_ID_TYPE must be unsigned."
		);


namespace fpmas { namespace api { namespace graph {
	class DistributedId;
}}}

namespace fpmas { namespace api { namespace communication {
	/**
	 * Structure used to serialize / unserialize DistributedId instances in MPI
	 * communications.
	 *
	 * The MPI_Datatype used is defined in DistributedId::mpiDistributedIdType.
	 */
	struct MpiDistributedId {
		/**
		 * MPI rank
		 */
		int rank;
		/**
		 * Local id
		 */
		FPMAS_ID_TYPE id;
	};
}}}

namespace fpmas { namespace api { namespace graph {
	class DistributedId;
}}}

using fpmas::api::communication::MpiDistributedId;

namespace fpmas { namespace api { namespace graph {

	/**
	 * Distributed `IdType` implementation, used by any \DistributedGraph.
	 *
	 * The id is represented by a pair constituted by:
	 * 1. The rank on which the item where created
	 * 2. An incrementing local id of type \FPMAS_ID_TYPE
	 */
	class DistributedId {
		friend nlohmann::adl_serializer<DistributedId>;

		private:
			int _rank;
			FPMAS_ID_TYPE _id;

		public:
			/**
			 * The maximum rank that can be represented.
			 */
			static int max_rank;
			/**
			 * The maximum id that can be represented.
			 *
			 * This can be controlled defining a custom \FPMAS_ID_TYPE.
			 *
			 * Overflows, that might occur if the \FPMAS_ID_TYPE is not large
			 * enough to represent ids of all \DistributedNodes and
			 * \DistributedEdges created during a single simulation, will
			 * produce unexpected behaviors.
			 */
			static FPMAS_ID_TYPE max_id;

			/**
			 * MPI_Datatype used to send and receive DistributedIds.
			 *
			 * Should be initialized with fpmas::communication::createMpiTypes
			 * and freed with fpmas::communication::freeMpiTypes
			 */
			static MPI_Datatype mpiDistributedIdType;

			/**
			 * Converts an MpiDistributedId instance into a DistributedId.
			 *
			 * @param mpi_id MpiDistributedId to convert
			 */
			explicit DistributedId(const MpiDistributedId& mpi_id)
				: _rank(mpi_id.rank), _id(mpi_id.id) {}

			/**
			 * Default DistributedId constructor.
			 */
			DistributedId() : DistributedId(0, 0) {}

			/**
			 * Initializes a DistributedId instance for the specified proc.
			 *
			 * Local id starts from 0.
			 *
			 * @param rank MPI rank
			 */
			explicit DistributedId(int rank) : DistributedId(rank, 0) {}

			/**
			 * Initializes a DistributedId instance from the specified rank and
			 * id.
			 *
			 * @param rank MPI rank
			 * @param id initial local id value
			 */
			DistributedId(int rank, FPMAS_ID_TYPE id) : _rank(rank), _id(id) {}

			/**
			 * Rank associated to this DistributedId instance.
			 *
			 * @return MPI rank
			 */
			int rank() const {
				return _rank;
			}

			/**
			 * Current local id.
			 *
			 * @return local id
			 */
			FPMAS_ID_TYPE id() const {
				return _id;
			}

			/**
			 * Returns the current DistributedId value and increments its local
			 * id.
			 */
			DistributedId operator++(int) {
				DistributedId old(*this);
				_id++;
				return old;
			};

			/**
			 * Comparison function that allows DistributedId usage in std::set.
			 *
			 * First, perform a comparison on the `rank` of the two instances,
			 * and return the result if one is less or greater than the other.
			 *
			 * If the two ranks are equal, return the result of the comparison
			 * of the local ids.
			 */
			bool operator<(const DistributedId& other) const {
				if(this->_rank==other._rank)
					return this->_id < other._id;
				if(this->_rank < other._rank)
					return true;
				return false;
			}

			/**
			 * DistributedId comparison function, consistent with the
			 * operator==() and operator<() definitions.
			 */
			bool operator<=(const DistributedId& other) const {
				if(*this == other)
					return true;
				return *this < other;
			}

			/**
			 * Two DistributedId instances are equal iff their ranks and their
			 * ids are equal.
			 *
			 * @return comparison result
			 */
			bool operator==(const DistributedId& other) const {
				return (other._rank == this->_rank) && (other._id == this->_id);
			}

			/**
			 * Two DistributedId instances are not equal iff their ranks or their
			 * ids are different.
			 *
			 * @return comparison result
			 */
			bool operator!=(const DistributedId& other) const {
				return (other._rank != this->_rank) || (other._id != this->_id);
			}

			/**
			 * @deprecated
			 *
			 * std::string conversion.
			 */
			HEDLEY_DEPRECATED(1.1)
			//TODO: Removes this in 2.0 (conflicts with nlohmann::json, when
			//declaring std::map<DistributedId, _>)
			operator std::string() const {
				return "[" + std::to_string(_rank) + ":" + std::to_string(_id) + "]";
			}

			/**
			 * Computes the hash value of this DistributedId.
			 *
			 * @return hash value
			 */
			std::size_t hash() const {
				return std::hash<FPMAS_ID_TYPE>()(_id);
			}
	};

	/**
	 * DistributedId stream output operator.
	 *
	 * @param os output stream
	 * @param id id to add to the stream
	 * @return os
	 */
	std::ostream& operator<<(std::ostream& os, const DistributedId& id);
}}}

using fpmas::api::graph::DistributedId;

namespace fpmas {
	/**
	 * Converts an api::graph::DistributedId instance to a string
	 * representation, in the form `[id.rank(), id.id()]`.
	 *
	 * @param id id to convert to std::string
	 * @return string representation of id
	 */
	std::string to_string(const api::graph::DistributedId& id);
}

namespace fpmas { namespace api { namespace communication {
	/**
	 * Initializes the static DistributedId::mpiDistributedIdType instance,
	 * used to send and receive DistributedId through MPI.
	 */
	inline static void createMpiTypes() {
		MPI_Datatype types[2] = {MPI_INT, MPI_UNSIGNED_LONG_LONG};
		int block[2] = {1, 1};
		MPI_Aint disp[2] = {
			offsetof(MpiDistributedId, rank),
			offsetof(MpiDistributedId, id)
		};
		MPI_Type_create_struct(2, block, disp, types, &DistributedId::mpiDistributedIdType);
		MPI_Type_commit(&DistributedId::mpiDistributedIdType);
	}

	/**
	 * Frees the static DistributedId::mpiDistributedIdType instance.
	 */
	inline static void freeMpiTypes() {
		MPI_Type_free(&DistributedId::mpiDistributedIdType);
	}
}}}

namespace std {
	/**
	 * DistributedId hash function object.
	 */
	template<> struct hash<DistributedId> {
		/**
		 * Returns a combination of the rank and local id hashes.
		 *
		 * @param id DistributedId to hash
		 */
		std::size_t operator()(DistributedId const& id) const noexcept
		{
			return id.hash();
		}

	};
}


namespace nlohmann {
	using fpmas::api::graph::DistributedId;

	/**
	 * DistributedId serializer.
	 */
	template<>
		struct adl_serializer<DistributedId> {
			/**
			 * Serializes a DistributedId instance as [rank, localId].
			 *
			 * @param j current json
			 * @param value DistributedId to serialize
			 */
			template<typename JsonType>
			static void to_json(JsonType& j, const DistributedId& value) {
				j[0] = value.rank();
				j[1] = value.id();
			}

			/**
			 * Unserializes a DistributedId id instance from a [rank, localId]
			 * json representation.
			 *
			 * @param j json to unserialize
			 * @param id DistributedId output
			 */
			template<typename JsonType>
			static void from_json(const JsonType& j, DistributedId& id) {
				id._rank = j[0].template get<int>();
				id._id = j[1].template get<FPMAS_ID_TYPE>();
			}
		};

	/*
	 * Temporary hack to solve the DistributedId std::string conversion issue
	 *
	 * Will be removed in a next release, when DistributedId automatic
	 * std::string conversion will be disabled.
	 */
	template<typename Value, typename Hash, typename Equal, typename Alloc>
		struct adl_serializer<std::unordered_map<DistributedId, Value, Hash, Equal, Alloc>> {
			typedef std::unordered_map<DistributedId, Value, Hash, Equal, Alloc> Map;
			static void to_json(json& j, const Map& map) {
				for(auto item : map)
					j[json(item.first).dump()] = json(item.second);
			}
			static void from_json(const json& j, Map& map) {
				for(auto item : j.items()) {
					map.insert({
							json::parse(item.key()).get<DistributedId>(),
							item.value().get<Value>()});
				}
			}
		};
}
#endif
