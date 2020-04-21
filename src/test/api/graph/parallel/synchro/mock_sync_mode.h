#ifndef MOCK_SYNC_MODE_H
#define MOCK_SYNC_MODE_H

#include "api/graph/parallel/synchro/sync_mode.h"
#include "mock_mutex.h"

template<typename ArcType>
class MockSyncLinker : public FPMAS::api::graph::parallel::synchro::SyncLinker<ArcType> {
	public:
		MOCK_METHOD(void, link, (const ArcType*), (override));
		MOCK_METHOD(void, unlink, (const ArcType*), (override));

};

template<template<typename> class Mutex, template<typename> class SyncLinker>
class MockSyncMode : public FPMAS::api::graph::parallel::synchro::SyncMode<Mutex, SyncLinker> {
	public:
		MOCK_METHOD(void, synchronize, (), (override));

};

#endif
