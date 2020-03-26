#ifndef ZOLTAN_UTILS_H
#define ZOLTAN_UTILS_H

#include "zoltan_cpp.h"
#include "../distributed_id.h"

#define ZOLTAN_NUM_OBJ_ARGS void*, int*
#define ZOLTAN_OBJ_LIST_ARGS void*, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int, float *, int *
#define ZOLTAN_NUM_EDGES_MULTI_ARGS void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
#define ZOLTAN_EDGE_LIST_MULTI_ARGS	void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, ZOLTAN_ID_PTR, int *, int, float *, int *ierr

#define ZOLTAN_OBJ_SIZE_ARGS void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
#define ZOLTAN_PACK_OBJ_ARGS void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *
#define ZOLTAN_UNPACK_OBJ_ARGS void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
#define ZOLTAN_MID_POST_MIGRATE_ARGS \
	void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, \
    int *, int , ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *

namespace FPMAS::graph::parallel {
	class DistributedId;
}

using FPMAS::graph::parallel::DistributedId;

/**
 * Zoltan utility functions.
 */
namespace FPMAS::graph::parallel::zoltan::utils {

	/**
	 * Convenient function to rebuild a regular node or arc id, as an
	 * unsigned long, from a ZOLTAN_ID_PTR global id array, that actually
	 * stores 2 unsigned int for each node unsigned long id.
	 * So, with our configuration, we use 2 unsigned int in Zoltan to
	 * represent each id. The `i` input parameter should correspond to the
	 * first part index of the node to query in the `global_ids` array. In
	 * consequence, `i` should actually always be even.
	 *
	 * @param global_ids an adress to a pair of Zoltan global ids
	 * @return rebuilt node id
	 */
	DistributedId read_zoltan_id(const ZOLTAN_ID_PTR global_ids);

	/**
	 * Writes zoltan global ids to the specified `global_ids` adress.
	 *
	 * The original `unsigned long` is decomposed into 2 `unsigned int` to
	 * fit the default Zoltan data structure. The written id can then be
	 * rebuilt using the node_id(const ZOLTAN_ID_PTR) function.
	 *
	 * @param id id to write
	 * @param global_ids adress to a Zoltan global_ids array
	 */
	void write_zoltan_id(DistributedId id, ZOLTAN_ID_PTR global_ids);


	/**
	 * A pack of zoltan query functions that vary depending on the
	 * synchronization mode used.
	 *
	 * Possible used functions are defined in FPMAS::graph::zoltan.
	 */
	class zoltan_query_functions {
		public:
			/**
			 * Node post migrate function.
			 */
			ZOLTAN_POST_MIGRATE_PP_FN* node_post_migrate_fn;
			/**
			 * Arc post migrate function.
			 */
			ZOLTAN_POST_MIGRATE_PP_FN* arc_post_migrate_fn;
			/**
			 * Arc mid migrate function.
			 */
			ZOLTAN_MID_MIGRATE_PP_FN* arc_mid_migrate_fn;

			/**
			 * Constructor.
			 *
			 * @param node_post_migrate_fn Node post migrate function
			 * @param arc_post_migrate_fn Arc post migrate function
			 * @param arc_mid_migrate_fn Arc mid migrate function
			 */
			zoltan_query_functions(
					ZOLTAN_POST_MIGRATE_PP_FN* node_post_migrate_fn,
					ZOLTAN_POST_MIGRATE_PP_FN* arc_post_migrate_fn,
					ZOLTAN_MID_MIGRATE_PP_FN* arc_mid_migrate_fn
					) :
				node_post_migrate_fn(node_post_migrate_fn),
				arc_post_migrate_fn(arc_post_migrate_fn),
				arc_mid_migrate_fn(arc_mid_migrate_fn) {}
	};
}
#endif
