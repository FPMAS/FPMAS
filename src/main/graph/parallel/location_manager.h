#ifndef LOCATION_MANAGER_H
#define LOCATION_MANAGER_H

#include "api/communication/communication.h"
#include "api/graph/parallel/location_manager.h"

namespace FPMAS::graph::parallel {

	template<typename DistNode>
		class LocationManager
		: public FPMAS::api::graph::parallel::LocationManager<DistNode> {
			private:
				typedef FPMAS::api::communication::MpiCommunicator MpiComm;
				MpiComm& communicator;
				std::unordered_map<DistributedId, int> managedNodesLocations;

			public:
				using typename FPMAS::api::graph::parallel::LocationManager<DistNode>::node_map;

				LocationManager(MpiComm& communicator)
					: communicator(communicator) {}

				void addManagedNode(DistNode* node, int initialLocation) override {
					this->managedNodesLocations[node->getId()] = initialLocation;
				}

				void removeManagedNode(DistNode* node) override {
					this->managedNodesLocations.erase(node->getId());
				}

				void updateLocations(
						node_map localNodes,
						node_map distantNodes
						) override;

				const std::unordered_map<DistributedId, int>& getCurrentLocations() const {
					return managedNodesLocations;
				}
		};

	template<typename DistNode>
		void LocationManager<DistNode>::updateLocations(
				node_map localNodes,
				node_map distantNodes
				) {
			// Useful types
			typedef 
			std::unordered_map<int, std::vector<DistributedId>>
			DistributedIdMap;
			typedef 
			std::unordered_map<int, std::vector<std::pair<DistributedId, int>>>
			LocationMap;
			using FPMAS::api::communication::migrate;

			/*
			 * Step 1 : send updated locations to origins, and receive
			 * locations of this origin's nodes
			 */
			DistributedIdMap exportedUpdatedLocations;
			for(auto node : localNodes) {
				// Updates local node field
				node.second->setLocation(communicator.getRank());

				if(node.first.rank() != communicator.getRank()) {
					// Notify node origin that node is currently on this proc
					exportedUpdatedLocations[node.first.rank()]
						.push_back(node.first);
				} else {
					// No need to export the new node location to itself :
					// just locally set this node location to this proc
					managedNodesLocations[node.first]
						= communicator.getRank();
				}
			}
			
			// Export / import location updates of managedNodesLocations
			DistributedIdMap importedUpdatedLocations =
				migrate(communicator, exportedUpdatedLocations);
			// Updates the managedNodesLocations data
			for(auto list : importedUpdatedLocations) {
				for(auto nodeId : list.second) {
					managedNodesLocations[nodeId]
						= list.first;
				}
			}

			// If some distant node has this proc has origin, the current
			// location has already been updated and won't be requested to this
			// proc himself
			for(auto node : distantNodes) {
				if(node.first.rank() == communicator.getRank()) {
					node.second->setLocation(
							managedNodesLocations[node.first]
							);
				}
			}

			/*
			 * Step 2 : ask for current locations of distantNodes
			 */
			DistributedIdMap locationRequests;
			for(auto node : distantNodes) {
				if(node.first.rank() != communicator.getRank()) {
					// For distantNodes that has an origin different from this
					// proc, asks for the current location of node to its
					// origin proc
					locationRequests[node.first.rank()]
						.push_back(node.first);
				}
			}
			// Export / import requests
			DistributedIdMap importedLocationRequests
				= migrate(communicator, locationRequests);

			// Builds requests response
			LocationMap exportedLocations;
			for(auto list : importedLocationRequests) {
				for(auto nodeId : list.second) {
					// for each requested node location, respond a tuple
					// {nodeId, currentLocation} to the requesting proc
					exportedLocations[list.first]
						.push_back({nodeId, managedNodesLocations[nodeId]});
				}
			}

			// Import / export responses
			LocationMap importedLocations
				= migrate(communicator, exportedLocations);

			// Finally, updates the distantNodes locations from the requests
			// responses
			for(auto list : importedLocations) {
				for(auto location : list.second) {
					distantNodes.at(location.first)->setLocation(location.second);
				}
			}
		}
}

#endif
