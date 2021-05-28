#include "fpmas/io/output.h"

#include "communication/mock_communication.h"
#include "runtime/mock_runtime.h"

using namespace testing;

class DynamicFileOutput : public Test {
	protected:
		NiceMock<MockMpiCommunicator<3, 12>> mock_comm;
		NiceMock<MockRuntime> mock_runtime;

		void checkFileContent(std::string filename, std::string content) {
			std::ifstream file (filename, std::ios_base::in);
			ASSERT_TRUE(file.is_open());
			std::string _content (
					(std::istreambuf_iterator<char>(file)),
					(std::istreambuf_iterator<char>())
					);
			ASSERT_EQ(_content, content);
			std::remove(filename.c_str());
		}
};

TEST_F(DynamicFileOutput, file_with_rank) {

	{
		fpmas::io::DynamicFileOutput file("test.%r.%t.txt", mock_comm, mock_runtime);

		ON_CALL(mock_runtime, currentDate)
			.WillByDefault(Return(4));

		file.get() << "hello from 4";

		ON_CALL(mock_runtime, currentDate)
			.WillByDefault(Return(8));

		file.get() << "hello from 8";
	}

	checkFileContent("test.3.4.txt", "hello from 4");
	checkFileContent("test.3.8.txt", "hello from 8");
}

TEST_F(DynamicFileOutput, file_without_rank) {

	{
		fpmas::io::DynamicFileOutput file("test.%t.txt", mock_comm, mock_runtime);

		ON_CALL(mock_runtime, currentDate)
			.WillByDefault(Return(4));

		file.get() << "hello from 4";

		ON_CALL(mock_runtime, currentDate)
			.WillByDefault(Return(8));

		file.get() << "hello from 8";
	}

	checkFileContent("test.4.txt", "hello from 4");
	checkFileContent("test.8.txt", "hello from 8");
}
