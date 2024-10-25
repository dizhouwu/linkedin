#include <iostream>
#include <vector>
#include <functional>
#include <optional>

template<typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>>
class HashTable {
public:
    HashTable(size_t initialSize = 8) : table(initialSize), size(0) {}

    size_t getSize() const { return size; }
    size_t getTableSize() const { return table.size(); }

    void insert(const KeyType& key, const ValueType& value) {
        if (loadFactor() >= 0.75) {
            rehash();
        }
        size_t index = calculateHash(key) % table.size();
        while (table[index].has_value()) {
            if (table[index]->first == key) {
                table[index]->second = value;
                return;
            }
            index = (index + 1) % table.size();
        }
        table[index] = std::make_pair(key, value);
        size++;
    }

    std::optional<ValueType> find(const KeyType& key) const {
        size_t index = calculateHash(key) % table.size();
        while (table[index].has_value()) {
            if (table[index]->first == key) {
                return table[index]->second;
            }
            index = (index + 1) % table.size();
        }
        return std::nullopt;
    }

    void remove(const KeyType& key) {
        size_t index = calculateHash(key) % table.size();
        bool removed = false;
        while (table[index].has_value()) {
            if (table[index]->first == key) {
                table[index] = std::nullopt;
                size--;
                removed = true;
                break;
            }
            index = (index + 1) % table.size();
        }
        if (removed) {
            rehashIfNeeded();
        }
    }

private:
    std::vector<std::optional<std::pair<KeyType, ValueType>>> table;
    size_t size;
    Hash hashFunction;

    double loadFactor() const {
        return static_cast<double>(size) / table.size();
    }

    size_t calculateHash(const KeyType& key) const {
        return hashFunction(key);
    }

    void rehash() {
        size_t newSize = table.size() * 2;
        std::vector<std::optional<std::pair<KeyType, ValueType>>> newTable(newSize);
        for (const auto& entry : table) {
            if (entry.has_value()) {
                size_t index = calculateHash(entry->first) % newSize;
                while (newTable[index].has_value()) {
                    index = (index + 1) % newSize;
                }
                newTable[index] = entry;
            }
        }
        table = newTable;
    }

    void rehashIfNeeded() {
        if (size == 0) {
            // Reset to initial size
            table.resize(8);
        } else if (loadFactor() <= 0.25 && table.size() > 8) {
            size_t newSize = table.size() / 2;
            std::vector<std::optional<std::pair<KeyType, ValueType>>> newTable(newSize);
            for (const auto& entry : table) {
                if (entry.has_value()) {
                    size_t index = calculateHash(entry->first) % newSize;
                    while (newTable[index].has_value()) {
                        index = (index + 1) % newSize;
                    }
                    newTable[index] = entry;
                }
            }
            table = newTable;
        }
    }
};
