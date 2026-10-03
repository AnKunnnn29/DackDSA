#pragma once
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <cstdint>

namespace shop {

/**
 * @brief Bảng băm tự cài đặt từ đầu (Custom Hash Table from scratch)
 * - Giải quyết đụng độ bằng phương pháp Separate Chaining (Danh sách liên kết đơn).
 * - Tự động mở rộng (rehashing) khi load factor vượt quá ngưỡng 0.75.
 * - Đáp ứng yêu cầu MC1: Tra cứu theo ID với độ phức tạp trung bình O(1).
 */
template <typename K, typename V>
class HashTable {
private:
    struct Node {
        K key;
        V value;
        Node* next;
        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    Node** table;
    size_t capacity;
    size_t elementCount;
    const float maxLoadFactor = 0.75f;

    // Danh sách các số nguyên tố dùng cho kích thước bảng để phân bố băm đều hơn
    static constexpr size_t PRIMES[] = {
        53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
        49157, 98317, 196613, 393241, 786433, 1572869, 3145739
    };
    static constexpr size_t NUM_PRIMES = sizeof(PRIMES) / sizeof(PRIMES[0]);

    // Hàm băm FNV-1a chuyên dụng cho chuỗi string, hoặc hash chuẩn cho các kiểu khác
    size_t hashKey(const K& key) const {
        if constexpr (std::is_same_v<K, std::string>) {
            uint64_t hash = 14695981039346656037ULL;
            for (char c : key) {
                hash ^= static_cast<uint8_t>(c);
                hash *= 1099511628211ULL;
            }
            return static_cast<size_t>(hash % capacity);
        } else {
            return std::hash<K>{}(key) % capacity;
        }
    }

    size_t getNextPrime(size_t currentCap) const {
        for (size_t i = 0; i < NUM_PRIMES; ++i) {
            if (PRIMES[i] > currentCap) return PRIMES[i];
        }
        return currentCap * 2 + 1;
    }

    void rehash() {
        size_t oldCapacity = capacity;
        Node** oldTable = table;

        capacity = getNextPrime(oldCapacity);
        table = new Node*[capacity]();
        elementCount = 0;

        for (size_t i = 0; i < oldCapacity; ++i) {
            Node* current = oldTable[i];
            while (current != nullptr) {
                Node* nextNode = current->next;
                insert(current->key, current->value);
                delete current;
                current = nextNode;
            }
        }
        delete[] oldTable;
    }

    void cleanup() {
        if (table == nullptr) return;
        for (size_t i = 0; i < capacity; ++i) {
            Node* current = table[i];
            while (current != nullptr) {
                Node* temp = current;
                current = current->next;
                delete temp;
            }
        }
        delete[] table;
        table = nullptr;
        elementCount = 0;
    }

public:
    explicit HashTable(size_t initialCapacity = 53)
        : capacity(initialCapacity), elementCount(0) {
        table = new Node*[capacity]();
    }

    ~HashTable() {
        cleanup();
    }

    HashTable(const HashTable& other) : capacity(other.capacity), elementCount(0) {
        table = new Node*[capacity]();
        for (size_t i = 0; i < other.capacity; ++i) {
            Node* curr = other.table[i];
            while (curr != nullptr) {
                insert(curr->key, curr->value);
                curr = curr->next;
            }
        }
    }

    HashTable& operator=(const HashTable& other) {
        if (this != &other) {
            cleanup();
            capacity = other.capacity;
            table = new Node*[capacity]();
            for (size_t i = 0; i < other.capacity; ++i) {
                Node* curr = other.table[i];
                while (curr != nullptr) {
                    insert(curr->key, curr->value);
                    curr = curr->next;
                }
            }
        }
        return *this;
    }

    size_t size() const { return elementCount; }
    bool empty() const { return elementCount == 0; }

    /**
     * @brief Thêm cặp (key, value). Trả về false nếu key đã tồn tại.
     */
    bool insert(const K& key, const V& value) {
        if ((float)(elementCount + 1) / capacity > maxLoadFactor) {
            rehash();
        }

        size_t idx = hashKey(key);
        Node* current = table[idx];

        while (current != nullptr) {
            if (current->key == key) {
                return false; // Trùng khóa định danh
            }
            current = current->next;
        }

        // Chèn vào đầu danh sách liên kết
        Node* newNode = new Node(key, value);
        newNode->next = table[idx];
        table[idx] = newNode;
        elementCount++;
        return true;
    }

    /**
     * @brief Tìm kiếm theo key. Trả về con trỏ tới Value, hoặc nullptr nếu không tìm thấy.
     */
    V* find(const K& key) {
        size_t idx = hashKey(key);
        Node* current = table[idx];
        while (current != nullptr) {
            if (current->key == key) {
                return &(current->value);
            }
            current = current->next;
        }
        return nullptr;
    }

    const V* find(const K& key) const {
        size_t idx = hashKey(key);
        Node* current = table[idx];
        while (current != nullptr) {
            if (current->key == key) {
                return &(current->value);
            }
            current = current->next;
        }
        return nullptr;
    }

    bool contains(const K& key) const {
        return find(key) != nullptr;
    }

    /**
     * @brief Xóa phần tử theo key.
     */
    bool remove(const K& key) {
        size_t idx = hashKey(key);
        Node* current = table[idx];
        Node* prev = nullptr;

        while (current != nullptr) {
            if (current->key == key) {
                if (prev == nullptr) {
                    table[idx] = current->next;
                } else {
                    prev->next = current->next;
                }
                delete current;
                elementCount--;
                return true;
            }
            prev = current;
            current = current->next;
        }
        return false;
    }

    /**
     * @brief Trả về toàn bộ danh sách các giá trị đang lưu trong bảng
     */
    std::vector<V> getAllValues() const {
        std::vector<V> values;
        values.reserve(elementCount);
        for (size_t i = 0; i < capacity; ++i) {
            Node* curr = table[i];
            while (curr != nullptr) {
                values.push_back(curr->value);
                curr = curr->next;
            }
        }
        return values;
    }

    void clear() {
        cleanup();
        capacity = 53;
        table = new Node*[capacity]();
    }
};

} // namespace shop
