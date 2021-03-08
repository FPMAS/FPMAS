#include "zoltan_load_balancing.h"

namespace fpmas { namespace graph { namespace zoltan {
	void zoltan_config(Zoltan* zz) {
		zz->Set_Param("DEBUG_LEVEL", "0");
		zz->Set_Param("LB_METHOD", "GRAPH");
		zz->Set_Param("LB_APPROACH", "PARTITION");
		zz->Set_Param("NUM_GID_ENTRIES", std::to_string(NUM_GID_ENTRIES));
		zz->Set_Param("NUM_LID_ENTRIES", "0");
		zz->Set_Param("OBJ_WEIGHT_DIM", "1");
		zz->Set_Param("EDGE_WEIGHT_DIM", "1");
		zz->Set_Param("RETURN_LISTS", "ALL");
		zz->Set_Param("CHECK_GRAPH", "0");
		zz->Set_Param("IMBALANCE_TOL", "1.02");
		zz->Set_Param("PHG_EDGE_SIZE_THRESHOLD", "1.0");
		// zz->Set_Param("DEBUG_LEVEL", "10");

		zz->Set_Param("NUM_LOCAL_PARTS", "1");
	}

	// Note : Zoltan should be able to directly use unsigned long as ids.
	// However, this require to set up compile time flags to Zoltan, so we
	// use 2 entries of the default zoltan unsigned int instead, to make it
	// compatible with any Zoltan installation.
	DistributedId read_zoltan_id(const ZOLTAN_ID_PTR global_ids) {
		// Interprets global_ids as an ID_TYPE array
		FPMAS_ID_TYPE* id = (FPMAS_ID_TYPE*) (global_ids+1);
		return DistributedId(
				(int) global_ids[0],
				*id
				);
	}

	void write_zoltan_id(DistributedId id, ZOLTAN_ID_PTR global_ids) {
		global_ids[0] = id.rank();
		// Interprets global_ids as an ID_TYPE array
		FPMAS_ID_TYPE* _id = (FPMAS_ID_TYPE*) (global_ids+1);
		*_id = id.id();
	}
}}}
