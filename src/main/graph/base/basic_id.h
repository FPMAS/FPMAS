#ifndef BASIC_ID_H
#define BASIC_ID_H

#include "api/graph/base/id.h"

namespace FPMAS::graph::base {

	class BasicId : public FPMAS::api::graph::base::Id<BasicId> {
		private:
			unsigned long value = 0;

		public:
			BasicId& operator++() override {
				++value;
				return *this;
			}

			bool operator==(const BasicId& id) const override {
				return this->value == id.value;
			}

			std::size_t hash() const override {
				return std::hash<unsigned long>()(value);
			}

			operator std::string() const override {
				return std::to_string(value);
			}
	};
}

#endif
