#include "zoltan_utils.h"

namespace FPMAS::graph::parallel::zoltan::utils {
	
	// Note : Zoltan should be able to directly use unsigned long as ids.
	// However, this require to set up compile time flags to Zoltan, so we
	// use 2 entries of the default zoltan unsigned int instead, to make it
	// compatible with any Zoltan installation.
	// Passing compile flags to the custom embedded CMake Zoltan
	// installation might also be possible.
	DistributedId read_zoltan_id(const ZOLTAN_ID_PTR global_ids) {
		return DistributedId(
			(int) global_ids[0],
			(unsigned int) global_ids[1]
			);
	}

	void write_zoltan_id(DistributedId id, ZOLTAN_ID_PTR global_ids) {
		global_ids[0] = id._rank;
		global_ids[1] = id._id;
	}


}
