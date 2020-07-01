#ifndef HARD_SYNC_MUTEX_H
#define HARD_SYNC_MUTEX_H

#include <queue>
#include "fpmas/api/communication/communication.h"
#include "fpmas/api/graph/parallel/location_state.h"
#include "fpmas/api/synchro/hard/hard_sync_mutex.h"
#include "mutex_client.h"
#include "mutex_server.h"

namespace fpmas::synchro::hard {

	using fpmas::api::graph::parallel::LocationState;
	using fpmas::api::synchro::hard::MutexRequestType;

	template<typename T>
		class HardSyncMutex
			: public fpmas::api::synchro::hard::HardSyncMutex<T> {
			private:
				typedef fpmas::api::synchro::hard::MutexRequest
					Request;
				typedef fpmas::api::synchro::hard::MutexClient<T>
					MutexClient;
				typedef fpmas::api::synchro::hard::MutexServer<T>
					MutexServer;

				api::graph::parallel::DistributedNode<T>* node;
				bool _locked = false;
				int _locked_shared = 0;

				MutexClient& mutex_client;
				MutexServer& mutex_server;

				std::queue<Request> lock_requests;
				std::queue<Request> lock_shared_requests;

				void _lock() override {_locked=true;}
				void _lockShared() override {_locked_shared++;}
				void _unlock() override {_locked=false;}
				void _unlockShared() override {_locked_shared--;}
			public:
				HardSyncMutex(api::graph::parallel::DistributedNode<T>* node, MutexClient& mutex_client, MutexServer& mutex_server) :
					node(node), mutex_client(mutex_client), mutex_server(mutex_server) {}

				void pushRequest(Request request) override;
				std::queue<Request> requestsToProcess() override;

				T& data() override {return node->data();}
				const T& data() const override {return node->data();}

				const T& read() override;
				void releaseRead() override;

				T& acquire() override;
				void releaseAcquire() override;

				void lock() override;
				void unlock() override;
				bool locked() const override {return _locked;}

				void lockShared() override;
				void unlockShared() override;
				int lockedShared() const override {return _locked_shared;}
		};

	template<typename T>
		const T& HardSyncMutex<T>::read() {
			if(node->state() == LocationState::LOCAL) {
				if(_locked) {
					Request req = Request(node->getId(), Request::LOCAL, MutexRequestType::READ);
					pushRequest(req);
					mutex_server.wait(req);
				}
				_locked_shared++;
				return node->data();
			}
			node->data() = std::move(mutex_client.read(node->getId(), node->getLocation()));
			return node->data();
		}
	template<typename T>
		void HardSyncMutex<T>::releaseRead() {
			if(node->state() == LocationState::LOCAL) {
				_locked_shared--;
				if(_locked_shared==0) {
					mutex_server.notify(node->getId());
				}
				return;
			}
			mutex_client.releaseRead(node->getId(), node->getLocation());
		}

	template<typename T>
		T& HardSyncMutex<T>::acquire() {
			if(node->state()==LocationState::LOCAL) {
				if(_locked || _locked_shared > 0) {
					Request req = Request(node->getId(), Request::LOCAL, MutexRequestType::ACQUIRE);
					pushRequest(req);
					mutex_server.wait(req);
				}
				this->_locked = true;
				return node->data();
			}
			node->data() = std::move(mutex_client.acquire(node->getId(), node->getLocation()));
			return node->data();
		}

	template<typename T>
		void HardSyncMutex<T>::releaseAcquire() {
			if(node->state()==LocationState::LOCAL) {
				this->_locked = false;
				mutex_server.notify(node->getId());
				return;
			}
			mutex_client.releaseAcquire(node->getId(), node->data(), node->getLocation());
		}

	template<typename T>
		void HardSyncMutex<T>::lock() {
			if(node->state()==LocationState::LOCAL) {
				if(_locked || _locked_shared > 0) {
					Request req = Request(node->getId(), Request::LOCAL, MutexRequestType::LOCK);
					pushRequest(req);
					mutex_server.wait(req);
				}
				this->_locked = true;
				return;
			}
			mutex_client.lock(node->getId(), node->getLocation());
		}

	template<typename T>
		void HardSyncMutex<T>::unlock() {
			if(node->state()==LocationState::LOCAL) {
				// TODO : this is NOT thread safe
				this->_locked = false;
				mutex_server.notify(node->getId());
				return;
			}
			mutex_client.unlock(node->getId(), node->getLocation());
		}

	template<typename T>
		void HardSyncMutex<T>::lockShared() {
			if(node->state()==LocationState::LOCAL) {
				if(_locked) {
					Request req = Request(node->getId(), Request::LOCAL, MutexRequestType::LOCK_SHARED);
					pushRequest(req);
					mutex_server.wait(req);
				}
				_locked_shared++;
				return;
			}
			mutex_client.lockShared(node->getId(), node->getLocation());
		}

	template<typename T>
		void HardSyncMutex<T>::unlockShared() {
			if(node->state()==LocationState::LOCAL) {
				_locked_shared--;
				if(_locked_shared==0) {
					mutex_server.notify(node->getId());
				}
				return;
			}
			mutex_client.unlockShared(node->getId(), node->getLocation());
		}

	template<typename T>
		void HardSyncMutex<T>::pushRequest(Request request) {
			switch(request.type) {
				case MutexRequestType::READ :
					lock_shared_requests.push(request);
					break;
				case MutexRequestType::LOCK :
					lock_requests.push(request);
					break;
				case MutexRequestType::ACQUIRE :
					lock_requests.push(request);
					break;
				case MutexRequestType::LOCK_SHARED:
					lock_shared_requests.push(request);
			}
		}

	// TODO : this must be the job of a ReadersWriters component
	template<typename T>
		std::queue<typename HardSyncMutex<T>::Request> HardSyncMutex<T>::requestsToProcess() {
			std::queue<Request> requests;
			if(_locked) {
				return requests;
			}
			while(!lock_shared_requests.empty()) {
				Request lock_shared_request = lock_shared_requests.front();
				requests.push(lock_shared_request);
				lock_shared_requests.pop();
				if(lock_shared_request.source == Request::LOCAL) {
					// Immediately returns, so that the last request processed
					// is the local request
					return requests;
				}
			}
			if(!lock_requests.empty()) {
				requests.push(lock_requests.front());
				lock_requests.pop();
			}
			return requests;
		}
}

#endif
