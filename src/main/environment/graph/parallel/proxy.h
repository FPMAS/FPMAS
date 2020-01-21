#ifndef PROXY_H
#define PROXY_H

#include <unordered_map>
#include <set>

#include "zoltan_cpp.h"
#include "communication/communication.h"

using FPMAS::communication::MpiCommunicator;

namespace FPMAS {
	namespace graph {
		/**
		 * Namespace containing classes and functions to manage proxies.
		 */
		namespace proxy {

			void obj_size_multi_fn(
					void *data,
					int num_gid_entries,
					int num_lid_entries,
					int num_ids,
					ZOLTAN_ID_PTR global_ids,
					ZOLTAN_ID_PTR local_ids,
					int *sizes,
					int *ierr); 

			void pack_obj_multi_fn(
					void *data,
					int num_gid_entries,
					int num_lid_entries,
					int num_ids,
					ZOLTAN_ID_PTR global_ids,
					ZOLTAN_ID_PTR local_ids,
					int *dest,
					int *sizes,
					int *idx,
					char *buf,
					int *ierr); 

			void unpack_obj_multi_fn(
					void *data,
					int num_gid_entries,
					int num_ids,
					ZOLTAN_ID_PTR global_ids,
					int *sizes,
					int *idx,
					char *buf,
					int *ierr); 

			/**
			 * The proxy can be used to locate ANY nodes currently accessible
			 * from the associated DistributedGraph, including GhostNodes.
			 *
			 * The corresponding maps are maintained externally at the
			 * DistributedGraph scale, when nodes are imported and exported
			 * using Zoltan.
			 *
			 * The Proxy must be synchronized at each step to maintain and
			 * update the current location map using the Zoltan migrations
			 * functions.
			 */
			class Proxy {
				friend void proxy::obj_size_multi_fn(
					void *data, int num_gid_entries, int num_lid_entries, int num_ids,
					ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids, int *sizes, int *ierr
					); 
				friend void proxy::pack_obj_multi_fn(
					void *data, int num_gid_entries, int num_lid_entries, int num_ids,
					ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids, int *dest, int *sizes,
					int *idx, char *buf, int *ierr
					);
				friend void proxy::unpack_obj_multi_fn(
					void *data, int num_gid_entries, int num_ids,
					ZOLTAN_ID_PTR global_ids, int *sizes,
					int *idx, char *buf, int *ierr
					);

				private:
					MpiCommunicator mpiCommunicator;
					Zoltan zoltan;
					const int localProc;
					std::unordered_map<unsigned long, int> origins;
					std::unordered_map<unsigned long, int> currentLocations;

					std::unordered_map<unsigned long, std::string> proxyUpdatesSerializationCache;

					// Contains the current key that should be updated at the
					// next synchronize() call
					std::set<unsigned long> updates;

				public:
					Proxy(int localProc);
					Proxy(int localProc, std::initializer_list<int>);
					int getLocalProc() const;
					void setOrigin(unsigned long, int);
					void setCurrentLocation(unsigned long, int);
					int getOrigin(unsigned long);
					int getCurrentLocation(unsigned long);
					void setLocal(unsigned long, bool upToDate=false);

					void synchronize();

			};

		}
	}
}

#endif
