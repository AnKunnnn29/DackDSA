#ifndef DACKDSA_PRODUCTLOOKUP_CPP
#define DACKDSA_PRODUCTLOOKUP_CPP

#include "Models.h"
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace dsa {
using namespace std;
// Separate chaining; nodes keep their addresses during rehash.
// Vector is only the bucket storage, not the lookup implementation.
template<class T>
class HashTable {
    struct Node {
        string key;
        T value;
        unique_ptr<Node> next;
        Node(string k, T v) : key(move(k)), value(move(v)) {}
    };
    vector<unique_ptr<Node>> buckets_;
    size_t size_ = 0;

    static size_t index(const string& key, size_t count) noexcept {
        uint64_t hash = 14695981039346656037ULL;
        for (unsigned char c : key) {
            hash ^= c;
            hash *= 1099511628211ULL;
        }
        return static_cast<size_t>(hash % count);
    }
    void rehash(size_t count) {
        vector<unique_ptr<Node>> replacement(count);
        for (auto& head : buckets_) {
            while (head) {
                auto node = move(head);
                head = move(node->next);
                auto i = index(node->key, count);
                node->next = move(replacement[i]);
                replacement[i] = move(node);
            }
        }
        buckets_.swap(replacement);
    }
public:
    void swap(HashTable& other) noexcept {
        buckets_.swap(other.buckets_);
        std::swap(size_, other.size_);
    }
    explicit HashTable(size_t buckets = 16) : buckets_(buckets ? buckets : 1) {}
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;
    ~HashTable() {
        // Iterative destruction avoids stack overflow on a long collision chain.
        for (auto& head : buckets_) {
            while (head) {
                auto node = move(head);
                head = move(node->next);
            }
        }
    }
    size_t size() const noexcept { return size_; }
    size_t bucketCount() const noexcept { return buckets_.size(); }
    const T* find(const string& key) const noexcept {
        auto node = buckets_[index(key, buckets_.size())].get();
        while (node) {
            if (node->key == key) return &node->value;
            node = node->next.get();
        }
        return nullptr;
    }
    T* find(const string& key) noexcept {
        return const_cast<T*>(as_const(*this).find(key));
    }
    bool insert(string key, T value) {
        if (find(key)) return false;
        auto node = make_unique<Node>(move(key), move(value));
        if (size_ + 1 > buckets_.size() * 3 / 4) rehash(buckets_.size() * 2);
        auto i = index(node->key, buckets_.size());
        node->next = move(buckets_[i]);
        buckets_[i] = move(node);
        ++size_;
        return true;
    }
    bool erase(const string& key) noexcept {
        auto* link = &buckets_[index(key, buckets_.size())];
        while (*link) {
            if ((*link)->key == key) {
                auto removed = move(*link);
                *link = move(removed->next);
                --size_;
                return true;
            }
            link = &(*link)->next;
        }
        return false;
    }
    template<class Function>
    void forEach(Function function) const {
        for (const auto& head : buckets_)
            for (auto* node = head.get(); node; node = node->next.get())
                function(node->key, as_const(node->value));
    }
};
} // namespace dsa

namespace shop {
using namespace std;
struct ProductStore {
    dsa::HashTable<Product> records;
    uint64_t revision = 1;
};

class ProductService {
    ProductStore& products_;
public:
    explicit ProductService(ProductStore& products) : products_(products) {}
    Result addProduct(Product product);
    const Product* findById(const string& id) const;
    vector<Product> listProducts() const;
};
} // namespace shop

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace std;

namespace shop {
namespace {
bool productLookupBlank(const string& value) {
    return value.empty() || all_of(value.begin(), value.end(),
        [](unsigned char c) { return isspace(c) != 0; });
}
bool productLookupValidId(const string& id) {
    return !productLookupBlank(id) && none_of(id.begin(), id.end(),
        [](unsigned char c) { return isspace(c) != 0; });
}
Result productLookupFailure(ErrorCode code, const string& message) {
    return {code, message, {}};
}
}
Result ProductService::addProduct(Product product) {
    if (!productLookupValidId(product.productId) || productLookupBlank(product.name) || product.price < 0 ||
        product.stock < 0 || product.minStock < 0)
        return productLookupFailure(ErrorCode::INVALID_INPUT, "Thong tin san pham khong hop le.");
    auto key = product.productId;
    if (!products_.records.insert(move(key), move(product)))
        return productLookupFailure(ErrorCode::DUPLICATE_ID, "Ma san pham da ton tai.");
    ++products_.revision;
    return {};
}
const Product* ProductService::findById(const string& id) const {
    return productLookupValidId(id) ? products_.records.find(id) : nullptr;
}
vector<Product> ProductService::listProducts() const {
    vector<Product> products;
    products.reserve(products_.records.size());
    products_.records.forEach([&](const auto&, const Product& p) { products.push_back(p); });
    sort(products.begin(), products.end(), [](const Product& a, const Product& b) {
        return a.productId < b.productId;
    });
    return products;
}
} // namespace shop

#endif // DACKDSA_PRODUCTLOOKUP_CPP
