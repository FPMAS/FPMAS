#include "proxy.h"
#include "utils/config.h"
#include "../zoltan/zoltan_utils.h"
#include <nlohmann/json.hpp>

using FPMAS::graph::proxy::Proxy;
using FPMAS::graph::zoltan::utils::read_zoltan_id;
using FPMAS::graph::zoltan::utils::write_zoltan_id;
using nlohmann::json;

namespace FPMAS {
	namespace graph {
		namespace proxy {

			/**
			 * Proxy constructor.
			 *
			 * The input local proc (i.e. the current MPI rank) of the proxy
			 * could be computed internally, but passing it as argument allows
			 * the value to be defined as const.
			 *
			 * @param localProc current MPI rank
			 */
			Proxy::Proxy(int localProc) : localProc(localProc) {
				this->zoltan = Zoltan(this->mpiCommunicator.getMpiComm());

				// Apply general configuration, even if load balancing won't be used with
				// this instance
				FPMAS::config::zoltan_config(&this->zoltan);

				this->zoltan.Set_Obj_Size_Multi_Fn(obj_size_multi_fn, this);
				this->zoltan.Set_Pack_Obj_Multi_Fn(pack_obj_multi_fn, this);
				this->zoltan.Set_Unpack_Obj_Multi_Fn(unpack_obj_multi_fn, this);
			}

			/**
			 * Builds a proxy managing the specified ranks.
			 *
			 * Because the proxy instanciates a new MpiCommunicator, this
			 * constructor follows the same requirements as
			 * MpiCommunicator(std::initializer_list<int>).
			 *
			 * @param localProc current MPI rank
			 * @param ranks ranks to include in this proxy
			 */
			Proxy::Proxy(int localProc, std::initializer_list<int> ranks) : localProc(localProc), mpiCommunicator(ranks) {

				int rank;
				MPI_Comm_rank(this->mpiCommunicator.getMpiComm(), &rank);
				this->zoltan = Zoltan(this->mpiCommunicator.getMpiComm());

				// Apply general configuration, even if load balancing won't be used with
				// this instance
				FPMAS::config::zoltan_config(&zoltan);

				this->zoltan.Set_Obj_Size_Multi_Fn(obj_size_multi_fn, this);
				this->zoltan.Set_Pack_Obj_Multi_Fn(pack_obj_multi_fn, this);
				this->zoltan.Set_Unpack_Obj_Multi_Fn(unpack_obj_multi_fn, this);
			}

			/**
			 * Returns the localProc associated to this proxy, that should
			 * correspond to the current MPI rank.
			 *
			 * @return local proc
			 */
			int Proxy::getLocalProc() const {
				return this->localProc;
			}

			/**
			 * Sets the origin of the node corresponding to the specified id to proc.
			 *
			 * @param id node id
			 * @param proc origin rank of the corresponding node
			 */
			void Proxy::setOrigin(unsigned long id, int proc) {
				this->origins[id] = proc;
			}

			/**
			 * Returns the origin rank of the node corresponding to the specified id.
			 *
			 * The origin of node is actually used as the reference proxy from which we can
			 * get the updated current location of the node.
			 *
			 * `localProc` is returned if the origin of the node is the current proc.
			 *
			 * @param id node id
			 */
			int Proxy::getOrigin(unsigned long id) {
				if(this->origins.count(id) == 1)
					return this->origins.at(id);

				return this->localProc;
			}

			/**
			 * Sets the current location of the node corresponding to the specified id to
			 * proc.
			 *
			 * @param id node id
			 * @param proc current location rank of the corresponding node
			 */
			void Proxy::setCurrentLocation(unsigned long id, int proc) {
				if(proc == this->localProc) {
					this->setLocal(id);
				} else {
					this->currentLocations[id] = proc;
				}
			}

			/**
			 * Returns the current location of the node corresponding to the specified id.
			 *
			 * If the Proxy is properly synchronized, it is ensured that the node is
			 * really living on the returned proc, what means that its associated data can
			 * be asked directly to that proc.
			 *
			 * @param id node id
			 */
			int Proxy::getCurrentLocation(unsigned long id) {
				if(this->currentLocations.count(id) == 1)
					return this->currentLocations.at(id);
				return this->localProc;
			}

			/**
			 * Defines the current proc as the current location of the node
			 * corresponding to the specified id.
			 *
			 * A `true` value for upToDate might be used when the next call to
			 * synchronize() does not need to send update information for this
			 * node, typically when the node has just been imported locally
			 * directly from its origin (in this case, the origin already has
			 * an updated information about the node's current location). In any
			 * other case, or in case of doubt, let `upToDate` to its default
			 * `false` value.
			 *
			 * @param id id of the local node
			 * @param upToDate true if no update information needs to be called
			 * for this node
			 */
			void Proxy::setLocal(unsigned long id, bool upToDate) {
				if(!upToDate)
					this->updates.insert(id);

				this->currentLocations.erase(id);
			}

