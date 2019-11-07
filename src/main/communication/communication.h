#include <cstdint>

class Communication {
	private:
		uint16_t _size;
		uint16_t _rank;
	
	public:
		Communication();

		uint16_t size();
		uint16_t rank();
};
