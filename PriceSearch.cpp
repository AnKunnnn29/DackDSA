#ifndef DACKDSA_PRICESEARCH_CPP
#define DACKDSA_PRICESEARCH_CPP
#include "ProductLookup.cpp"

namespace shop {
using namespace std;
class PriceSearchService {
    const ProductStore& products_;
    vector<Product> sorted_;
    uint64_t seenRevision_ = 0;
    void refresh() {
        if (seenRevision_ == products_.revision) return;
        vector<Product> replacement;
        replacement.reserve(products_.records.size());
        products_.records.forEach([&](const auto&, const Product& p) { replacement.push_back(p); });
        sort(replacement.begin(), replacement.end(), [](const Product& a, const Product& b) {
            return a.price != b.price ? a.price < b.price : a.productId < b.productId;
        });
        sorted_.swap(replacement);
        seenRevision_ = products_.revision;
    }
public:
    explicit PriceSearchService(const ProductStore& products) : products_(products) {}
    Result findInRange(Money minPrice, Money maxPrice, vector<Product>& result) {
        result.clear();
        if (minPrice < 0 || maxPrice < minPrice)
            return {ErrorCode::INVALID_INPUT, "Khoang gia khong hop le.", {}};
        refresh();
        // Binary search lower_bound: first price >= minPrice.
        size_t left = 0, right = sorted_.size();
        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (sorted_[mid].price < minPrice) left = mid + 1;
            else right = mid;
        }
        const size_t begin = left;
        // Binary search upper_bound: first price > maxPrice.
        right = sorted_.size();
        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (sorted_[mid].price <= maxPrice) left = mid + 1;
            else right = mid;
        }
        result.assign(sorted_.begin() + begin, sorted_.begin() + left);
        return {};
    }
};
} // namespace shop
#endif
