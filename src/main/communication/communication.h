#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>

class Communication {
	private:
		int _size;
		int _rank;
	
		// Custom comunicators
	public:
		Communication();

		int size();
		int rank();

};
#endif
