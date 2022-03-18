#ifndef FPMAS_HARD_SYNC_MUTEX_H
#define FPMAS_HARD_SYNC_MUTEX_H

/** \file src/fpmas/synchro/hard/hard_sync_mutex.h
 * HardSyncMode Mutex implementation.
 */

#include "mutex_client.h"
#include "mutex_server.h"
#include "../synchro.h"

namespace fpmas { namespace synchro { namespace hard {

	using fpmas::api::graph::LocationState;
	using api::MutexRequestType;

	/**
	 * HardSyncMode Mutex implementation.
	 *
	 * Each operation is added to a queue and potentially transmitted to
	 * \DISTANT processes to ensure a strict global concurrent access
	 * management.
	 */
	template<typename T>
		class HardSyncMutex : public api::HardSyncMutex<T> {
			private:
				typedef api::MutexRequest Request;
				typedef api::MutexClient<T> MutexClient;
				typedef api::MutexServer<T> MutexServer;

				fpmas::api::graph::DistributedNode<T>* node;
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
				/**
				 * HardSyncMutex constructor.
				 *
				 * @param node node associated to this mutex
				 * @param mutex_client client used to transmit requests to
				 * other processes
				 * @param mutex_server server used to handle incoming mutex
				 * requests
				 */
				HardSyncMutex(
						fpmas::api::graph::DistributedNode<T>* node,
						MutexClient& mutex_client,
						MutexServer& mutex_server)
					: node(node), mutex_client(mutex_client), mutex_server(mutex_server) {}

				void pushRequest(Request request) override;
				std::queue<Request> requestsToProcess() override;

				/**
				 * \copydoc fpmas::api::synchro::Mutex::data
				 */
				T& data() override {return node->data();}
				/**
				 * \copydoc fpmas::api::synchro::Mutex::data
				 */
				const T& data() const override {return node->data();}

				const T& read() override;
				void releaseRead() override;

				T& acquire() override;
				void releaseAcquire() override;

				void lock() override;
				void unlock() override;
				/**
				 * \copydoc fpmas::api::synchro::Mutex::locked()
				 */
				bool locked() const override {return _locked;}

				void lockShared() override;
				void unlockShared() override;
				/**
				 * \copydoc fpmas::api::synchro::Mutex::sharedLockCount()
				 */
				int sharedLockCount() const override {return _locked_shared;}

				void synchronize() override {};
		};

	/**
	 * \copydoc fpmas::api::synchro::Mutex::read()
	 *
	 * \implem
	 * If the protected node is \LOCAL and locked, adds the local request to
	 * the queue and waits for the request to complete. Else if its unlocked,
	 * directly returns the read data (which is already local).
	 *
	 * \par
	 * If the node is \DISTANT, transmits the request using the MutexClient.
	 *
	 * @see MutexServer::wait()
	 * @see MutexClient::read()
	 */
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
			synchro::DataUpdate<T>::update(
					node->data(),
					mutex_client.read(node->getId(), node->location())
					);
			return node->data();
		}

	/**
	 * \copydoc fpmas::api::synchro::Mutex::releaseRead()
	 *
	 * \implem
	 * If the protected node is \LOCAL, releases a shared lock from the mutex.
	 * If this unlocks the resource, the MutexServer is notified and pending
	 * requests are handled.
	 *
	 * \par
	 * If the node is \DISTANT, transmits the request using the MutexClient.
	 *
	 * @see MutexServer::notify()
	 * @see MutexClient::releaseRead()
	 */
	template<typename T>
		void HardSyncMutex<T>::releaseRead() {
			if(node->state() == LocationState::LOCAL) {
				_locked_shared--;
				if(_locked_shared==0) {
					mutex_server.notify(node->getId());
				}
				return;
			}
			mutex_client.releaseRead(node->getId(), node->location());
		}

