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
	struct MpiDistributedId {
		int rank;
		unsigned int id;
	};

	class SyncMpiCommunicator;
}

namespace FPMAS::graph::parallel {
	class DistributedId;
}
namespace std {
	template<> struct hash<FPMAS::graph::parallel::DistributedId>;
}

using FPMAS::communication::MpiDistributedId;

namespace FPMAS::graph::parallel {

	class DistributedId {
		friend std::hash<DistributedId>;
		friend FPMAS::communication::SyncMpiCommunicator;
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
			DistributedId(const MpiDistributedId& mpiId)
				: _rank(mpiId.rank), _id(mpiId.id) {}

		public:
			static MPI_Datatype mpiDistributedIdType;
			DistributedId() : DistributedId(0, 0) {}
			explicit DistributedId(int rank) : DistributedId(rank, 0) {}
			DistributedId(int rank, unsigned int id) : _rank(rank), _id(id) {}

			int rank() const {
				return _rank;
			}
			unsigned int id() const {
				return _id;
			}

			DistributedId operator++(int) {
				DistributedId old(*this);
				_id++;
				return old;
			};

			bool operator<(const DistributedId& other) const {
				if(this->_rank==other._rank)
					return this->_id < other._id;
				if(this->_rank < other._rank)
					return true;
				return false;
			}

			bool operator==(const DistributedId& other) const {
				return (other._rank == this->_rank) && (other._id == this->_id);
			}

			operator std::string() const {
				return "[" + std::to_string(_rank) + ":" + std::to_string(_id) + "]";

			}
	};
	inline MPI_Datatype DistributedId::mpiDistributedIdType;
}

using FPMAS::graph::parallel::DistributedId;

namespace FPMAS::communication {
	static void initMpiTypes() {
		MPI_Datatype types[2] = {MPI_INT, MPI_UNSIGNED};
		int block[2] = {1, 1};
		MPI_Aint disp[2] = {
			offsetof(MpiDistributedId, rank),
			offsetof(MpiDistributedId, id)
		};
		MPI_Type_create_struct(2, block, disp, types, &DistributedId::mpiDistributedIdType);
		int err = MPI_Type_commit(&DistributedId::mpiDistributedIdType);

	}

	static void clearMpiTypes() {
		MPI_Type_free(&DistributedId::mpiDistributedIdType);
	}
}

namespace std {
	template<> struct hash<DistributedId> {
		std::size_t operator()(DistributedId const& id) const noexcept
		{
			std::size_t h1 = std::hash<int>{}(id._rank);
			std::size_t h2 = std::hash<int>{}(id._id);
			return h1 ^ (h2 << 1); // or use boost::hash_combine (see Discussion)
		}

	};
}

using FPMAS::graph::parallel::DistributedId;

namespace nlohmann {
	template<>
		struct adl_serializer<DistributedId> {
			static void to_json(json& j, const DistributedId& value) {
				j["rank"] = value._rank;
				j["id"] = value._id;
			}

			static DistributedId from_json(const json& j) {
				return DistributedId(
						j.at("rank").get<int>(),
						j.at("id").get<unsigned int>()
						);
			}
		};
}

#endif
