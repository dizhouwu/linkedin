#include <gtest/gtest.h>
#include "../intrusive_ll.cpp"  // Assume the provided code is in linked_list.h

// Test fixture for setting up a list and items for testing
class ListHeadTest : public ::testing::Test {
protected:
    ListHead head;
    ListEntry<int> item1{1};
    ListEntry<int> item2{2};
    ListEntry<int> item3{3};
    
    void SetUp() override {
        // Reset head to ensure it's empty before each test
        head.next = &head;
        head.prev = &head;
    }
};

// Test if the list is empty initially
TEST_F(ListHeadTest, ListIsEmptyInitially) {
    EXPECT_TRUE(head.empty());
}

// Test adding a single item to the front
TEST_F(ListHeadTest, AddSingleItem) {
    item1.list.add(&head);
    EXPECT_FALSE(head.empty());
    EXPECT_EQ(head.next, &item1.list);
    EXPECT_EQ(head.prev, &item1.list);
}

// Test adding multiple items to the front
TEST_F(ListHeadTest, AddMultipleItems) {
    item1.list.add(&head);
    item2.list.add(&head);
    item3.list.add(&head);

    // Check if items are added in reverse order (stack-like behavior)
    EXPECT_EQ(head.next, &item3.list);
    EXPECT_EQ(head.next->next, &item2.list);
    EXPECT_EQ(head.next->next->next, &item1.list);
    EXPECT_EQ(head.prev, &item1.list);
}

// Test adding items to the end (tail)
TEST_F(ListHeadTest, AddItemsToTail) {
    item1.list.add_tail(&head);
    item2.list.add_tail(&head);
    item3.list.add_tail(&head);

    // Check if items are added in the same order (queue-like behavior)
    EXPECT_EQ(head.next, &item1.list);
    EXPECT_EQ(head.next->next, &item2.list);
    EXPECT_EQ(head.next->next->next, &item3.list);
    EXPECT_EQ(head.prev, &item3.list);
}

// Test removing an item
TEST_F(ListHeadTest, RemoveItem) {
    item1.list.add(&head);
    item2.list.add(&head);
    item1.list.remove();

    EXPECT_FALSE(head.empty());
    EXPECT_EQ(head.next, &item2.list);
    EXPECT_EQ(head.prev, &item2.list);

    item2.list.remove();
    EXPECT_TRUE(head.empty());
}

// Test removing all items after adding them
TEST_F(ListHeadTest, RemoveAllItems) {
    item1.list.add(&head);
    item2.list.add(&head);
    item3.list.add(&head);

    item1.list.remove();
    item2.list.remove();
    item3.list.remove();

    EXPECT_TRUE(head.empty());
}

// Test iterating through the list
TEST_F(ListHeadTest, IterateList) {
    item1.list.add_tail(&head);
    item2.list.add_tail(&head);
    item3.list.add_tail(&head);

    ListHead *pos = head.next;
    int expected_data[] = {1, 2, 3};
    int index = 0;

    while (pos != &head) {
        ListEntry<int> *entry = ListEntry<int>::from_list_head(pos);
        EXPECT_EQ(entry->data, expected_data[index++]);
        pos = pos->next;
    }
}

// Test from_list_head utility function
TEST_F(ListHeadTest, FromListHeadUtility) {
    item1.list.add(&head);
    ListEntry<int> *entry = ListEntry<int>::from_list_head(head.next);
    EXPECT_EQ(entry->data, 1);
}
