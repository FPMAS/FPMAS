#include "fpmas/io/output.h"
#include "gmock/gmock.h"

TEST(FileOutput, write_test) {
	{
		fpmas::io::FileOutput file("test_distributed_file_output.txt");

		if(fpmas::communication::WORLD.getRank() == 0)
			file.get() << "hello";
	}
	fpmas::communication::WORLD.barrier();
	std::ifstream file("test_distributed_file_output.txt", std::ios_base::in);
	std::string file_content {
			std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()
			};

	ASSERT_EQ(file_content, "hello");
}
