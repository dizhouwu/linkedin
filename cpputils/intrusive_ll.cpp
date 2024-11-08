#include <iostream>

// Define the ListHead structure (equivalent to struct list_head in Linux)
struct ListHead {
    ListHead *next, *prev;

    ListHead() : next(this), prev(this) {}

    // Insert a new entry between two known consecutive entries
    void insert_between(ListHead *prev_entry, ListHead *next_entry) {
        next = next_entry;
        prev = prev_entry;
        prev_entry->next = this;
        next_entry->prev = this;
    }

    // Add a new entry after the specified head (adding to the front)
    void add(ListHead *head) {
        insert_between(head, head->next);
    }

    // Add a new entry before the specified head (adding to the end)
    void add_tail(ListHead *head) {
        insert_between(head->prev, head);
    }

    // Remove an entry from the list
    void remove() {
        prev->next = next;
        next->prev = prev;
        next = this;
        prev = this;
    }

    // Check if the list is empty
    bool empty() const {
        return next == this;
    }
};

// Define a templated ListEntry to link any type of struct/class
template <typename T>
struct ListEntry {
    ListHead list;
    T data;

    ListEntry(const T &data) : data(data) {}

    // Utility to get the container class from ListHead pointer
    static ListEntry* from_list_head(ListHead *head) {
        return reinterpret_cast<ListEntry*>(reinterpret_cast<char*>(head) - offsetof(ListEntry, list));
    }
};

