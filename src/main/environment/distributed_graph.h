#include "graph.h"
#include "zoltan_cpp.h"

namespace FPMAS {
	namespace graph {
		template<class T> int zoltan_num_obj(void *data, int* ierr);
		template<class T> void zoltan_obj_list(
				void *,
				int,
				int,
				ZOLTAN_ID_PTR,
				ZOLTAN_ID_PTR,
				int,
				float *,
				int *
				);

		template<class T> class DistributedGraph : public Graph<T> {
			private:
				Zoltan* zoltan;

			public:
				DistributedGraph<T>(Zoltan*);


				void distribute();

		};

		template<class T> DistributedGraph<T>::DistributedGraph(Zoltan* z)
			: Graph<T>(), zoltan(z) {
				zoltan->Set_Num_Obj_Fn(FPMAS::graph::zoltan_num_obj<T>, this);
				zoltan->Set_Obj_List_Fn(FPMAS::graph::zoltan_obj_list<T>, this);

			}

		template<class T> int zoltan_num_obj(void *data, int* ierr) {
			return ((DistributedGraph<T>*) data)->getNodes().size();
		}

		unsigned long node_id(ZOLTAN_ID_PTR global_ids, int i) {
			return ((unsigned long) (global_ids[i] << 16)) | global_ids[i+1];
		}

		// Note : all zoltan result arrays are allocated and freed by
		// Zoltan
		template<class T> void zoltan_obj_list(
				void *data, // user data
				int num_gid_entries, // number of entries used to describe global ids
				int num_lid_entries, // number of entries used to describe local ids
				ZOLTAN_ID_PTR global_ids, // Result : global ids assigned to processor
				ZOLTAN_ID_PTR local_ids, // Result : local ids assigned to processor
				int wgt_dim, // Number of weights of each object, defined by OBJ_WEIGHT_DIM
				float *obj_wgts, // Result : weights list
				int *ierr // Result : error code
				) {
			int i = 0;
			for(auto n : ((DistributedGraph<T>*) data)->getNodes()) {
				Node<T>* node = n.second;


				global_ids[2*i] = (node->getId() & 0xFFFF0000) >> 16 ;
				global_ids[2*i+1] = (node->getId() & 0xFFFF);

				obj_wgts[i++] = node->getWeight();
			}
		}

	}
}

