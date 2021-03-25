#ifndef FPMAS_OUTPUT_H
#define FPMAS_OUTPUT_H

#include <fstream>

/**
 * @file src/fpmas/io/output.h
 *
 * fpmas::api::output related implementations.
 *
 * FPMAS defines a powerful output system that can be used to efficiently and
 * easily collect data accross processes.
 *
 * Data can be locally fetched on each process using a Watcher instance. Data
 * can then be gathered, summed, filtered or any other operation thanks to the
 * usage of `DistributedOperation` types, described below. For concrete usage
 * and implementations, see fpmas::output::Local, fpmas::output::Reduce and
 * fpmas::output::DistributedCsvOutput.
 *
 * ## DistributedOperation Trait
 * A `DistributedOperation`, that can for example be used by
 * fpmas::output::DistributedCsvOutput to specify distributed CSV fields, must
 * satisfy the following requirements.
 *
 * ### Member types
 * Member Type | Description
 * ----------- | -----------
 * Type   | Datatype used by local watchers
 * Params | Extra parameters that can be passed to `Single` or `All`
 * Single | Operator used when data must be returned on a **single** process
 * All    | Operator used when data must be returned on **all** processes
 *
 * In addition, `Single` and `All` must satisfy the following requirements:
 *
 * ### Single
 * #### Constructor
 *
 * `Single` must define a constructor with the following signature:
 * ```cpp
 * Single(
 * 	fpmas::api::communication::MpiCommunicator& comm,
 * 	int root,
 * 	const Watcher<T>& watcher,
 * 	const Params& params
 * 	)
 * ```
 * @param comm MPI communicator
 * @param root process on which data must be returned. Data returned on all
 * other processes is undefined.
 * @param watcher local watcher used on each process
 * @param params extra parameters
 *
 * #### Call operator
 * `Single` must define a call operator that does not take any parameter and
 * returns a instance of Type:
 * ```cpp
 * Type operator()();
 * ```
 * This method must return fetched data if the current process is `root`: results
 * returned on other processes are undefined.
 *
 * ### All
 * #### Constructor
 *
 * `All` must define a constructor with the following signature:
 * ```cpp
 * Single(
 * 	fpmas::api::communication::MpiCommunicator& comm,
 * 	const Watcher<T>& watcher,
 * 	const Params& params
 * 	)
 * ```
 * @param comm MPI communicator
 * @param watcher local watcher used on each process
 * @param params extra parameters
 *
 * #### Call operator
 * `All` must define a call operator that does not take any parameter and
 * returns a instance of Type:
 * ```cpp
 * Type operator()();
 * ```
 * This method must return fetched data on **all** processes. Notice that this
 * does not mean that fetched data is the same on all processes (see for
 * example fpmas::output::Local).
 */

#include "fpmas/api/io/output.h"
#include "fpmas/communication/communication.h"

namespace fpmas { namespace io {
	using api::io::Watcher;

	/**
	 * Trivial type that can be used as the `Params` member type of
	 * `DistributedOperation` implementation when no extra parameter is
	 * required.
	 */
	struct Void {};


	/**
	 * A `DistributedOperation` that can be used to fetch data.
	 *
	 * The concept of Local is to fetch data only from the local process,
	 * without considering other processes. The current rank of the process or
	 * the current simulation step are good examples of fields that are likely
	 * to be defined Local.
	 *
	 * More precisely, the call operator simply returns data returned by the
	 * local `watcher` instance.
	 */
	template<typename T>
		struct Local  {
			/**
			 * Type returned by the internal Watcher.
			 */
			typedef T Type;
			/**
			 * Local does not take any extra parameter.
			 */
			typedef Void Params;

			/**
			 * _Single_ mode for the Local operation.
			 */
			class Single {
				private:
					Watcher<T> watcher;
				public:
					/**
					 * Local::Single constructor.
					 *
					 * @param watcher local watcher used to fetch data
					 */
					Single(
							api::communication::MpiCommunicator&,
							int,
							const Watcher<T>& watcher,
							const Params&)
						: watcher(watcher) {}
					/**
					 * Returns local data fetched by the `watcher` (even if the
					 * current process is not `root`.
					 *
					 * @return local data
					 */
					T operator()() {
						return watcher();
					}
			};
			/**
			 * _All_ mode for the Local operation.
			 */
			class All {
				private:
					Watcher<T> watcher;
				public:
					/**
					 * Local::All constructor.
					 *
					 * @param watcher local watcher used to fetch data
					 */
					All(
							api::communication::MpiCommunicator&,
							const Watcher<T>& watcher,
							const Params&)
						: watcher(watcher) {}

					/**
					 * Returns local data fetched by the `watcher`.
					 *
					 * @return local data
					 */
					T operator()() {
						return watcher();
					}

			};
		};

	/**
	 * A `DistributedOperation` that can be used to fetch data.
	 *
	 * Data is gathered from all processes, and reduced using the provided
	 * `BinaryOp`, which applies the + operator by default.
	 *
	 * @tparam BinaryOp Function object used to reduce data. See
	 * https://en.cppreference.com/w/cpp/utility/functional for useful
	 * predefined operators.
	 * @tparam Mpi fpmas::api::communication::TypedMpi implementation
	 */
	template<typename T, typename BinaryOp = std::plus<T>, template<typename> class Mpi = communication::TypedMpi>
		struct Reduce {
			/**
			 * Type returned by the internal Watcher.
			 */
			typedef T Type;
			/**
			 * Reduce does not take any extra parameter.
			 */
			typedef Void Params;

