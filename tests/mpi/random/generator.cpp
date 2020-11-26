#include "fpmas/random/generator.h"
#include "fpmas/communication/communication.h"

#include "gmock/gmock.h"

using namespace testing;

class DistributedRandomGeneratorTest : public Test {
	protected:
		static fpmas::random::DistributedGenerator<> distributed_generator;
};
fpmas::random::DistributedGenerator<> DistributedRandomGeneratorTest::distributed_generator;

TEST_F(DistributedRandomGeneratorTest, test) {
	std::array<typename fpmas::api::random::Generator::result_type, 100> values;
	for(int i = 0; i < 100; i++)
		values[i] = distributed_generator();
	fpmas::communication::TypedMpi<decltype(values)> mpi(
			fpmas::communication::MpiCommunicator::WORLD);

	std::vector<decltype(values)> generated_values = mpi.gather(values, 0);

	// Only significant on process 0
	for(std::size_t i = 0; i < generated_values.size(); i++)
		for(std::size_t j = 0; j < generated_values.size(); j++)
			if(i != j) {
				ASSERT_THAT(generated_values[i], Not(ElementsAreArray(generated_values[j])));
			}
}
