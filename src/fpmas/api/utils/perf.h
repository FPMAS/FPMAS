#ifndef FPMAS_PERF_API_H
#define FPMAS_PERF_API_H

/** \file src/fpmas/api/utils/perf.h
 * Performance analysis API
 */

#include <chrono>
#include <string>
#include <vector>

namespace fpmas { namespace api { namespace utils { namespace perf {

	/**
	 * Standard clock used to perform durations measurements.
	 */
	typedef std::chrono::steady_clock Clock;
	/**
	 * The `duration` type used by Clock.
	 */
	typedef std::chrono::steady_clock::duration Duration;
	/**
	 * The `time_point` type used by Clock.
	 */
	typedef std::chrono::steady_clock::time_point ClockTime;

	/**
	 * Probe API.
	 *
	 * A Probe can be used to perform duration measurements.
	 *
	 * The probe measures data associated to a given label().
	 * Time measurements are performed between consecutive start() and end()
	 * calls, and added to the durations() list, so that they can be committed
	 * later to a Monitor.
	 *
	 * @par example
	 * ```cpp
	 * fpmas::utils::perf::Monitor monitor;
	 * fpmas::utils::perf::Probe probe("method_time");
	 *
	 * probe.start();
	 * call_some_method();
	 * probe.end();
	 *
	 * monitor.commit(probe);
	 *
	 * // count() displays the number of ticks that corresponds
	 * // to fpmas::api::utils::perf::Duration
	 * std::cout << monitor.totalDuration("method_time").count() << std::endl;
	 * ```
	 */
	class Probe {
		public:
			/**
			 * Label associated to measured data.
			 *
			 * Notice that it is allowed to have several \Probes associated to
			 * the same label.
			 *
			 * @return probe label
			 */
			virtual std::string label() const = 0;
			/**
			 * Returns a reference to durations currently buffered within this
			 * probe.
			 *
			 * Since a reference is returned, the buffer can eventually be
			 * cleared by an external process.
			 *
			 * @return reference to currently buffered durations
			 */
			virtual std::vector<Duration>& durations() = 0;

			/**
			 * Returns a reference to durations currently buffered within this
			 * probe.
			 *
			 * @return reference to currently buffered durations
			 */
			virtual const std::vector<Duration>& durations() const = 0;

			/**
			 * Starts a duration measurement with this probe.
			 *
			 * @note
			 * Starting an already running Probe currently produces an
			 * undefined behavior.
			 */
			virtual void start() = 0;
			/**
			 * Stops a duration measurement, previously started with start().
			 *
			 * The duration between start() and end() calls is added to
			 * durations().
			 *
			 * @note
			 * Stopping a Probe that is not running currently produces an
			 * undefined behavior.
			 */
			virtual void stop() = 0;

			virtual ~Probe() {}
	};

	/**
	 * Monitor API.
	 *
	 * A Monitor can be used to gather duration measurement performed by
	 * Probes.
	 */
	class Monitor {
		public:
			/**
			 * Commits measures performed by `probe` to this monitor.
			 *
			 * 1. Durations contained in `probe.durations()` are added to the
			 * value returned by `totalDuration(probe.label())`.
			 * 2. The count of durations in `probe.durations()` is added to the
			 * value returned by `callCount(probe.label())`
			 * 3. The `probe.durations()` buffer is cleared.
			 *
			 * @note
			 * Committing a running Probe currently produces an undefined
			 * behavior.
			 *
			 * @param probe probe to commit
			 */
			virtual void commit(Probe& probe) = 0;
			/**
			 * Returns the total number of calls that has been performed (and
			 * committed) with \Probes associated to `probe_label`.
			 *
			 * A "call" is defined as a duration measurement, i.e. a pair of
			 * `probe.start()` / `probe.stop()` call.
			 *
			 * If no probe with `probe_label` has been committed, 0 is
			 * returned.
			 *
			 * @param probe_label a Probe label
			 * @return call count corresponding to `probe_label`
			 */
			virtual std::size_t callCount(std::string probe_label) const = 0;

			/**
			 * Returns the total duration of measurements performed (and
			 * committed) with \Probes associated to `probe_label`.
			 *
			 * If no probe with `probe_label` has been committed, a null
			 * duration is returned.
			 *
			 * This methods returns an instance of
			 * fpmas::api::utils::perf::Duration, that is the default duration
			 * type associated to fpmas::api::utils::perf::Clock. However, this
			 * duration can be easily converted using the [C++ standard
			 * library](https://en.cppreference.com/w/cpp/chrono/duration).
			 *
			 * @par example
			 * ```cpp
			 * using namespace std::chrono;
			 * auto duration_in_ms = duration_cast<milliseconds>(monitor.totalDuration("foo"))
			 * ```
			 *
			 * @param probe_label a Probe label
			 * @return total duration of measures corresponding to `probe_label`
			 */
			virtual Duration totalDuration(std::string probe_label) const = 0;

			/**
			 * Clears the monitor content.
			 */
			virtual void clear() = 0;

			virtual ~Monitor() {}
	};
}}}}
#endif
