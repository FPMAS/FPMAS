#ifndef FPMAS_GLOBAL_GHOST_MODE_H
#define FPMAS_GLOBAL_GHOST_MODE_H

#include "ghost_mode.h"

namespace fpmas { namespace synchro {
	namespace ghost {
		/**
		 * GlobalGhostMode Mutex implementation.
		 *
		 * read() and acquire() methods return a reference to a _ghost_ copy,
		 * updated from the origin data only at each synchronize() calls.
		 *
		 * @see fpmas::synchro::GlobalGhostMode
		 */
		template<typename T>
			class GlobalGhostMutex : public SingleThreadMutex<T> {
				private:
					T ghost_data;

				public:
					/**
					 * GlobalGhostMutex constructor.
					 *
					 * The internal _ghost_ copy state is initialized from the
					 * provided data.
					 *
					 * @param data reference to node data
					 */
					GlobalGhostMutex(T& data)
						: SingleThreadMutex<T>(data), ghost_data(data) {
						}

					const T& read() override {return ghost_data;};
					void releaseRead() override {};
					T& acquire() override {return ghost_data;};
					void releaseAcquire() override {};

					void synchronize() override {
						this->ghost_data = this->data();
					}
			};
	}

	/**
	 * Strict GhostMode implementation, based on the GlobalGhostMutex.
	 *
	 * read and acquire operations, on \LOCAL or \DISTANT nodes, are always
	 * performed on a temporary _ghost_ copy. The copy is updated at each
	 * DataSync::synchronize() call from the state of the original data.
	 *
	 * In the \Agent context, this means that modifications on \LOCAL agents
	 * are only perceived after the next synchronization.
	 *
	 * Since modifications on \DISTANT agents are erased at each
	 * synchronization, modifications between agents is likely to produce
	 * unexpected behaviors in this mode. However, each \LOCAL agent is allowed
	 * to modify its own state.
	 *
	 * In consequence, when an agent modifies its own state, modifications are
	 * not perceived yet by other \LOCAL agent who still see the _ghost_ copy.
	 *
	 * This ensures that \LOCAL agents **always** perceive the state of other
	 * \LOCAL **or** \DISTANT agents at the previous time step. In the case of
	 * a properly implemented read-only model, the execution does not depends
	 * on the order of execution of agents nor on their \LOCAL or \DISTANT
	 * state. In consequence, this mode implements the strongest
	 * reproducibility requirements.
	 *
	 *
	 * This synchronization policy is inspired from
	 * [D-MASON](https://github.com/isislab-unisa/dmason).
	 *
	 * @see fpmas::synchro::GhostMode
	 * @see fpmas::synchro::HardSyncMode
	 */
	template<typename T>
		using GlobalGhostMode = ghost::GhostMode<T, ghost::GlobalGhostMutex>;
}}
#endif
