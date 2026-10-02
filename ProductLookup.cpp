#include "ProductLookup.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace std;

namespace shop {
namespace {
bool blank(const string& value) {
    return value.empty() || all_of(value.begin(), value.end(),
        [](unsigned char c) { return isspace(c) != 0; });
}
bool validId(const string& id) {
    return !blank(id) && none_of(id.begin(), id.end(),
        [](unsigned char c) { return isspace(c) != 0; });
}
Result failure(ErrorCode code, const string& message) {
    return {code, message, {}};
}
}
Result ProductService::addProduct(Product product) {
    if (!validId(product.productId) || blank(product.name) || product.price < 0 ||
        product.stock < 0 || product.minStock < 0)
        return failure(ErrorCode::INVALID_INPUT, "Thong tin san pham khong hop le.");
    auto key = product.productId;
    if (!products_.records.insert(move(key), move(product)))
        return failure(ErrorCode::DUPLICATE_ID, "Ma san pham da ton tai.");
    return {};
}
const Product* ProductService::findById(const string& id) const {
    return validId(id) ? products_.records.find(id) : nullptr;
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
