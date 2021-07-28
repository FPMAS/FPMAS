#include "fpmas/io/output.h"

#include "communication/mock_communication.h"
#include "runtime/mock_runtime.h"

using namespace testing;

class FileOutput : public Test {
	private:
	std::vector<std::string> files_to_remove;

	protected:
	void check_file_content(std::string filename, std::string content) {
		files_to_remove.push_back(filename);
		std::ifstream file (filename, std::ios::in);
		ASSERT_TRUE(file.is_open());
		std::string _content (
				(std::istreambuf_iterator<char>(file)),
				(std::istreambuf_iterator<char>())
				);
		ASSERT_EQ(_content, content);
	}

	void TearDown() override {
		for(auto file : files_to_remove)
			std::remove(file.c_str());
	}
};

TEST_F(FileOutput, app) {
	{
		fpmas::io::FileOutput file("test.txt", std::ios::app);
		file.get() << "hello\n";
		file.get() << "hello";
	}

	check_file_content("test.txt", "hello\nhello");
}

class DynamicFileOutput : public FileOutput {
	protected:
		NiceMock<MockMpiCommunicator<3, 12>> mock_comm;
		NiceMock<MockRuntime> mock_runtime;

};

TEST_F(DynamicFileOutput, file_with_rank) {

	{
		fpmas::io::DynamicFileOutput file(
				"test.%r.%t.txt", mock_comm, mock_runtime,
				std::ios::app
				);

		ON_CALL(mock_runtime, currentDate)
			.WillByDefault(Return(4));

		file.get() << "hello from 4";
		file.get() << "\nother line";

		ON_CALL(mock_runtime, currentDate)
			.WillByDefault(Return(8));

		file.get() << "hello from 8";
	}

	check_file_content("test.3.4.txt", "hello from 4\nother line");
	check_file_content("test.3.8.txt", "hello from 8");
}

TEST_F(DynamicFileOutput, file_without_rank) {

	{
		fpmas::io::DynamicFileOutput file(
				"test.%t.txt", mock_comm, mock_runtime, std::ios::app);

		ON_CALL(mock_runtime, currentDate)
			.WillByDefault(Return(4));

		file.get() << "hello from 4";
		file.get() << "\nother line";

		ON_CALL(mock_runtime, currentDate)
			.WillByDefault(Return(8));

		file.get() << "hello from 8";
	}

	check_file_content("test.4.txt", "hello from 4\nother line");
	check_file_content("test.8.txt", "hello from 8");
}
