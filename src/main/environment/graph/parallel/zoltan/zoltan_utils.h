#ifndef ZOLTAN_UTILS_H
#define ZOLTAN_UTILS_H

#include "zoltan_cpp.h"

#define ZOLTAN_OBJ_SIZE_ARGS void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
#define ZOLTAN_PACK_OBJ_ARGS void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *
#define ZOLTAN_UNPACK_OBJ_ARGS void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
#define ZOLTAN_MID_POST_MIGRATE_ARGS \
	void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR , int *, \
    int *, int , ZOLTAN_ID_PTR , ZOLTAN_ID_PTR , int *, int *,int *

namespace FPMAS {
	namespace graph {
		namespace zoltan {
			namespace utils {

			unsigned long read_zoltan_id(const ZOLTAN_ID_PTR);

			void write_zoltan_id(unsigned long, ZOLTAN_ID_PTR);

			class zoltan_query_functions {
				public:
				ZOLTAN_POST_MIGRATE_PP_FN* node_post_migrate_fn;
				ZOLTAN_POST_MIGRATE_PP_FN* arc_post_migrate_fn;
				ZOLTAN_MID_MIGRATE_PP_FN* arc_mid_migrate_fn;

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
		}
	}
}
#endif