	/**
	 * \copydoc fpmas::api::synchro::Mutex::releaseAcquire()
	 *
	 * \implem
	 * If the protected node is \LOCAL and locked, adds the local request to
	 * the queue and waits for the request to complete. Else if its unlocked,
	 * directly returns the acquired data (which is already local).
	 *
	 * \par
	 * If the node is \DISTANT, transmits the request using the MutexClient.
	 *
	 * @see MutexServer::wait()
	 * @see MutexClient::acquire()
	 */
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

			synchro::DataUpdate<T>::update(
					node->data(),
					mutex_client.acquire(node->getId(), node->location())
					);
			return node->data();
		}

	/**
	 * \copydoc fpmas::api::synchro::Mutex::releaseAcquire()
	 *
	 * \implem
	 * If the protected node is \LOCAL, releases the exclusive lock from the
	 * mutex and notifies the MutexServer so that pending requests are handled.
	 *
	 * \par
	 * If the node is \DISTANT, transmits the request with local data updates
	 * using the MutexClient.
	 *
	 * @see MutexServer::notify()
	 * @see MutexClient::releaseAcquire()
	 */
	template<typename T>
		void HardSyncMutex<T>::releaseAcquire() {
			if(node->state()==LocationState::LOCAL) {
				this->_locked = false;
				mutex_server.notify(node->getId());
				return;
			}
			mutex_client.releaseAcquire(node->getId(), node->data(), node->location());
		}

	/**
	 * \copydoc fpmas::api::synchro::Mutex::lock()
	 *
	 * \implem
	 * If the protected node is \LOCAL and locked, adds the local request to
	 * the queue and waits for the request to complete. Else if its unlocked,
	 * directly locks the mutex.
	 *
	 * \par
	 * If the node is \DISTANT, transmits the request using the MutexClient.
	 *
	 * @see MutexServer::wait()
	 * @see MutexClient::lock()
	 */
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
			mutex_client.lock(node->getId(), node->location());
		}

	/**
	 * \copydoc fpmas::api::synchro::Mutex::unlock()
	 *
	 * \implem
	 * If the protected node is \LOCAL, releases the exclusive lock from the
	 * mutex and notifies the MutexServer so that pending requests are handled.
	 *
	 * \par
	 * If the node is \DISTANT, transmits the request using the MutexClient.
	 *
	 * @see MutexServer::notify()
	 * @see MutexClient::unlock()
	 */
	template<typename T>
		void HardSyncMutex<T>::unlock() {
			if(node->state()==LocationState::LOCAL) {
				// TODO : this is NOT thread safe
				this->_locked = false;
				mutex_server.notify(node->getId());
				return;
			}
			mutex_client.unlock(node->getId(), node->location());
		}

	/**
	 * \copydoc fpmas::api::synchro::Mutex::lockShared()
	 *
	 * \implem
	 * If the protected node is \LOCAL and locked, adds the local request to
	 * the queue and waits for the request to complete. Else if its unlocked
	 * (or shared lock), adds a share lock to the mutex.
	 *
	 * \par
	 * If the node is \DISTANT, transmits the request using the MutexClient.
	 *
	 * @see MutexServer::wait()
	 * @see MutexClient::lockShared()
	 */
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
			mutex_client.lockShared(node->getId(), node->location());
		}

	/**
	 * \copydoc fpmas::api::synchro::Mutex::unlockShared()
	 *
	 * \implem
	 * If the protected node is \LOCAL, releases a shared lock from the mutex.
	 * If this unlocks the resource, the MutexServer is notified and pending
	 * requests are handled.
	 *
	 * \par
	 * If the node is \DISTANT, transmits the request using the MutexClient.
	 *
	 * @see MutexServer::notify()
	 * @see MutexClient::unlockShared()
	 */
	template<typename T>
		void HardSyncMutex<T>::unlockShared() {
			if(node->state()==LocationState::LOCAL) {
				_locked_shared--;
				if(_locked_shared==0) {
					mutex_server.notify(node->getId());
				}
				return;
			}
			mutex_client.unlockShared(node->getId(), node->location());
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
}}}
#endif
