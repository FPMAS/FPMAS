#ifndef LOCATION_MANAGER_H
#define LOCATION_MANAGER_H

#include "api/graph/parallel/location_manager.h"
#include "api/graph/parallel/location_state.h"

#include "communication/communication.h"

namespace FPMAS::graph::parallel {

	using FPMAS::api::graph::parallel::LocationState;

	template<typename DistNode, template<typename> class Mpi>
		class LocationManager
		: public FPMAS::api::graph::parallel::LocationManager<DistNode> {
			public:
				using typename FPMAS::api::graph::parallel::LocationManager<DistNode>::NodeMap;
			private:
				typedef FPMAS::api::communication::MpiCommunicator MpiComm;
				MpiComm& communicator;
				Mpi<DistributedId> distIdMpi {communicator};
				Mpi<std::pair<DistributedId, int>> location_mpi {communicator};
				std::unordered_map<DistributedId, int> managed_nodes_locations;

				NodeMap local_nodes;
				NodeMap distant_nodes;

			public:
				LocationManager(MpiComm& communicator)
					: communicator(communicator) {}

				const Mpi<DistributedId>& getDistributedIdMpi() const {return distIdMpi;}
				const Mpi<std::pair<DistributedId, int>>& getLocationMpi() const {return location_mpi;}

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

	template<typename DistNode, template<typename> class Mpi>
	void LocationManager<DistNode, Mpi>::setLocal(DistNode* node) {
			node->setLocation(communicator.getRank());
			node->setState(LocationState::LOCAL);
			this->local_nodes.insert({node->getId(), node});
			this->distant_nodes.erase(node->getId());
		}

	template<typename DistNode, template<typename> class Mpi>
	void LocationManager<DistNode, Mpi>::setDistant(DistNode* node) {
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

	template<typename DistNode, template<typename> class Mpi>
	void LocationManager<DistNode, Mpi>::remove(DistNode* node) {
		switch(node->state()) {
			case LocationState::LOCAL:
				local_nodes.erase(node->getId());
				break;
			case LocationState::DISTANT:
				distant_nodes.erase(node->getId());
				break;
		}
	}

	template<typename DistNode, template<typename> class Mpi>
		void LocationManager<DistNode, Mpi>::updateLocations(
				const NodeMap& local_nodes_to_update
				) {
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
				node.second->setLocation(communicator.getRank());

				if(node.first.rank() != communicator.getRank()) {
					// Notify node origin that node is currently on this proc
					exported_updated_locations[node.first.rank()]
						.push_back(node.first);
				} else {
					// No need to export the new node location to itself :
					// just locally set this node location to this proc
					managed_nodes_locations[node.first]
						= communicator.getRank();
				}
			}
			
			// Export / import location updates of managed_nodes_locations
			DistributedIdMap imported_updated_locations =
				distIdMpi.migrate(exported_updated_locations);
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
				if(node.first.rank() == communicator.getRank()) {
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
				if(node.first.rank() != communicator.getRank()) {
					// For distant_nodes that has an origin different from this
					// proc, asks for the current location of node to its
					// origin proc
					locationRequests[node.first.rank()]
						.push_back(node.first);
				}
			}
			// Export / import requests
			DistributedIdMap importedLocationRequests
				= distIdMpi.migrate(locationRequests);

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
		}

	template<typename Node>
	using DefaultLocationManager = LocationManager<Node, communication::TypedMpi>;
}

#endif
