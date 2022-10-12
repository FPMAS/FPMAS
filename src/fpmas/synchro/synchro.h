#ifndef FPMAS_SYNCHRO_H
#define FPMAS_SYNCHRO_H

#include <utility>

/** \file src/fpmas/synchro/synchro.h
 * Generic synchronisation utilities.
 */

namespace fpmas { namespace synchro {
	/**
	 * Generic structure to update local data from a temporary updated data
	 * instance.
	 *
	 * The principle of the update() method is to pull updates into the local
	 * data. By default, this is performed using an `std::move` operation, but
	 * specializations might introduce specific behaviors for some type T.
	 */
	template<typename T>
		struct DataUpdate {
			/**
			 * Updates `local_data` from `updated_data`.
			 *
			 * By default, performs `local_data = std::move(updated_data)`, so
			 * the [move assignment
			 * operator](https://en.cppreference.com/w/cpp/language/move_assignment)
			 * T::operator=(T&&) is used, in order to maximize performances and
			 * avoid useless copies.
			 *
			 * @param local_data reference to the local data to update
			 * @param updated_data reference to the temporary updated data that
			 * must be pulled into local data. `updated_data` can be "consumed"
			 * in the process, i.e. left in an undefined/void state.
			 */
			static void update(T& local_data, T&& updated_data) {
				local_data = std::move(updated_data);
			}
		};
}}
#endif
