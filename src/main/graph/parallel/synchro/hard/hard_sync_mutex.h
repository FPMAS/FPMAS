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
	using FPMAS::api::graph::parallel::synchro::hard::MutexRequestType;

	template<typename T>
		class HardSyncMutex
			: public FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T> {
			private:
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexRequest
					request_t;
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexClient<T>
					mutex_client_t;
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexServer<T>
					mutex_server_t;

				DistributedId id;
				std::reference_wrapper<T> _data;
				LocationState* state;
				int* location;
				bool _locked = false;
				mutex_client_t* mutexClient;
				mutex_server_t* mutexServer;

				std::queue<request_t> readRequests;
				std::queue<request_t> lockRequests;
				std::queue<request_t> acquireRequests;

				void _lock() override {_locked=true;}
				void _unlock() override {_locked=false;}

			public:
				HardSyncMutex(T& data) : _data(std::ref(data)) {}

				void setUp(
					DistributedId id,
					LocationState& state,
					int& location,
					mutex_client_t& mutexClient,
					mutex_server_t& mutexServer) {
					this->id = id;
					this->state = &state;
					this->location = &location;
					this->mutexClient = &mutexClient;
					this->mutexServer = &mutexServer;
				}

				void pushRequest(request_t request) override;
				std::queue<request_t> requestsToProcess() override;

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
			if(*state == LocationState::LOCAL) {
				request_t req = request_t(id, request_t::LOCAL, MutexRequestType::READ);
				pushRequest(req);
				mutexServer->wait(req);
				return _data;
			}
			_data.get() = mutexClient->read(id, *location);
			return _data;
		};

	template<typename T>
		T& HardSyncMutex<T>::acquire() {
			if(*state==LocationState::LOCAL) {
				if(_locked) {
					request_t req = request_t(id, request_t::LOCAL, MutexRequestType::ACQUIRE);
					pushRequest(req);
					mutexServer->wait(req);
				}
				this->_locked = true;
				return _data;
			}
			_data.get() = mutexClient->acquire(id, *location);
			return _data;
		};

	template<typename T>
		void HardSyncMutex<T>::release() {
			if(*state==LocationState::LOCAL) {
				mutexServer->notify(id);
				this->_locked = false;
				return;
			}
			mutexClient->release(id, _data, *location);
		}

	template<typename T>
		void HardSyncMutex<T>::lock() {
			if(*state==LocationState::LOCAL) {
				if(_locked) {
					request_t req = request_t(id, request_t::LOCAL, MutexRequestType::LOCK);
					pushRequest(req);
					mutexServer->wait(req);
				}
				this->_locked = true;
				return;
			}
			mutexClient->lock(id, *location);
		}

	template<typename T>
		void HardSyncMutex<T>::unlock() {
			if(*state==LocationState::LOCAL) {
				mutexServer->notify(id);
				this->_locked = false;
				return;
			}
			mutexClient->unlock(id, *location);
		}

	template<typename T>
		void HardSyncMutex<T>::pushRequest(request_t request) {
			switch(request.type) {
				case MutexRequestType::READ :
					readRequests.push(request);
					break;
				case MutexRequestType::LOCK :
					lockRequests.push(request);
					break;
				case MutexRequestType::ACQUIRE :
					acquireRequests.push(request);
			}
		}

	template<typename T>
		std::queue<typename HardSyncMutex<T>::request_t> HardSyncMutex<T>::requestsToProcess() {
			std::queue<request_t> requests;
			if(_locked) {
				return requests;
			}
			while(!readRequests.empty()) {
				request_t readRequest = readRequests.front();
				requests.push(readRequest);
				readRequests.pop();
				if(readRequest.source == request_t::LOCAL) {
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
