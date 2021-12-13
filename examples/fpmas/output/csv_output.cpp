#include "fpmas.h"

FPMAS_DEFAULT_JSON_SET_UP();

/**
 * @example fpmas/output/csv_output.cpp
 *
 * fpmas::output::CsvOutput usage example.
 *
 * @par output on one process
 * ```
 * process,i,f,s
 * 0,1,1.5,hello
 * 0,2,0,world
 * ```
 *
 * If the example is run on several processes, each process will independently
 * output the same data
 */

int main(int argc, char** argv) {
	fpmas::init(argc, argv);
	{
		int i = 1;
		float f = 1.5;
		std::string s = "hello";

		fpmas::io::StringOutput output_string;
		// Field types are specified as template parameters
		fpmas::io::CsvOutput<int, int, float, std::string> local_output(
				// Output where data will be written
				output_string,
				// How each field is fetched at each dump() call can be
				// specified using lambda functions. When value are captured
				// by reference (using &), latest modifications are
				// automatically fetched when dump() is called.
				{"process", [] () {return fpmas::communication::WORLD.getRank();}},
				{"i", [&i] () {return i;}},
				{"f", [&f] () {return f;}},
				{"s", [&s] () {return s;}}
				);
		// Simply dumps data as CSV to output_string, independently on all
		// processes
		local_output.dump();

		// Updating local data
		i = 2;
		f = 0.f;
		s = "world";

		local_output.dump();

		// For demonstration purpose, std::cout might have been used directly
		// in `local_output`
		std::cout << output_string.str();
	}
	fpmas::finalize();
}
