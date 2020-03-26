#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "mpi.h"
#include <set>
#include <unordered_map>

namespace FPMAS::graph::parallel {
	template<typename Node_t>
		class Scheduler {
			private:
				mutable std::unordered_map<int, std::set<Node_t*>> scheduleMap;
			public:
				void setSchedule(int time, Node_t*);
				const std::set<Node_t*>& get(int time) const;
				const std::unordered_map<int, std::set<Node_t*>>& get() const;
				int getSchedule(const Node_t*) const;
				void unschedule(const Node_t*);

				std::set<int> gatherPeriods(MPI_Comm);

		};

	template<typename Node_t>
		void Scheduler<Node_t>::setSchedule(int time, Node_t* node) {
			scheduleMap[time].insert(node);
			node->data()->_schedule = time;
		}

	template<typename Node_t>
		const std::set<Node_t*>& Scheduler<Node_t>::get(int time) const {
			return scheduleMap[time];
		}
	template<typename Node_t>
		const std::unordered_map<int, std::set<Node_t*>>&
		Scheduler<Node_t>::get() const {
			return scheduleMap;
		}

	template<typename Node_t>
		int Scheduler<Node_t>::getSchedule(const Node_t* node) const {
			return node->data()->schedule();
		}

	template<typename Node_t>
		void Scheduler<Node_t>::unschedule(const Node_t* node) {
			scheduleMap.at(getSchedule(node)).erase(const_cast<Node_t*>(node));
		}

	template<typename Node_t>
		std::set<int> Scheduler<Node_t>::gatherPeriods(MPI_Comm comm) {
			int size;
			int rank;
			MPI_Comm_size(comm, &size);
			MPI_Comm_rank(comm, &rank);
			int periodsCounts[size];
			periodsCounts[rank] = scheduleMap.size();
			MPI_Allgather(MPI_IN_PLACE, 0, MPI_INT, &periodsCounts, 1, MPI_INT, comm);

			int sum = 0;
			int displs[size];
			for(int i = 0; i < size; i++) {
				displs[i] = sum;
				sum += periodsCounts[i];
			}
			const int* const_periodsCounts = periodsCounts;
			const int* const_displs = displs;

			int periods[sum];
			int i = 0;
			for(auto scheduleMapItem : scheduleMap) {
				periods[displs[rank] + i++] = scheduleMapItem.first;
			}
			MPI_Allgatherv(
					MPI_IN_PLACE, 0, MPI_INT, &periods,
					const_periodsCounts, const_displs, MPI_INT, comm
					);

			std::set<int> periodsSet;

			for(int i = 0; i < sum; i++) {
				periodsSet.insert(periods[i]);
			}
			return periodsSet;
		}
}
#endif
