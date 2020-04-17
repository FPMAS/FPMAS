#include "gtest/gtest.h"

#include <mpi.h>
#include <unordered_map>
#include <thread>
#include <chrono>

#include "api/communication/resource_handler.h"
#include "communication/readers_writers.h"
#include "api/communication/mock_request_handler.h"
#include "utils/test.h"

using ::testing::Assign;
using ::testing::Sequence;

using namespace std::chrono_literals;

class TestFirstReadersWriters : public ::testing::Test {
	protected:
		MockRequestHandler requestHandler;
		FPMAS::communication::FirstReadersWriters readersWriters {DistributedId(0, 0), requestHandler, 0};
		
};

/*
 * Simple read test
 */
TEST_F(TestFirstReadersWriters, simple_read) {
	EXPECT_CALL(requestHandler, respondToRead(2, DistributedId(0, 0))).Times(1);

	// Procs 2 reads the data
	readersWriters.read(2);
	ASSERT_FALSE(readersWriters.isLocked());
}

/*
 * Simple acquire test.
 * When data is acquired, it should be locked.
 */
TEST_F(TestFirstReadersWriters, simple_acquire) {
	EXPECT_CALL(requestHandler, respondToAcquire(4, DistributedId(0, 0))).Times(1);

	// Procs 4 acquires the data
	readersWriters.acquire(4);
	ASSERT_TRUE(readersWriters.isLocked());

	// Give back
	readersWriters.release();
	ASSERT_FALSE(readersWriters.isLocked());
}

/*
 * Read while acquired test.
 * When a read is performed while data is acquired, response to read should
 * only happen when data is released.
 */
TEST_F(TestFirstReadersWriters, read_while_acquired) {
	EXPECT_CALL(requestHandler, respondToAcquire(4, DistributedId(0, 0)))
		.Times(1);

	// Procs 4 acquires the data
	readersWriters.acquire(4);

	// respond should not be called now, since the data is locked
	EXPECT_CALL(requestHandler, respondToRead(2, DistributedId(0, 0))).Times(0);
	// Procs 2 attempts to read the data while its locked
	readersWriters.read(2);

	// respond should be called upon release
	EXPECT_CALL(requestHandler, respondToRead(2, DistributedId(0, 0)))
		.Times(1);

	// Read response is sent to 2 only after the data has been released
	readersWriters.release();
}

/*
 * Multiple read while acquired test.
 *
 * Read requests received while data is acquired should be put in a waiting
 * queue.
 * Responses should then happen, in the right order, once the data is released.
 */
TEST_F(TestFirstReadersWriters, multiple_read_while_acquired) {
	Sequence seq;
	EXPECT_CALL(requestHandler, respondToAcquire(11, DistributedId(0, 0)))
		.InSequence(seq);
	readersWriters.acquire(11);

	for(int i = 0; i < 10; i++) {
		// Respond should not happen now, because the resource is locked
		EXPECT_CALL(requestHandler, respondToRead(i, DistributedId(0, 0))).Times(0);
		readersWriters.read(i);
	}
	
	for(int i = 0; i < 10; i++) {
		// Once data is released, all respond should happen
		// IN THE ORDER of the original read requests 
		EXPECT_CALL(requestHandler, respondToRead(i, DistributedId(0, 0)))
			.InSequence(seq);
	}
	readersWriters.release();
}


/*
 * Acquire while acquired test.
 * When an acquire is performed while data is acquired, response to the second
 * acquire should only happen when data is released.
 */
TEST_F(TestFirstReadersWriters, acquire_while_acquired) {
	// First acquire should immediatly get its response
	EXPECT_CALL(requestHandler, respondToAcquire(4, DistributedId(0, 0))).Times(1);
	readersWriters.acquire(4);

	// Data is locked, so no respond for now
	EXPECT_CALL(requestHandler, respondToAcquire(2, DistributedId(0, 0))).Times(0);
	readersWriters.acquire(2);

	// Upon release, respond to 2
	EXPECT_CALL(requestHandler, respondToAcquire(2, DistributedId(0, 0))).Times(1);
	readersWriters.release();

	// Data is still locked, because 2 acquired it
	ASSERT_TRUE(readersWriters.isLocked());

	// 2 releases the data
	readersWriters.release();
	ASSERT_FALSE(readersWriters.isLocked());
}

TEST_F(TestFirstReadersWriters, multiple_acquire_while_acquired) {
	Sequence seq;
	EXPECT_CALL(requestHandler, respondToAcquire(11, DistributedId(0, 0)))
		.InSequence(seq);
	readersWriters.acquire(11);

	for(int i = 0; i < 10; i++) {
		// Respond should not happen now, because the resource is locked
		EXPECT_CALL(requestHandler, respondToAcquire(i, DistributedId(0, 0))).Times(0);
		readersWriters.acquire(i);
	}
	
	for(int i = 0; i < 10; i++) {
		// Once data is released, all respond should happen
		// IN THE ORDER of the original acquire requests 
		EXPECT_CALL(requestHandler, respondToAcquire(i, DistributedId(0, 0)))
			.InSequence(seq);
		readersWriters.release();
		ASSERT_TRUE(readersWriters.isLocked());
	}
	readersWriters.release();
	ASSERT_FALSE(readersWriters.isLocked());
}

TEST_F(TestFirstReadersWriters, multiple_read_and_acquire_while_acquired) {
	Sequence seq;
	EXPECT_CALL(requestHandler, respondToAcquire(11, DistributedId(0, 0)))
		.InSequence(seq);
	readersWriters.acquire(11);

	for(int i = 0; i < 5; i++) {
		// Respond should not happen now, because the resource is locked
		EXPECT_CALL(requestHandler, respondToAcquire(i, DistributedId(0, 0))).Times(0);
		readersWriters.read(i);
	}
	for(int i = 0; i < 5; i++) {
		// Respond should not happen now, because the resource is locked
		EXPECT_CALL(requestHandler, respondToAcquire(i, DistributedId(0, 0))).Times(0);
		readersWriters.acquire(i);
	}

	for(int i = 0; i < 5; i++) {
		// Respond should not happen now, because the resource is locked
		EXPECT_CALL(requestHandler, respondToRead(i, DistributedId(0, 0)))
			.InSequence(seq);
	}
	EXPECT_CALL(requestHandler, respondToAcquire(0, DistributedId(0, 0)))
		.InSequence(seq);
	readersWriters.release();
	ASSERT_TRUE(readersWriters.isLocked());
	
	for(int i = 1; i < 5; i++) {
		// Once data is released, all respond should happen
		// IN THE ORDER of the original acquire requests 
		EXPECT_CALL(requestHandler, respondToAcquire(i, DistributedId(0, 0)))
			.InSequence(seq);
		readersWriters.release();
		ASSERT_TRUE(readersWriters.isLocked());
	}
	readersWriters.release();
	ASSERT_FALSE(readersWriters.isLocked());

};
