#include "output.h"
#include "fpmas/utils/format.h"

namespace fpmas { namespace io {
	FileOutput::FileOutput(
			std::string filename,
			std::ios::openmode mode
			)
		: filename(filename), mode(mode) {}

	FileOutput::FileOutput(
			std::string file_format,
			int rank,
			std::ios::openmode mode
			)
		: filename(fpmas::utils::format(file_format, rank)), mode(mode) {}

	FileOutput::FileOutput(
			std::string file_format,
			api::scheduler::TimeStep step,
			std::ios::openmode mode
			)
		: filename(fpmas::utils::format(file_format, step)), mode(mode) {}

	FileOutput::FileOutput(
			std::string file_format,
			int rank,
			api::scheduler::TimeStep step,
			std::ios::openmode mode
			)
		: filename(fpmas::utils::format(file_format, rank, step)), mode(mode) {}

	std::ofstream& FileOutput::get() {
		if(!file.is_open())
			file = std::ofstream {filename, mode};
		return file;
	}

}}
