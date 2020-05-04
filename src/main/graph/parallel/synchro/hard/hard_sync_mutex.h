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
	template<typename T>
		class HardSyncMutex
			: public FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T> {
			private:
				typedef FPMAS::api::graph::parallel::synchro::hard::Request
					request_t;
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
				std::queue<request_t> _readRequests;
				std::queue<request_t> _acquireRequests;

				void _lock() override {_locked=true;}
				void _unlock() override {_locked=false;}

			public:
				HardSyncMutex(
						DistributedId id, T& data, LocationState& state, int& location,
						mutex_client_t& mutexClient, mutex_server_t& mutexServer
						) :
					_id(id), _data(data), state(state), location(location),
					mutexClient(mutexClient), mutexServer(mutexServer) {}

				std::queue<request_t>& readRequests() override {return _readRequests;}
				std::queue<request_t>& acquireRequests() override {return _acquireRequests;}

				const T& data() override {return _data;}
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
				request_t req = request_t(_id, -1);
				_readRequests.push(req);
				mutexServer.wait(req);
				return _data;
			}
			_data = mutexClient.read(_id, location);
			return _data;
		};

	template<typename T>
		T& HardSyncMutex<T>::acquire() {
			if(state==LocationState::LOCAL) {
				request_t req = request_t(_id, -1);
				_readRequests.push(req);
				mutexServer.wait(req);
				return _data;
			}
			_data = mutexClient.acquire(_id, location);
			return _data;
		};

	template<typename T>
		void HardSyncMutex<T>::release() {
			if(state==LocationState::LOCAL) {
				mutexServer.notify(_id);
				return;
			}
			mutexClient.release(_id, location);
		}

	template<typename T>
		void HardSyncMutex<T>::lock() {
			if(state==LocationState::LOCAL) {
				auto req = request_t(_id, -1);
				_acquireRequests.push(req);
				mutexServer.wait(req);
				return;
			}
			mutexClient.lock(_id, location);
		}

	template<typename T>
		void HardSyncMutex<T>::unlock() {
			if(state==LocationState::LOCAL) {
				mutexServer.notify(_id);
				return;
			}
			mutexClient.unlock(_id, location);
		}
}

#endif
