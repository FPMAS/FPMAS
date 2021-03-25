#include "output.h"
#include "fpmas/utils/format.h"

namespace fpmas { namespace io {
FileOutput::FileOutput(
		std::string filename,
		std::ios_base::openmode mode
		)
	: file(filename, mode) {}
FileOutput::FileOutput(
				std::string file_format,
				int rank,
				std::ios_base::openmode mode
		)
	: file(fpmas::utils::format(file_format, rank), mode) {}

FileOutput::FileOutput(
		std::string file_format,
		api::scheduler::TimeStep step,
		std::ios_base::openmode mode
		)
	: file(fpmas::utils::format(file_format, step), mode) {}

FileOutput::FileOutput(
		std::string file_format,
		int rank,
		api::scheduler::TimeStep step,
		std::ios_base::openmode mode
		)
	: file(fpmas::utils::format(file_format, rank, step), mode) {}

}}