			/**
			 * _Single_ mode for the Reduce operation.
			 *
			 * Result is gathered only at `root`.
			 */
			class Single {
				private:
					Mpi<T> mpi;
					int root;
					Watcher<T> watcher;
					BinaryOp binary_op;

				public:
					/**
					 * Reduce::Single constructor.
					 *
					 * @param comm MPI communicator
					 * @param root rank of the process where data is fetched
					 * @param watcher local watcher
					 */
					Single(
							api::communication::MpiCommunicator& comm,
							int root,
							const Watcher<T>& watcher,
							const Params&)
						: mpi(comm), root(root), watcher(watcher) {}

					/**
					 * Applies the fpmas::communication::reduce() operation on
					 * data returned by `watcher()`.
					 *
					 * Reduced data is returned at `root`. On other processes,
					 * the local data returned by `watcher()` is returned.
					 *
					 * @return fetched data
					 */
					T operator()() {
						return fpmas::communication::reduce(mpi, root, watcher(), binary_op);
					}
			};

			/**
			 * _All_ mode for the Reduce operation.
			 *
			 * Result is gathered on all processes.
			 */
			class All {
				private:
					Mpi<T> mpi;
					Watcher<T> watcher;
					BinaryOp binary_op;

				public:
					/**
					 * Reduce::All constructor.
					 *
					 * @param comm MPI communicator
					 * @param watcher local watcher
					 */
					All(
							api::communication::MpiCommunicator& comm,
							const Watcher<T>& watcher,
							const Params&)
						: mpi(comm), watcher(watcher) {}

					/**
					 * Applies the fpmas::communication::reduce() operation on
					 * data returned by `watcher()`.
					 *
					 * Reduced data is returned on all processes.
					 *
					 * @return fetched data
					 */
					T operator()() {
						return fpmas::communication::all_reduce(mpi, watcher(), binary_op);
					}
			};
		};

	/**
	 * An helper class that can be used to initialize a file output stream.
	 *
	 * This class is notably useful to extend output classes that take an
	 * output stream as constructor parameter, such as CsvOutput.
	 *
	 * For example, the following code is **wrong** and will produce a
	 * segmentation fault:
	 * ```cpp
	 * class MyOutput : public fpmas::io::CsvOutput<int> {
	 * 	private:
	 * 		std::ofstream file("file.csv");
	 *
	 * 	public:
	 * 		MyOutput()
	 * 			: fpmas::io::CsvOutput<int>(
	 * 				this->file, 
	 * 				{"my_field", [] () {return ...}}
	 * 				) {}
	 * };
	 * ```
	 * Indeed, the CsvOutput constructor uses the specified output stream (to
	 * automatically write CSV headers), BUT CsvOutput, as a base class of
	 * `MyOutput`, is initialized **before** `MyOutput`, so before `file`, what
	 * produces a segmentation fault.
	 *
	 * This FileOutput class can be used to easily solve the issue:
	 * ```cpp
	 * // FileOutput must be specified BEFORE CsvOutput
	 * class MyOutput : public fpmas::io::FileOutput, fpmas::io::CsvOutput<int> {
	 * 	public:
	 * 		MyOutput() :
	 * 			fpmas::io::FileOutput("file.csv"),
	 * 			fpmas::io::CsvOutput<int>(
	 * 				this->file,
	 * 				{"my_field", [] () {return ...}}
	 * 				) {}
	 * };
	 * ```
	 * In this case, FileOutput is properly initialized before CsvOutput so no
	 * error occurs.
	 */
	struct FileOutput {
		/**
		 * std::ofstream file instance
		 */
		std::ofstream file;

		/**
		 * FileOutput constructor.
		 *
		 * @see https://en.cppreference.com/w/cpp/io/basic_filebuf/open
		 *
		 * @param filename file name
		 * @param mode standard open mode
		 */
		FileOutput(
				std::string filename,
				std::ios_base::openmode mode = std::ios_base::out
				);

		/**
		 * FileOutput constructor.
		 *
		 * `file_format` is formatted using fpmas::utils::format(std::string, int)
		 * to produce a file name.
		 *
		 * @see https://en.cppreference.com/w/cpp/io/basic_filebuf/open
		 *
		 * @param file_format file name to format
		 * @param rank rank of the current process
		 * @param mode standard open mode
		 */
		FileOutput(
				std::string file_format,
				int rank,
				std::ios_base::openmode mode = std::ios_base::out
				);

		/**
		 * FileOutput constructor.
		 *
		 * `file_format` is formatted using
		 * fpmas::utils::format(std::string, fpmas::api::scheduler::TimeStep)
		 * to produce a file name.
		 *
		 * @see https://en.cppreference.com/w/cpp/io/basic_filebuf/open
		 *
		 * @param file_format file name to format
		 * @param step current time step
		 * @param mode standard open mode
		 */
		FileOutput(
				std::string file_format,
				api::scheduler::TimeStep step,
				std::ios_base::openmode mode = std::ios_base::out
				);
		/**
		 * FileOutput constructor.
		 *
		 * `file_format` is formatted using
		 * fpmas::utils::format(std::string, int, fpmas::api::scheduler::TimeStep)
		 * to produce a file name.
		 *
		 * @see https://en.cppreference.com/w/cpp/io/basic_filebuf/open
		 *
		 * @param file_format file name to format
		 * @param rank rank of the current process
		 * @param step current time step
		 * @param mode standard open mode
		 */
		FileOutput(
				std::string file_format,
				int rank,
				api::scheduler::TimeStep step,
				std::ios_base::openmode mode = std::ios_base::out
				);
	};
}}
#endif