			/**
			 * Updates proxies data with actual nodes current locations.
			 *
			 * Each process will be waiting until everybody calls this
			 * function.
			 *
			 * Updates are performed in two steps.
			 * 1. Each process sends the current location of its local nodes
			 * (so that this location is obviously the current process) to the
			 * corresponding origin processes. In the same step, each origin
			 * imports locations and updates its current locations table.
			 *
			 * 2. For each node not contained in the current process, the proxy
			 * asks for their current location to the corresponding origin,
			 * that is now updated from step 1.
			 *
			 * This function should be called by all the processes each time a
			 * node might have migrated somewhere in the global execution, when
			 * load balancing is performed for example. If this requirement is
			 * met, the implementation ensures that all the simulation proxies
			 * contain updated information about each node location, and so
			 * can contact them or ask their data directly.
			 */
			void Proxy::synchronize() {
				ZOLTAN_ID_TYPE export_global_ids[updates.size() * 2];
				int export_procs[updates.size()];

				int i = 0;
				for(auto id : this->updates) {
					write_zoltan_id(id, &export_global_ids[i * 2]);
					export_procs[i] = this->getOrigin(id);
					i++;
				}

				this->zoltan.Migrate(
						-1,
						NULL,
						NULL,
						NULL,
						NULL,
						this->updates.size(),
						export_global_ids,
						NULL,
						export_procs,
						export_procs // proc = part
						);

				this->updates.clear();

				ZOLTAN_ID_TYPE import_global_ids[currentLocations.size() * 2];
				int import_procs[currentLocations.size()];

				int j = 0;
				for(auto loc : currentLocations) {
					write_zoltan_id(loc.first, &import_global_ids[j * 2]);
					import_procs[j] = this->getOrigin(loc.first);
					j++;
				}

				this->zoltan.Migrate(
						this->currentLocations.size(),
						import_global_ids,
						NULL,
						import_procs,
						import_procs, // proc = part
						-1,
						NULL,
						NULL,
						NULL,
						NULL
						);
			}

			/**
			 * Computes the buffer sizes required to serialize proxy entries corresponding
			 * to global_ids.
			 *
			 * For more information about this function, see the [Zoltan
			 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_OBJ_SIZE_MULTI_FN).
			 *
			 * @param data user data (local proxy)
			 * @param num_gid_entries number of entries used to describe global ids (should be 2)
			 * @param num_lid_entries number of entries used to describe local ids (should be 0)
			 * @param num_ids number of entries to serialize
			 * @param global_ids global ids of nodes to update
			 * @param local_ids unused
			 * @param sizes Result : buffer sizes for each proxy entry
			 * @param ierr Result : error code
			 */
			void obj_size_multi_fn(
					void *data,
					int num_gid_entries,
					int num_lid_entries,
					int num_ids,
					ZOLTAN_ID_PTR global_ids,
					ZOLTAN_ID_PTR local_ids,
					int *sizes,
					int *ierr) {
				Proxy* proxy = (Proxy*) data;

				for (int i = 0; i < num_ids; ++i) {
					unsigned long nodeId = read_zoltan_id(&global_ids[i * num_gid_entries]);
					// Updates to export are computed so that each proxy only
					// export updates for nodes currently living in the local proc
					std::string proxyUpdateStr = std::to_string(proxy->getCurrentLocation(nodeId));

					sizes[i] = proxyUpdateStr.size() + 1;
				}
			}

			/**
			 * Serializes the input list of node locations.
			 *
			 * For more information about this function, see the [Zoltan
			 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_PACK_OBJ_MULTI_FN).
			 *
			 * @param data user data (local proxy)
			 * @param num_gid_entries number of entries used to describe global ids (should be 2)
			 * @param num_lid_entries number of entries used to describe local ids (should be 0)
			 * @param num_ids number of proxy entries to pack
			 * @param global_ids global ids of nodes to update
			 * @param local_ids unused
			 * @param dest destination part numbers
			 * @param sizes buffer sizes for each object
			 * @param idx each object starting point in buf
			 * @param buf communication buffer
			 * @param ierr Result : error code
			 */
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
					int *ierr) {
					Proxy* proxy = (Proxy*) data;
					for (int i = 0; i < num_ids; ++i) {
						// Rebuilt node id
						unsigned long id = read_zoltan_id(&global_ids[i * num_gid_entries]);
						std::string locStr = std::to_string(proxy->getCurrentLocation(id));
						std::sprintf(&buf[idx[i]], "%s", locStr.c_str());
					}
			}

			/**
			 * Deserializes received updates to the local proxy.
			 *
			 * For more information about this function, see the [Zoltan
			 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_UNPACK_OBJ_MULTI_FN).
			 *
			 * @param data user data (local proxy)
			 * @param num_gid_entries number of entries used to describe global ids (should be 2)
			 * @param num_ids number of nodes to update
			 * @param global_ids item global ids
			 * @param sizes buffer sizes for each object
			 * @param idx each object starting point in buf
			 * @param buf communication buffer
			 * @param ierr Result : error code
			 */
			void unpack_obj_multi_fn(
					void *data,
					int num_gid_entries,
					int num_ids,
					ZOLTAN_ID_PTR global_ids,
					int *sizes,
					int *idx,
					char *buf,
					int *ierr) {
				Proxy* proxy = (Proxy*) data;

				for (int i = 0; i < num_ids; ++i) {
					unsigned long nodeId = read_zoltan_id(&global_ids[i * num_gid_entries]);

					std::string locStr = std::string(&buf[idx[i]]);
					int currentLocation = std::stoi(locStr);
					proxy->setCurrentLocation(nodeId, currentLocation);
				}
			}
		}
	}
}

