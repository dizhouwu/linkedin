#include <gtest/gtest.h>
#include "../wub_open_addressing_hashtable.cpp"

// Test fixture for HashTable tests
class HashTableTest : public ::testing::Test {
protected:
    HashTable<int, std::string> ht;
};

// Test insert and find operations
TEST_F(HashTableTest, InsertAndFind) {
    ht.insert(1, "one");
    ht.insert(2, "two");
    ht.insert(3, "three");

    auto value = ht.find(2);
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "two");
}

// Test remove operation
TEST_F(HashTableTest, Remove) {
    ht.insert(1, "one");
    ht.insert(2, "two");
    ht.insert(3, "three");

    ht.remove(2);

    auto value = ht.find(2);
    EXPECT_FALSE(value.has_value());
}

// Test load factor and rehashing on insert
TEST_F(HashTableTest, LoadFactorAndRehashInsert) {
    for (int i = 0; i < 7; ++i) {
        ht.insert(i, std::to_string(i));
    }
    size_t tableSize = ht.getTableSize();
    size_t numElements = ht.getSize();
    EXPECT_EQ(static_cast<double>(numElements) / tableSize, 7.0 / ht.getTableSize());
    ht.insert(7, "seven");
    EXPECT_EQ(static_cast<double>(ht.getSize()) / ht.getTableSize(), 8.0 / ht.getTableSize());
}

// Test rehashing if needed after removal
TEST_F(HashTableTest, RehashIfNeededAfterRemoval) {
    for (int i = 0; i < 10; ++i) {
        ht.insert(i, std::to_string(i));
    }
    for (int i = 0; i < 10; ++i) {  // Remove all 10 elements
        ht.remove(i);
    }
    size_t tableSize = ht.getTableSize();
    size_t numElements = ht.getSize();
    EXPECT_EQ(static_cast<double>(numElements) / tableSize, 0.0);
    EXPECT_EQ(ht.getTableSize(), 8);  // Should be reset to initial size (8)
}
