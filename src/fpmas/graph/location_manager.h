#ifndef FPMAS_LOCATION_MANAGER_H
#define FPMAS_LOCATION_MANAGER_H

/** \file src/fpmas/graph/location_manager.h
 * LocationManager implementation.
 */

#include "fpmas/api/graph/location_manager.h"

#include "fpmas/communication/communication.h"
#include "fpmas/utils/log.h"

namespace fpmas { namespace graph {

	using api::graph::LocationState;

	/**
	 * api::graph::LocationManager implementation.
	 */
	template<typename T>
		class LocationManager
		: public api::graph::LocationManager<T> {
			public:
				using typename api::graph::LocationManager<T>::NodeMap;

				/**
				 * Type used to transmit DistributedId with MPI.
				 */
				typedef api::communication::TypedMpi<DistributedId> IdMpi;
				/**
				 * Type used to transmit node locations with MPI.
				 */
				typedef api::communication::TypedMpi<std::pair<DistributedId, int>> LocationMpi;

			private:
				// MPI communicators are likely to be owned by the parent
				// DistributedGraph. In any case, their lifetimes must span the
				// LocationManager lifetime.
				api::communication::MpiCommunicator* comm;
				IdMpi* id_mpi;
				LocationMpi* location_mpi;
				std::unordered_map<DistributedId, int> managed_nodes_locations;

				NodeMap local_nodes;
				NodeMap distant_nodes;
				NodeMap new_local_nodes;

			public:
				/**
				 * LocationManager constructor.
				 *
				 * @param comm MPI communicator
				 * @param id_mpi IdMpi instance
				 * @param location_mpi LocationMpi instance
				 */
				LocationManager(api::communication::MpiCommunicator& comm, IdMpi& id_mpi, LocationMpi& location_mpi)
					: comm(&comm), id_mpi(&id_mpi), location_mpi(&location_mpi) {}

				void addManagedNode(api::graph::DistributedNode<T>* node, int initial_location) override {
					this->managed_nodes_locations[node->getId()] = initial_location;
				}
				void addManagedNode(DistributedId id, int initial_location) override {
					this->managed_nodes_locations[id] = initial_location;
				}

				void removeManagedNode(api::graph::DistributedNode<T>* node) override {
					this->managed_nodes_locations.erase(node->getId());
				}

				void removeManagedNode(DistributedId id) override {
					this->managed_nodes_locations.erase(id);
				}

				void setLocal(api::graph::DistributedNode<T>*) override;
				void setDistant(api::graph::DistributedNode<T>*) override;
				void remove(api::graph::DistributedNode<T>*) override;

				const NodeMap& getLocalNodes() const override {return local_nodes;}
				const NodeMap& getDistantNodes() const override {return distant_nodes;}

				void updateLocations() override;

				
				std::unordered_map<DistributedId, int> getCurrentLocations() const override {
					return managed_nodes_locations;
				}
		};

	template<typename T>
	void LocationManager<T>::setLocal(api::graph::DistributedNode<T>* node) {
			node->setLocation(comm->getRank());
			node->setState(LocationState::LOCAL);
			this->local_nodes.insert({node->getId(), node});
			this->distant_nodes.erase(node->getId());
			new_local_nodes.insert({node->getId(), node});
		}

	template<typename T>
	void LocationManager<T>::setDistant(api::graph::DistributedNode<T>* node) {
			node->setState(LocationState::DISTANT);
			this->local_nodes.erase(node->getId());
			this->distant_nodes.insert({node->getId(), node});
			for(auto edge : node->getOutgoingEdges()) {
				edge->setState(LocationState::DISTANT);
			}
			for(auto edge : node->getIncomingEdges()) {
				edge->setState(LocationState::DISTANT);
			}
		}

	template<typename T>
	void LocationManager<T>::remove(api::graph::DistributedNode<T>* node) {
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
		void LocationManager<T>::updateLocations() {
			FPMAS_LOGD(comm->getRank(), "LOCATION_MANAGER", "Updating node locations...", "");
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
			for(auto node : new_local_nodes) {
				if(node.first.rank() != comm->getRank()) {
					// Notify node origin that node is currently on this proc
					exported_updated_locations[node.first.rank()]
						.push_back(node.first);
				} else {
					// No need to export the new node location to itself :
					// just locally set this node location to this proc
					managed_nodes_locations[node.first]
						= comm->getRank();
				}
			}
			
			// Export / import location updates of managed_nodes_locations
			DistributedIdMap imported_updated_locations =
				id_mpi->migrate(exported_updated_locations);
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
				if(node.first.rank() == comm->getRank()) {
					node.second->setLocation(
							managed_nodes_locations[node.first]
							);
				}
			}

			/*
			 * Step 2 : ask for current locations of distant_nodes
			 */
			DistributedIdMap location_requests;
			for(auto node : distant_nodes) {
				if(node.first.rank() != comm->getRank()) {
					// For distant_nodes that has an origin different from this
					// proc, asks for the current location of node to its
					// origin proc
					location_requests[node.first.rank()]
						.push_back(node.first);
				}
			}
			// Export / import requests
			DistributedIdMap imported_location_requests
				= id_mpi->migrate(location_requests);

			// Builds requests response
			LocationMap exported_locations;
			for(auto list : imported_location_requests) {
				for(auto node_id : list.second) {
					// for each requested node location, respond a tuple
					// {node_id, currentLocation} to the requesting proc
					exported_locations[list.first]
						.push_back({node_id, managed_nodes_locations[node_id]});
				}
			}

			// Import / export responses
			LocationMap imported_locations
				= location_mpi->migrate(exported_locations);

			// Finally, updates the distant_nodes locations from the requests
			// responses
			for(auto list : imported_locations) {
				for(auto location : list.second) {
					distant_nodes.find(location.first)->second->setLocation(location.second);
				}
			}
			new_local_nodes.clear();
			FPMAS_LOGD(comm->getRank(), "LOCATION_MANAGER", "Node locations updated.", "");
		}
}}

namespace nlohmann {
	/**
	 * Defines json serialization rules for generic
	 * fpmas::api::graph::LocationManager instances.
	 *
	 * Notice that it is assumed that the internal state of a LocationManager
	 * is defined only by the location of its managed nodes. In consequence,
	 * the LocationManager::getLocalNodes() and
	 * LocationManager::getDistantNodes() lists for example are not considered
	 * by the serialization process. It is the responsibility of the
	 * DistributedGraph to call LocationManager::setLocal() and
	 * LocationManager::setDistant() methods accordingly when the
	 * DistributedGraph is unserialized.
	 */
	template<typename T>
		struct adl_serializer<fpmas::api::graph::LocationManager<T>> {

			/**
			 * Serializes the `location_manager` to the json `j`.
			 *
			 * @param j json output
			 * @param location_manager location manager to serialize
			 */
			static void to_json(json& j, const fpmas::api::graph::LocationManager<T>& location_manager) {
				for(auto item : location_manager.getCurrentLocations())
					j.push_back({item.first, item.second});
			}

			/**
			 * Unserializes the json `j` into the specified `location_manager`.
			 *
			 * Managed nodes read from the input json are added to the
			 * `location_manager`, without considering its current state, so
			 * clearing the `location_manager` before loading data into it
			 * might be required, depending on the expected behavior.
			 *
			 * @param j json input
			 * @param location_manager output location manager
			 */
			static void from_json(const json& j, fpmas::api::graph::LocationManager<T>& location_manager) {
				for(auto item : j) {
					location_manager.addManagedNode(
							item[0].get<DistributedId>(),
							item[1].get<int>()
							);
				}
			}

		};
}
#endif
