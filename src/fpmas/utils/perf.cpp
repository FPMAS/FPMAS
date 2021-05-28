#include "perf.h"


namespace fpmas { namespace utils { namespace perf {


	void Probe::start() {
		if(!running && condition()) {
			running = true;
			start_time = Clock::now();
		}
	}

	void Probe::stop() {
		if(running) {
			_durations.push_back(Clock::now() - start_time);
			running = false;
		}
	}

	void Monitor::commit(api::utils::perf::Probe& probe) {
		for(auto duration : probe.durations()) {
			auto& item = data[probe.label()];
			item.first++;
			item.second+=duration;
		}
		probe.durations().clear();
	}

	std::size_t Monitor::callCount(std::string probe_label) const {
		return data[probe_label].first;
	}

	Duration Monitor::totalDuration(std::string probe_label) const {
		return data[probe_label].second;
	}
}}}
