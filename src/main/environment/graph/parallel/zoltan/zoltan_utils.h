#ifndef ZOLTAN_UTILS_H
#define ZOLTAN_UTILS_H

#include "zoltan_cpp.h"

namespace FPMAS {
	namespace graph {
		namespace zoltan {
			namespace utils {

			unsigned long read_zoltan_id(const ZOLTAN_ID_PTR);

			void write_zoltan_id(unsigned long, ZOLTAN_ID_PTR);
			}
		}
	}
}
#endif
