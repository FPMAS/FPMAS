#include "fpmas.h"

/**
 * @example fpmas/output/distributed_csv_output.cpp
 *
 * fpmas::output::DistributedCsvOutput usage example.
 *
 * @par output on 4 processes
 * ```
 * rank,i,f
 * 0,0,6
 * 0,16,0
 * ```
 *
 * If the example is run on several processes, each process will independently
 * output the same data
 */

using namespace fpmas::output;

int main(int argc, char** argv) {
	fpmas::init(argc, argv);
	{
		// The value of i is different on each process
		int i = fpmas::communication::WORLD.getRank();
		float f = 1.5;

		// Field types are specified as template parameters.
		// The Local field is only considered on the process where data is
		// fetched, i.e. 0 in this case.
		// The second field, of type `int`, is gathered from all the processes and
		// reduced using a multiplication.
		// The third field, of type `float`, is gathered from all the processes
		// and reduced using an addition.
		// It the `root` parameter (0 in this case), was not specified, output
		// would be performed on all processes. While reduced fields would be
		// identical on all processes, the Local field would be equal to the rank
		// of the process on which the output is performed.
		DistributedCsvOutput<
			Local<int>, Reduce<int, std::multiplies<int>>, Reduce<float>> distributed_output(
				// Output data on process 0 using the WORLD communicator.
				// If the root process is ommitted, data will be output to
				// std::cout on all processes.
				fpmas::communication::WORLD, 0,
				// Output to stdout, but only on process 0
				std::cout,
				// How each field is fetched at each dump() call can be
				// specified using lambda functions. When value are captured
				// by reference (using &), latest modifications are
				// automatically fetched when dump() is called.
				{"rank", [] () {return fpmas::communication::WORLD.getRank();}}, // Local field
				{"i", [&i] () {return i;}}, // Reduced field using *
				{"f", [&f] () {return f;}} // Reduced field using +
				);

		// Gathers all value of i and f, performs a sum of all data coming from
		// each process, and prints it to stdout on process 0
		distributed_output.dump();

		// Updating local data
		i = 2;
		f = 0.f;

		distributed_output.dump();
	}
	fpmas::finalize();
}
