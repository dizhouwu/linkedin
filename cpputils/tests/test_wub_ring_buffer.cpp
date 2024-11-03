#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <optional>
#include "../wub_ring_buffer.cpp" // Adjust the path as needed

class LocklessRingBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = new LocklessRingBuffer<int>(5); // Size of 5 for testing
    }

    void TearDown() override {
        delete buffer;
    }

    LocklessRingBuffer<int>* buffer;
};

TEST_F(LocklessRingBufferTest, InitialStateIsEmpty) {
    EXPECT_TRUE(buffer->is_empty());
    EXPECT_FALSE(buffer->is_full());
}

TEST_F(LocklessRingBufferTest, PushAndPopSingleElement) {
    EXPECT_TRUE(buffer->push(42));
    EXPECT_FALSE(buffer->is_empty());
    EXPECT_EQ(buffer->pop(), 42);
    EXPECT_TRUE(buffer->is_empty());
}

TEST_F(LocklessRingBufferTest, PushUntilFull) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(buffer->push(i));
    }
    EXPECT_FALSE(buffer->push(5)); // Buffer should be full
    EXPECT_TRUE(buffer->is_full());
}

TEST_F(LocklessRingBufferTest, PopUntilEmpty) {
    for (int i = 0; i < 5; ++i) {
        buffer->push(i);
    }
    
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(buffer->pop(), i);
    }
    
    EXPECT_FALSE(buffer->pop().has_value()); // Buffer should be empty
    EXPECT_TRUE(buffer->is_empty());
}

TEST_F(LocklessRingBufferTest, AlternatePushPop) {
    EXPECT_TRUE(buffer->push(1));
    EXPECT_TRUE(buffer->push(2));
    EXPECT_EQ(buffer->pop(), 1);
    EXPECT_TRUE(buffer->push(3));
    EXPECT_EQ(buffer->pop(), 2);
    EXPECT_EQ(buffer->pop(), 3);
    EXPECT_TRUE(buffer->is_empty());
}

TEST_F(LocklessRingBufferTest, FullAndEmptyStates) {
    EXPECT_TRUE(buffer->push(1));
    EXPECT_TRUE(buffer->push(2));
    EXPECT_TRUE(buffer->push(3));
    EXPECT_TRUE(buffer->push(4));
    EXPECT_TRUE(buffer->push(5));
    EXPECT_TRUE(buffer->is_full());

    EXPECT_EQ(buffer->pop(), 1);
    EXPECT_EQ(buffer->pop(), 2);
    EXPECT_TRUE(buffer->push(6)); // Should succeed now
    EXPECT_FALSE(buffer->is_full());
}

TEST_F(LocklessRingBufferTest, SizeMustBeGreaterThanZero) {
    EXPECT_THROW(new LocklessRingBuffer<int>(0), std::invalid_argument);
    EXPECT_THROW(new LocklessRingBuffer<int>(-1), std::invalid_argument);
}

