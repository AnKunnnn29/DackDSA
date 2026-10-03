#ifndef DACKDSA_LOWSTOCK_CPP
#define DACKDSA_LOWSTOCK_CPP
#include "ProductLookup.cpp"

namespace shop {
class LowStockService {
    const ProductStore& products_;
public:
    explicit LowStockService(const ProductStore& products) : products_(products) {}
    vector<Product> warnings() const {
        vector<Product> result;
        products_.records.forEach([&](const auto&, const Product& p) {
            if (p.stock <= p.minStock) result.push_back(p);
        });
        sort(result.begin(), result.end(), [](const Product& a, const Product& b) { return a.productId < b.productId; });
        return result;
    }
};
} // namespace shop
#endif
