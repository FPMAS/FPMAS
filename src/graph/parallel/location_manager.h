#ifndef LOCATION_MANAGER_H
#define LOCATION_MANAGER_H

#include "api/graph/parallel/location_manager.h"
#include "api/graph/parallel/location_state.h"

#include "communication/communication.h"

namespace fpmas::graph::parallel {

	using fpmas::api::graph::parallel::LocationState;

	template<typename T>
		class LocationManager
		: public fpmas::api::graph::parallel::LocationManager<T> {
			public:
				using typename fpmas::api::graph::parallel::LocationManager<T>::NodeMap;
				using typename fpmas::api::graph::parallel::LocationManager<T>::DistNode;

				typedef api::communication::MpiCommunicator MpiComm;
				typedef api::communication::TypedMpi<DistributedId> IdMpi;
				typedef api::communication::TypedMpi<std::pair<DistributedId, int>> LocationMpi;
			private:
				MpiComm& comm;
				IdMpi& id_mpi;
				LocationMpi& location_mpi;
				std::unordered_map<DistributedId, int> managed_nodes_locations;

				NodeMap local_nodes;
				NodeMap distant_nodes;

			public:
				LocationManager(MpiComm& comm, IdMpi& id_mpi, LocationMpi& location_mpi)
					: comm(comm), id_mpi(id_mpi), location_mpi(location_mpi) {}

				void addManagedNode(DistNode* node, int initialLocation) override {
					this->managed_nodes_locations[node->getId()] = initialLocation;
				}

				void removeManagedNode(DistNode* node) override {
					this->managed_nodes_locations.erase(node->getId());
				}

				void setLocal(DistNode*) override;
				void setDistant(DistNode*) override;
				void remove(DistNode*) override;

				const NodeMap& getLocalNodes() const override {return local_nodes;}
				const NodeMap& getDistantNodes() const override {return distant_nodes;}

				void updateLocations(const NodeMap& local_nodes_to_update) override;

				const std::unordered_map<DistributedId, int>& getCurrentLocations() const {
					return managed_nodes_locations;
				}
		};

	template<typename T>
	void LocationManager<T>::setLocal(DistNode* node) {
			node->setLocation(comm.getRank());
			node->setState(LocationState::LOCAL);
			this->local_nodes.insert({node->getId(), node});
			this->distant_nodes.erase(node->getId());
		}

	template<typename T>
	void LocationManager<T>::setDistant(DistNode* node) {
			node->setState(LocationState::DISTANT);
			this->local_nodes.erase(node->getId());
			this->distant_nodes.insert({node->getId(), node});
			for(auto arc : node->getOutgoingArcs()) {
				arc->setState(LocationState::DISTANT);
			}
			for(auto arc : node->getIncomingArcs()) {
				arc->setState(LocationState::DISTANT);
			}
		}

	template<typename T>
	void LocationManager<T>::remove(DistNode* node) {
		switch(node->state()) {
			case LocationState::LOCAL:
				local_nodes.erase(node->getId());
				break;
			case LocationState::DISTANT:
				distant_nodes.erase(node->getId());
				break;
		}
	}

	template<typename T>
		void LocationManager<T>::updateLocations(
				const NodeMap& local_nodes_to_update
				) {
			FPMAS_LOGD(comm.getRank(), "LOCATION_MANAGER", "Updating node locations...");
			// Useful types
			typedef 
			std::unordered_map<int, std::vector<DistributedId>>
			DistributedIdMap;
			typedef 
			std::unordered_map<int, std::vector<std::pair<DistributedId, int>>>
			LocationMap;

			/*
			 * Step 1 : send updated locations to origins, and receive
			 * locations of this origin's nodes
			 */
			DistributedIdMap exported_updated_locations;
			for(auto node : local_nodes_to_update) {
				// Updates local node field
				node.second->setLocation(comm.getRank());

				if(node.first.rank() != comm.getRank()) {
					// Notify node origin that node is currently on this proc
					exported_updated_locations[node.first.rank()]
						.push_back(node.first);
				} else {
					// No need to export the new node location to itself :
					// just locally set this node location to this proc
					managed_nodes_locations[node.first]
						= comm.getRank();
				}
			}
			
			// Export / import location updates of managed_nodes_locations
			DistributedIdMap imported_updated_locations =
				id_mpi.migrate(exported_updated_locations);
			// Updates the managed_nodes_locations data
			for(auto list : imported_updated_locations) {
				for(auto node_id : list.second) {
					managed_nodes_locations[node_id]
						= list.first;
				}
			}

			// If some distant node has this proc has origin, the current
			// location has already been updated and won't be requested to this
			// proc himself
			for(auto node : distant_nodes) {
				if(node.first.rank() == comm.getRank()) {
					node.second->setLocation(
							managed_nodes_locations[node.first]
							);
				}
			}

			/*
			 * Step 2 : ask for current locations of distant_nodes
			 */
			DistributedIdMap locationRequests;
			for(auto node : distant_nodes) {
				if(node.first.rank() != comm.getRank()) {
					// For distant_nodes that has an origin different from this
					// proc, asks for the current location of node to its
					// origin proc
					locationRequests[node.first.rank()]
						.push_back(node.first);
				}
			}
			// Export / import requests
			DistributedIdMap importedLocationRequests
				= id_mpi.migrate(locationRequests);

			// Builds requests response
			LocationMap exportedLocations;
			for(auto list : importedLocationRequests) {
				for(auto node_id : list.second) {
					// for each requested node location, respond a tuple
					// {node_id, currentLocation} to the requesting proc
					exportedLocations[list.first]
						.push_back({node_id, managed_nodes_locations[node_id]});
				}
			}

			// Import / export responses
			LocationMap imported_locations
				= location_mpi.migrate(exportedLocations);

			// Finally, updates the distant_nodes locations from the requests
			// responses
			for(auto list : imported_locations) {
				for(auto location : list.second) {
					distant_nodes.at(location.first)->setLocation(location.second);
				}
			}
			FPMAS_LOGD(comm.getRank(), "LOCATION_MANAGER", "Node locations updated.");
		}
}

#endif
