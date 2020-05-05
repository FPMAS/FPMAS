#ifndef HARD_SYNC_MUTEX_H
#define HARD_SYNC_MUTEX_H

#include <queue>
#include "api/communication/communication.h"
#include "api/graph/parallel/location_state.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"
#include "mutex_client.h"
#include "mutex_server.h"

namespace FPMAS::graph::parallel::synchro::hard {

	using FPMAS::api::graph::parallel::LocationState;
	using FPMAS::api::graph::parallel::synchro::hard::Request;
	using FPMAS::api::graph::parallel::synchro::hard::RequestType;

	template<typename T>
		class HardSyncMutex
			: public FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T> {
			private:
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexClient<T>
					mutex_client_t;
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexServer<T>
					mutex_server_t;

				DistributedId _id;
				T& _data;
				LocationState& state;
				int& location;
				bool _locked = false;
				mutex_client_t& mutexClient;
				mutex_server_t& mutexServer;
				std::queue<Request> readRequests;
				std::queue<Request> lockRequests;
				std::queue<Request> acquireRequests;

				void _lock() override {_locked=true;}
				void _unlock() override {_locked=false;}

			public:
				HardSyncMutex(
						DistributedId id, T& data, LocationState& state, int& location,
						mutex_client_t& mutexClient, mutex_server_t& mutexServer
						) :
					_id(id), _data(data), state(state), location(location),
					mutexClient(mutexClient), mutexServer(mutexServer) {}

				void pushRequest(Request request) override;
				std::queue<Request> requestsToProcess() override;

				T& data() override {return _data;}
				const T& read() override;
				T& acquire() override;
				void release() override;

				void lock() override;
				void unlock() override;
				bool locked() const override {return _locked;}
		};

	template<typename T>
		const T& HardSyncMutex<T>::read() {
			if(state == LocationState::LOCAL) {
				Request req = Request(_id, Request::LOCAL, RequestType::READ);
				pushRequest(req);
				mutexServer.wait(req);
				return _data;
			}
			_data = mutexClient.read(_id, location);
			return _data;
		};

	template<typename T>
		T& HardSyncMutex<T>::acquire() {
			if(state==LocationState::LOCAL) {
				if(_locked) {
					Request req = Request(_id, Request::LOCAL, RequestType::ACQUIRE);
					pushRequest(req);
					mutexServer.wait(req);
				}
				this->_locked = true;
				return _data;
			}
			_data = mutexClient.acquire(_id, location);
			std::cout << "acquired data : " << _data << std::endl;
			return _data;
		};

	template<typename T>
		void HardSyncMutex<T>::release() {
			std::cout << "release" << std::endl;
			if(state==LocationState::LOCAL) {
				mutexServer.notify(_id);
				this->_locked = false;
				return;
			}
			mutexClient.release(_id, location);
		}

	template<typename T>
		void HardSyncMutex<T>::lock() {
			if(state==LocationState::LOCAL) {
				if(_locked) {
					Request req = Request(_id, Request::LOCAL, RequestType::LOCK);
					pushRequest(req);
					mutexServer.wait(req);
				}
				this->_locked = true;
				return;
			}
			mutexClient.lock(_id, location);
		}

	template<typename T>
		void HardSyncMutex<T>::unlock() {
			if(state==LocationState::LOCAL) {
				mutexServer.notify(_id);
				this->_locked = false;
				return;
			}
			mutexClient.unlock(_id, location);
		}

	template<typename T>
		void HardSyncMutex<T>::pushRequest(Request request) {
			switch(request.type) {
				case RequestType::READ :
					readRequests.push(request);
					break;
				case RequestType::LOCK :
					lockRequests.push(request);
					break;
				case RequestType::ACQUIRE :
					acquireRequests.push(request);
			}
		}

	template<typename T>
		std::queue<Request> HardSyncMutex<T>::requestsToProcess() {
			std::queue<Request> requests;
			if(_locked) {
				return requests;
			}
			while(!readRequests.empty()) {
				Request readRequest = readRequests.front();
				requests.push(readRequest);
				readRequests.pop();
				if(readRequest.source == Request::LOCAL) {
					// Immediately returns, so that the last request processed
					// is the local request
					return requests;
				}
			}
			if(!lockRequests.empty()) {
				requests.push(lockRequests.front());
				lockRequests.pop();
			}
			else if(!acquireRequests.empty()) {
				requests.push(acquireRequests.front());
				acquireRequests.pop();
			}
			return requests;
		};
}

#endif
