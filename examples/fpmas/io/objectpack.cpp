#include "fpmas/model/serializer.h"
#include "fpmas/io/datapack.h"

FPMAS_DEFAULT_JSON_SET_UP();

using namespace fpmas::io::datapack;


void compound_objectpack() {
	std::vector<int> data {4, -12, 17};
	std::string str = "Hello";

	ObjectPack pack;
	pack.allocate(pack.size(data) + pack.size(str));
	pack.put(data);
	pack.put(str);

	std::cout << "pack: " << pack << std::endl;
	std::cout << "data: ";
	for(auto& item : pack.get<std::vector<int>>())
		std::cout << item << " ";
	std::cout << std::endl;
	std::cout << "str: " << pack.get<std::string>() << std::endl;
}

void base_objectpack() {
	std::string data = "Hello world";
	ObjectPack pack = data;

	std::cout << "pack: " << pack << std::endl;
	std::cout << "str: " << pack.get<std::string>() << std::endl;
}

struct Data {
	std::unordered_map<int, std::string> map {{4, "hey"}, {2, "ho"}};
	double x = 127.4;
};

namespace fpmas { namespace io { namespace datapack {
	template<>
		struct Serializer<Data> {
			static std::size_t size(const ObjectPack& pack, const Data& data) {
				// Computes the ObjectPack size required to serialize data
				return pack.size(data.map) + pack.size(data.x);
			}

			static void to_datapack(ObjectPack& pack, const Data& data) {
				// The buffer is pre-allocated, so there is no need to call
				// allocate()
				pack.put(data.map);
				pack.put(data.x);
			}

			static Data from_datapack(const ObjectPack& pack) {
				Data data;
				data.map = pack.get<std::unordered_map<int, std::string>>();
				data.x = pack.get<double>();
				return data;
			}
		};
}}}

void custom_objectpack() {
	Data data;
	// The required size is automatically allocated thanks to the custom
	// Serializer<Data>::size() method.
	ObjectPack pack = data;
	std::cout << "pack: " << pack << std::endl;
	Data unserial_data = pack.get<Data>();
	std::cout << "data.map: ";
	for(auto& item : unserial_data.map)
		std::cout << "(" << item.first << "," << item.second << ") ";
	std::cout << std::endl;
	std::cout << "data.x: " << unserial_data.x << std::endl;
}

void read_write() {
	std::uint64_t i = 23000;
	char str[6] = "abcde";

	ObjectPack pack;
	pack.allocate(sizeof(i) + sizeof(str));
	pack.write(i);
	pack.write(str, 3);

	std::cout << "pack: " << pack << std::endl;

	std::uint64_t u_i;
	char u_str[3];
	pack.read(u_i);
	pack.read(u_str, 3);

	std::cout << "u_i: " << u_i << std::endl;
	std::cout << "u_str: ";
	for(std::size_t i = 0; i < 3; i++)
		std::cout << u_str[i];
	std::cout << std::endl;
}

int main(int argc, char** argv) {
	compound_objectpack();

	base_objectpack();

	custom_objectpack();

	read_write();
}
