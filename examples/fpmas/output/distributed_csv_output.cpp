#include "fpmas.h"

/**
 * @example fpmas/output/distributed_csv_output.cpp
 *
 * fpmas::output::DistributedCsvOutput usage example.
 *
 * @par output on 4 processes
 * ```
 * i,f
 * 6,6
 * 8,0
 * ```
 *
 * If the example is run on several processes, each process will independently
 * output the same data
 */

int main(int argc, char** argv) {
	fpmas::init(argc, argv);
	{
		// The value of i is different on each process
		int i = fpmas::communication::WORLD.getRank();
		float f = 1.5;

		// Field types are specified as template parameters
		fpmas::output::DistributedCsvOutput<int, float> distributed_output(
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
				{"i", [&i] () {return i;}},
				{"f", [&f] () {return f;}}
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
