#ifndef DISTRIBUTED_ID_H
#define DISTRIBUTED_ID_H

#include <functional>
#include <nlohmann/json.hpp>
#include "zoltan_cpp.h"
#include "zoltan/zoltan_utils.h"
#include "utils/log.h"


namespace FPMAS::graph::parallel {
	class DistributedId;
}

namespace FPMAS::graph::parallel::zoltan::utils {
	DistributedId read_zoltan_id(const ZOLTAN_ID_PTR);
	void write_zoltan_id(DistributedId, ZOLTAN_ID_PTR);
}

namespace FPMAS::communication {
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
		unsigned int id;
	};

	//class SyncMpiCommunicator;
}

namespace FPMAS::graph::parallel {
	class DistributedId;
}
namespace std {
	template<> struct hash<FPMAS::graph::parallel::DistributedId>;
}

using FPMAS::communication::MpiDistributedId;

namespace FPMAS::graph::parallel {

	/**
	 * Distributed `IdType` implementation, used by any DistibutedGraphBase.
	 *
	 * The id is represented through a pair of :
	 * 1. The rank on which the item where created
	 * 2. An incrementing local id of type `unsigned int`
	 */
	class DistributedId {
		friend std::hash<DistributedId>;
		//friend FPMAS::communication::SyncMpiCommunicator;
		friend DistributedId zoltan::utils::read_zoltan_id(const ZOLTAN_ID_PTR);
		friend void zoltan::utils::write_zoltan_id(DistributedId, ZOLTAN_ID_PTR);
		friend nlohmann::adl_serializer<DistributedId>;
		friend std::ostream& operator<<(std::ostream& os, const DistributedId& id) {
			os << std::string(id);
			return os;
		}

		private:
			int _rank;
			unsigned int _id;

		public:
			/**
			 * MPI_Datatype used to send and receive DistributedIds.
			 *
			 * Should be initialized with FPMAS::communication::createMpiTypes
			 * and freed with FPMAS::communication::freeMpiTypes
			 */
			static MPI_Datatype mpiDistributedIdType;

			explicit DistributedId(const MpiDistributedId& mpiId)
				: _rank(mpiId.rank), _id(mpiId.id) {}

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
			DistributedId(int rank, unsigned int id) : _rank(rank), _id(id) {}

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
			unsigned int id() const {
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
			 *
			 * @return comparison result
			 */
			bool operator<(const DistributedId& other) const {
				if(this->_rank==other._rank)
					return this->_id < other._id;
				if(this->_rank < other._rank)
					return true;
				return false;
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
			 * std::string conversion.
			 */
			operator std::string() const {
				return "[" + std::to_string(_rank) + ":" + std::to_string(_id) + "]";

			}

			std::size_t hash() const {
				return std::hash<unsigned long int>()(
						_id + UINT_MAX * (unsigned int) _rank
					);
			}
	};
	inline MPI_Datatype DistributedId::mpiDistributedIdType;
}

using FPMAS::graph::parallel::DistributedId;

namespace FPMAS::communication {
	/**
	 * Initializes the static DistributedId::mpiDistributedIdType instance,
	 * used to send and receive DistributedId through MPI.
	 */
	static void createMpiTypes() {
		MPI_Datatype types[2] = {MPI_INT, MPI_UNSIGNED};
		int block[2] = {1, 1};
		MPI_Aint disp[2] = {
			offsetof(MpiDistributedId, rank),
			offsetof(MpiDistributedId, id)
		};
		MPI_Type_create_struct(2, block, disp, types, &DistributedId::mpiDistributedIdType);
		int err = MPI_Type_commit(&DistributedId::mpiDistributedIdType);

	}

	/**
	 * Frees the static DistributedId::mpiDistributedIdType instance.
	 */
	static void freeMpiTypes() {
		MPI_Type_free(&DistributedId::mpiDistributedIdType);
	}
}

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

using FPMAS::graph::parallel::DistributedId;

namespace nlohmann {
	template<>
		/**
		 * DistributedId serializer.
		 */
		struct adl_serializer<DistributedId> {
			/**
			 * Serializes a DistributedId instance as [rank, localId].
			 *
			 * @param j current json
			 * @param value DistributedId to serialize
			 */
			static void to_json(json& j, const DistributedId& value) {
				j[0] = value._rank;
				j[1] = value._id;
			}

			/**
			 * Unserializes a DistributedId id instance from a [rank, localId]
			 * json representation.
			 *
			 * @param j json to unserialize
			 * @return unserialized DistributedId
			 */
			static DistributedId from_json(const json& j) {
				return DistributedId(
						j[0].get<int>(),
						j[1].get<unsigned int>()
						);
			}
		};
}

#endif
