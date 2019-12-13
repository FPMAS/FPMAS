#include "proxy.h"
#include "utils/config.h"
#include "zoltan/zoltan_utils.h"
#include <nlohmann/json.hpp>

using FPMAS::graph::proxy::Proxy;
using FPMAS::graph::zoltan::utils::read_zoltan_id;
using FPMAS::graph::zoltan::utils::write_zoltan_id;
using nlohmann::json;

namespace FPMAS {
	namespace graph {
		namespace proxy {
			Proxy::Proxy(int _localProc) : localProc(_localProc) {
				this->zoltan = new Zoltan(this->mpiCommunicator.getMpiComm());
				// Apply general configuration, even if load balancing won't be used with
				// this instance
				FPMAS::config::zoltan_config(this->zoltan);

				this->zoltan->Set_Obj_Size_Multi_Fn(obj_size_multi_fn, this);
				this->zoltan->Set_Pack_Obj_Multi_Fn(pack_obj_multi_fn, this);
				this->zoltan->Set_Unpack_Obj_Multi_Fn(unpack_obj_multi_fn, this);
			}

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

			void Proxy::setLocal(unsigned long id, bool upToDate) {
				if(!upToDate)
					this->updates.insert(id);

				this->currentLocations.erase(id);
			}

			void Proxy::synchronize() {
				ZOLTAN_ID_TYPE export_global_ids[updates.size() * 2];
				int export_procs[updates.size()];

				int i = 0;
				for(auto id : this->updates) {
					write_zoltan_id(id, &export_global_ids[i * 2]);
					export_procs[i] = this->getOrigin(id);
					i++;
				}

				this->zoltan->Migrate(
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

				this->zoltan->Migrate(
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

					int currentLocation = std::stoi(std::string(&buf[idx[i]]));
					proxy->setCurrentLocation(nodeId, currentLocation);
				}
			}
		}
	}
}

