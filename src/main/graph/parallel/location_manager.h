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
				using typename FPMAS::api::graph::parallel::LocationManager<DistNode>::node_map;
			private:
				typedef FPMAS::api::communication::MpiCommunicator MpiComm;
				MpiComm& communicator;
				Mpi<DistributedId> distIdMpi {communicator};
				Mpi<std::pair<DistributedId, int>> locationMpi {communicator};
				std::unordered_map<DistributedId, int> managedNodesLocations;

				node_map localNodes;
				node_map distantNodes;

			public:
				LocationManager(MpiComm& communicator)
					: communicator(communicator) {}

				const Mpi<DistributedId>& getDistributedIdMpi() const {return distIdMpi;}
				const Mpi<std::pair<DistributedId, int>>& getLocationMpi() const {return locationMpi;}

				void addManagedNode(DistNode* node, int initialLocation) override {
					this->managedNodesLocations[node->getId()] = initialLocation;
				}

				void removeManagedNode(DistNode* node) override {
					this->managedNodesLocations.erase(node->getId());
				}

				void setLocal(DistNode*) override;
				void setDistant(DistNode*) override;
				void remove(DistNode*) override;

				const node_map& getLocalNodes() const override {return localNodes;}
				const node_map& getDistantNodes() const override {return distantNodes;}

				void updateLocations(const node_map& localNodesToUpdate) override;

				const std::unordered_map<DistributedId, int>& getCurrentLocations() const {
					return managedNodesLocations;
				}
		};

	template<typename DistNode, template<typename> class Mpi>
	void LocationManager<DistNode, Mpi>::setLocal(DistNode* node) {
			node->setLocation(communicator.getRank());
			node->setState(LocationState::LOCAL);
			this->localNodes.insert({node->getId(), node});
			this->distantNodes.erase(node->getId());
		}

	template<typename DistNode, template<typename> class Mpi>
	void LocationManager<DistNode, Mpi>::setDistant(DistNode* node) {
			node->setState(LocationState::DISTANT);
			this->localNodes.erase(node->getId());
			this->distantNodes.insert({node->getId(), node});
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
				localNodes.erase(node->getId());
				break;
			case LocationState::DISTANT:
				distantNodes.erase(node->getId());
				break;
		}
	}

	template<typename DistNode, template<typename> class Mpi>
		void LocationManager<DistNode, Mpi>::updateLocations(
				const node_map& localNodesToUpdate
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
			DistributedIdMap exportedUpdatedLocations;
			for(auto node : localNodesToUpdate) {
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
				distIdMpi.migrate(exportedUpdatedLocations);
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
				= distIdMpi.migrate(locationRequests);

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
				= locationMpi.migrate(exportedLocations);

			// Finally, updates the distantNodes locations from the requests
			// responses
			for(auto list : importedLocations) {
				for(auto location : list.second) {
					distantNodes.at(location.first)->setLocation(location.second);
				}
			}
		}

	template<typename Node>
	using DefaultLocationManager = LocationManager<Node, communication::TypedMpi>;
}

#endif
