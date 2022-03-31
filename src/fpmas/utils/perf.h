#ifndef FPMAS_PERF_H
#define FPMAS_PERF_H

/** \file src/fpmas/utils/perf.h
 * Performance analysis implementation.
 */

#include <unordered_map>
#include <functional>
#include "fpmas/api/utils/perf.h"
#include "nlohmann/json.hpp"
#include "fpmas/io/csv_output.h"
#include "fpmas/api/model/model.h"

namespace fpmas { namespace utils { namespace perf {
	using api::utils::perf::Clock;
	using api::utils::perf::ClockTime;
	using api::utils::perf::Duration;

	/**
	 * fpmas::api::utils::perf::Probe implementation.
	 */
	class Probe : public api::utils::perf::Probe {
		private:
			std::string _label;
			std::vector<Duration> _durations;
			ClockTime start_time;
			std::function<bool()> condition {[] {return true;}};
			bool running = false;

		public:
			/**
			 * Probe constructor.
			 *
			 * @param label probe label
			 */
			Probe(std::string label) : _label(label) {}
			/**
			 * Conditionnal probe constructor.
			 *
			 * When start() is called, the probe effectively perform a measure
			 * if the `condition` is true. Notice that even if `condition` is
			 * false, stop() still must be called, even if it has no visible
			 * effect.
			 *
			 * Even if any
			 * [std::function](https://en.cppreference.com/w/cpp/utility/functional/function)
			 * can be provided, it might be useful to use [lambda
			 * functions](https://en.cppreference.com/w/cpp/language/lambda) to
			 * easily define conditions.
			 *
			 * @par example
			 * ```cpp
			 * bool measure = true;
			 * fpmas::utils::perf::Probe probe("example", [&measure] () {return measure;});
			 *
			 * probe.start();
			 * probe.stop();
			 * assert(probe.durations().size() == 1);
			 *
			 * // Disables probe
			 * measure = false;
			 *
			 * probe.start();
			 * probe.stop();
			 * // No measure was performed
			 * assert(probe.durations().size() == 1);
			 * ```
			 */
			Probe(std::string label, std::function<bool()> condition) :
				_label(label), condition(condition) {}

			std::string label() const override {
				return _label;
			}

			std::vector<Duration>& durations() override {
				return _durations;
			}
			const std::vector<Duration>& durations() const override {
				return _durations;
			}

			void start() override;
			void stop() override;
	};

	/**
	 * fpmas::api::utils::perf::Monitor implementation.
	 */
	class Monitor : public api::utils::perf::Monitor {
		private:
			mutable std::unordered_map<std::string, std::pair<std::size_t, Duration>> data;

		public:
			void commit(api::utils::perf::Probe& probe) override;
			std::size_t callCount(std::string probe_label) const override;
			Duration totalDuration(std::string probe_label) const override;
			void clear() override;
	};

	/**
	 * An api::model::Behavior implementation that wraps an other
	 * api::model::Behavior in order to estimate its execution time.
	 *
	 * The ProbeBehavior can be safely used instead of the probed behavior
	 * (assigning the ProbeBehavior to an \AgentGroup instead of the original
	 * behavior for example) without altering the \Model execution.
	 */
	class ProbeBehavior : public api::model::Behavior {
		private:
			api::utils::perf::Monitor& monitor;
			api::model::Behavior& behavior;
			mutable Probe probe;

		public:
			/**
			 * ProbeBehavior constructor.
			 *
			 * @param monitor monitor to which execution times will be
			 * committed
			 * @param behavior probed behavior
			 * @param probe_name name of the probe that is used to commit
			 * measures to the `monitor`. This name is also used to retrieve
			 * values using Monitor.callCount() and Monitor.totalDuration().
			 */
			ProbeBehavior(
					api::utils::perf::Monitor& monitor,
					api::model::Behavior& behavior,
					std::string probe_name
					);

			/**
			 * Executes the probed behavior on `agent`, probing and committing
			 * its execution time to the `monitor`.
			 *
			 * @param agent agent on which the probed behavior is executed
			 */
			void execute(api::model::Agent* agent) const override;
	};
}}}

namespace nlohmann {
	/**
	 * Json serialization rules for std::chrono::duration.
	 */
	template<typename Rep, typename Ratio>
		struct adl_serializer<std::chrono::duration<Rep, Ratio>> {
			/**
			 * Serializes an std::chrono::duration instance to the specified
			 * `json`.
			 *
			 * @param json json output
			 * @param duration duration to serialize
			 */
			template<typename JsonType>
				static void to_json(JsonType& json, const std::chrono::duration<Rep, Ratio>& duration) {
					json = duration.count();
				}
			/**
			 * Unserializes an std::chrono::duration instance from the
			 * specified `json`.
			 *
			 * @param json input json
			 * @param duration duration output
			 */
			template<typename JsonType>
				static void from_json(JsonType& json, std::chrono::duration<Rep, Ratio>& duration) {
					duration = std::chrono::duration<Rep, Ratio>(json.template get<Rep>());
				}
		};
}

namespace fpmas { namespace io { namespace datapack {

	template<typename Rep, typename Ratio>
		struct Serializer<std::chrono::duration<Rep, Ratio>> {
			template<typename Pack>
				static std::size_t size(
						const Pack& pack,
						const std::chrono::duration<Rep, Ratio>&) {
					return pack.template size<Rep>();
				}

			template<typename Pack>
				static void to_datapack(
						Pack& pack,
						const std::chrono::duration<Rep, Ratio>& duration) {
					pack.put(duration.count());
				}

			template<typename Pack>
				static std::chrono::duration<Rep, Ratio> from_datapack(
						const Pack& pack) {
					return std::chrono::duration<Rep, Ratio>(pack.template get<Rep>());
				}
		};
}}}

namespace fpmas { namespace io {
	/**
	 * fpmas::io::CsvSerial specialization for std::chrono::duration.
	 */
	template<typename Rep, typename Ratio>
		struct CsvSerial<std::chrono::duration<Rep, Ratio>> {
			/**
			 * Returns a string representation of `duration.count()`.
			 *
			 * Notice that the duration is represented as a raw integer (of
			 * type `Rep`), so `Ratio` must be known by other means to
			 * properly unserialize a duration from the returned string
			 * representation.
			 *
			 * @param duration duration to serialize
			 * @return duration string representation
			 */
			static std::string to_csv(const std::chrono::duration<Rep, Ratio>& duration) {
				std::ostringstream os;
				os << duration.count();
				return os.str();
			}
		};
}}
#endif
