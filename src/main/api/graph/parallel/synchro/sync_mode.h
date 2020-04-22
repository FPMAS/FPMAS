#ifndef SYNC_MODE_API_H
#define SYNC_MODE_API_H

#include <string>

namespace FPMAS::api::graph::parallel::synchro {
	template<typename ArcType>
		class SyncLinker {
			public:
				virtual void link(const ArcType*) = 0;
				virtual void unlink(const ArcType*) = 0;
		};

template<template<typename> class Mutex, template<typename> class SyncLinker>
	class SyncMode {
		public:
			template<typename T>
			using mutex_type = Mutex<T>;

			template<typename ArcType>
			using sync_linker = SyncLinker<ArcType>;

			virtual void synchronize() = 0;
	};
}
#endif
