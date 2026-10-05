#ifndef DACKDSA_PRODUCTMANAGEMENT_CPP
#define DACKDSA_PRODUCTMANAGEMENT_CPP
#include "OrderManager.cpp"

namespace shop {
using namespace std;
// Nam's scope. origin/Product contained no implementation at integration time.
class ProductManagementService {
    ProductStore& products_;
    OrderStore& orders_;
public:
    ProductManagementService(ProductStore& products, OrderStore& orders)
        : products_(products), orders_(orders) {}
    Result addProduct(Product product) {
        return ProductService(products_).addProduct(move(product));
    }
    Result updateInfo(const string& id, const string& name, Money price, Quantity minStock) {
        if (productLookupBlank(name) || price < 0 || minStock < 0)
            return {ErrorCode::INVALID_INPUT, "Thong tin san pham khong hop le.", {}};
        auto* product = products_.records.find(id);
        if (!product) return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham.", {}};
        string replacementName = name; // allocate before changing the record
        product->name.swap(replacementName);
        product->price = price;
        product->minStock = minStock;
        ++products_.revision;
        return {};
    }
    Result changeStock(const string& id, Quantity quantity, bool incoming) {
        if (quantity <= 0) return {ErrorCode::INVALID_INPUT, "So luong phai duong.", {}};
        auto* product = products_.records.find(id);
        if (!product) return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham.", {}};
        if (incoming && quantity > numeric_limits<Quantity>::max() - product->stock)
            return {ErrorCode::AMOUNT_OVERFLOW, "Ton kho vuot gioi han.", {}};
        if (!incoming && quantity > product->stock)
            return {ErrorCode::INSUFFICIENT_STOCK, "Khong du ton kho.", {}};
        product->stock = incoming ? product->stock + quantity : product->stock - quantity;
        ++products_.revision;
        return {};
    }
    Result updateClassification(const string& id, const string& category, const string& brand) {
        if (category.size() > 400 || brand.size() > 400 ||
            (!category.empty() && productLookupBlank(category)) || (!brand.empty() && productLookupBlank(brand)))
            return {ErrorCode::INVALID_INPUT, "Phan loai san pham khong hop le.", {}};
        auto* product = products_.records.find(id);
        if (!product) return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham.", {}};
        string replacementCategory = category, replacementBrand = brand;
        product->category.swap(replacementCategory);
        product->brand.swap(replacementBrand);
        ++products_.revision;
        return {};
    }
    Result removeProduct(const string& id) {
        if (!products_.records.find(id))
            return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham.", {}};
        bool referenced = false;
        orders_.records.forEach([&](const auto&, const Order& order) {
            for (const auto& item : order.items) if (item.productId == id) referenced = true;
        });
        if (referenced) return {ErrorCode::PRODUCT_IN_USE, "San pham da duoc luu trong don hang; khong xoa.", {}};
        products_.records.erase(id);
        ++products_.revision;
        return {};
    }
};
} // namespace shop
#endif
