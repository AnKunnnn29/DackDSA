#pragma once
#include <algorithm>
#include <vector>
#include <string>
#include <functional>
#include "Models.h"

namespace shop {

/**
 * @brief Khóa phụ để đánh chỉ mục theo giá sản phẩm trong Cây AVL.
 * Hỗ trợ nhiều sản phẩm có cùng mức giá bằng cách so sánh thêm productId.
 */
struct PriceKey {
    Money price;
    std::string productId;

    bool operator<(const PriceKey& other) const {
        if (price != other.price) return price < other.price;
        return productId < other.productId;
    }
    bool operator>(const PriceKey& other) const {
        if (price != other.price) return price > other.price;
        return productId > other.productId;
    }
    bool operator==(const PriceKey& other) const {
        return price == other.price && productId == other.productId;
    }
    bool operator<=(const PriceKey& other) const {
        return *this < other || *this == other;
    }
    bool operator>=(const PriceKey& other) const {
        return *this > other || *this == other;
    }
};

/**
 * @brief Cây AVL tự cân bằng tự cài đặt từ đầu (Custom Balanced BST from scratch)
 * - Đảm bảo chiều cao luôn là O(log N).
 * - Phục vụ yêu cầu MC2: Truy xuất và duyệt theo khoảng giá (range query) có thứ tự tăng dần trong O(log N + K).
 */
template <typename K, typename V>
class AVLTree {
private:
    struct Node {
        K key;
        V value;
        Node* left;
        Node* right;
        int height;

        Node(const K& k, const V& v)
            : key(k), value(v), left(nullptr), right(nullptr), height(1) {}
    };

    Node* root;
    size_t nodeCount;

    int getHeight(Node* node) const {
        return node ? node->height : 0;
    }

    int getBalanceFactor(Node* node) const {
        return node ? getHeight(node->left) - getHeight(node->right) : 0;
    }

    void updateHeight(Node* node) {
        if (node) {
            node->height = 1 + std::max(getHeight(node->left), getHeight(node->right));
        }
    }

    Node* rotateRight(Node* y) {
        Node* x = y->left;
        Node* T2 = x->right;

        x->right = y;
        y->left = T2;

        updateHeight(y);
        updateHeight(x);

        return x;
    }

    Node* rotateLeft(Node* x) {
        Node* y = x->right;
        Node* T2 = y->left;

        y->left = x;
        x->right = T2;

        updateHeight(x);
        updateHeight(y);

        return y;
    }

    Node* rebalance(Node* node) {
        updateHeight(node);
        int balance = getBalanceFactor(node);

        // Trường hợp Lệch Trái - Trái (Left Left)
        if (balance > 1 && getBalanceFactor(node->left) >= 0) {
            return rotateRight(node);
        }

        // Trường hợp Lệch Trái - Phải (Left Right)
        if (balance > 1 && getBalanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
            return rotateRight(node);
        }

        // Trường hợp Lệch Phải - Phải (Right Right)
        if (balance < -1 && getBalanceFactor(node->right) <= 0) {
            return rotateLeft(node);
        }

        // Trường hợp Lệch Phải - Trái (Right Left)
        if (balance < -1 && getBalanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
            return rotateLeft(node);
        }

        return node;
    }

    Node* insertNode(Node* node, const K& key, const V& val, bool& inserted) {
        if (node == nullptr) {
            inserted = true;
            return new Node(key, val);
        }

        if (key < node->key) {
            node->left = insertNode(node->left, key, val, inserted);
        } else if (key > node->key) {
            node->right = insertNode(node->right, key, val, inserted);
        } else {
            // Đã tồn tại khóa, cập nhật giá trị
            node->value = val;
            inserted = false;
            return node;
        }

        return rebalance(node);
    }

    Node* findMinNode(Node* node) const {
        Node* current = node;
        while (current && current->left != nullptr) {
            current = current->left;
        }
        return current;
    }

    Node* removeNode(Node* node, const K& key, bool& removed) {
        if (node == nullptr) {
            removed = false;
            return nullptr;
        }

        if (key < node->key) {
            node->left = removeNode(node->left, key, removed);
        } else if (key > node->key) {
            node->right = removeNode(node->right, key, removed);
        } else {
            removed = true;
            // Node có 0 hoặc 1 con
            if (node->left == nullptr || node->right == nullptr) {
                Node* temp = node->left ? node->left : node->right;
                if (temp == nullptr) {
                    temp = node;
                    node = nullptr;
                } else {
                    *node = *temp;
                }
                delete temp;
            } else {
                // Node có 2 con: lấy successor nhỏ nhất ở cây con bên phải
                Node* temp = findMinNode(node->right);
                node->key = temp->key;
                node->value = temp->value;
                node->right = removeNode(node->right, temp->key, removed);
            }
        }

        if (node == nullptr) return nullptr;
        return rebalance(node);
    }

    void rangeCollect(Node* node, Money minPrice, Money maxPrice, std::vector<V>& results) const {
        if (node == nullptr) return;

        // Nếu nhánh trái có khả năng chứa phần tử trong khoảng [minPrice, maxPrice]
        if (node->key.price >= minPrice) {
            rangeCollect(node->left, minPrice, maxPrice, results);
        }

        // Nếu node hiện tại nằm trong khoảng giá
        if (node->key.price >= minPrice && node->key.price <= maxPrice) {
            results.push_back(node->value);
        }

        // Nếu nhánh phải có khả năng chứa phần tử trong khoảng [minPrice, maxPrice]
        if (node->key.price <= maxPrice) {
            rangeCollect(node->right, minPrice, maxPrice, results);
        }
    }

    void inOrderCollect(Node* node, std::vector<V>& results) const {
        if (node == nullptr) return;
        inOrderCollect(node->left, results);
        results.push_back(node->value);
        inOrderCollect(node->right, results);
    }

    void destroyTree(Node* node) {
        if (node != nullptr) {
            destroyTree(node->left);
            destroyTree(node->right);
            delete node;
        }
    }

public:
    AVLTree() : root(nullptr), nodeCount(0) {}

    ~AVLTree() {
        destroyTree(root);
    }

    // Không cho copy nông (shallow copy) để tránh double-delete
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    size_t size() const { return nodeCount; }
    bool empty() const { return nodeCount == 0; }

    bool insert(const K& key, const V& val) {
        bool inserted = false;
        root = insertNode(root, key, val, inserted);
        if (inserted) {
            nodeCount++;
        }
        return inserted;
    }

    bool remove(const K& key) {
        bool removed = false;
        root = removeNode(root, key, removed);
        if (removed) {
            nodeCount--;
        }
        return removed;
    }

    /**
     * @brief Truy vấn các phần tử có giá trong đoạn [minPrice, maxPrice]
     * Trả về vector phần tử đã được sắp xếp tăng dần theo giá trong thời gian O(log N + K).
     */
    std::vector<V> rangeQueryPrice(Money minPrice, Money maxPrice) const {
        std::vector<V> results;
        rangeCollect(root, minPrice, maxPrice, results);
        return results;
    }

    /**
     * @brief Lấy toàn bộ phần tử đã sắp xếp theo thứ tự in-order
     */
    std::vector<V> getAllSorted() const {
        std::vector<V> results;
        inOrderCollect(root, results);
        return results;
    }

    void clear() {
        destroyTree(root);
        root = nullptr;
        nodeCount = 0;
    }
};

} // namespace shop
