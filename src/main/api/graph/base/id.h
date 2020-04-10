#ifndef ID_API_H
#define ID_API_H

#include <functional>
#include "nlohmann/json.hpp"

namespace FPMAS::api::graph::base {

	template<typename IdImpl>
	class Id {
		public:
			virtual IdImpl& operator++() = 0;

			virtual bool operator==(const IdImpl& id) const = 0;

			virtual std::size_t hash() const = 0;

			virtual operator std::string() const = 0;

			virtual ~Id() {}
	};

	template<typename IdImpl>
	struct IdHash {
		std::size_t operator()(const IdImpl& id) const {
			return id.hash();
		}
	};
}
#endif

