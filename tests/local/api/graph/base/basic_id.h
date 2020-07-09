#ifndef FPMAS_BASIC_ID_H
#define FPMAS_BASIC_ID_H

#include "fpmas/api/graph/base/id.h"
class BasicId : public fpmas::api::graph::Id<BasicId> {
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
#endif
