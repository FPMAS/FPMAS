#include "fpmas/io/csv_output.h"
#include "fpmas/runtime/runtime.h"
#include "../mocks/scheduler/mock_scheduler.h"
#include "gmock/gmock.h"

TEST(CsvOutputBase, job) {
	struct MockCsvOuput : public fpmas::io::CsvOutputBase<int, float> {
		using fpmas::io::CsvOutputBase<int, float>::CsvOutputBase;

		MockCsvOuput(fpmas::api::io::OutputStream& o,
				std::pair<std::string, std::function<int()>> f,
				std::pair<std::string, std::function<float()>> g
				)
			: fpmas::io::CsvOutputBase<int, float>(o, f, g) {}

		MOCK_METHOD(void, dump, (), (override));
	};

	fpmas::io::StringOutput out_stream;
	MockCsvOuput csv_out(out_stream,
			{"f", [] () {return 0;}},
			{"g", [] () {return 1.f;}});

	MockScheduler scheduler;
	fpmas::runtime::Runtime runtime(scheduler);

	EXPECT_CALL(csv_out, dump);
	runtime.execute(csv_out.job());
}

TEST(CsvOutput, output) {
	int i = 0;
	std::string s {"hello"};
	fpmas::io::StringOutput out;

	fpmas::io::CsvOutput<int, std::string> csv_out {
		out,
		{"field_1", [&i] () {return i;}},
		{"field_2", [&s] () {return s;}}
	};
	csv_out.dump();

	i = 4;
	s = "world";
	csv_out.dump();

	ASSERT_EQ(
			out.str(),
			"field_1,field_2\n"
			"0,hello\n"
			"4,world\n");
}
