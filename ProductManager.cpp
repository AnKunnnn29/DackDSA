#include "ProductManager.h"

namespace shop {

Result ProductManager::addProduct(const Product& product) {
    if (product.productId.empty()) {
        return {ErrorCode::INVALID_INPUT, "Ma san pham khong duoc de trong", ""};
    }
    if (product.price < 0) {
        return {ErrorCode::INVALID_INPUT, "Gia san pham khong duoc am", ""};
    }
    if (product.stock < 0 || product.minStock < 0) {
        return {ErrorCode::INVALID_INPUT, "So luong ton kho khong duoc am", ""};
    }

    // Kiểm tra trùng ID (Uniqueness constraint)
    if (productTable.contains(product.productId)) {
        return {ErrorCode::DUPLICATE_ID, "Ma san pham da ton tai: " + product.productId, ""};
    }

    // Chèn vào Bảng băm (Primary Index)
    productTable.insert(product.productId, product);

    // Chèn vào Cây AVL (Secondary Index theo Giá)
    PriceKey pKey{product.price, product.productId};
    priceIndex.insert(pKey, product.productId);

    return {ErrorCode::NONE, "Them san pham thanh cong", ""};
}

const Product* ProductManager::getProduct(const std::string& productId) const {
    return productTable.find(productId);
}

Product* ProductManager::getProduct(const std::string& productId) {
    return productTable.find(productId);
}

Result ProductManager::updateProductInfo(const std::string& productId, const std::string& newName, Quantity newMinStock) {
    Product* prod = productTable.find(productId);
    if (!prod) {
        return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham: " + productId, ""};
    }
    if (newMinStock < 0) {
        return {ErrorCode::INVALID_INPUT, "Nguong ton kho khong duoc am", ""};
    }

    if (!newName.empty()) {
        prod->name = newName;
    }
    prod->minStock = newMinStock;

    return {ErrorCode::NONE, "Cap nhat thong tin san pham thanh cong", ""};
}

Result ProductManager::updateProductPrice(const std::string& productId, Money newPrice) {
    if (newPrice < 0) {
        return {ErrorCode::INVALID_INPUT, "Gia san pham khong the am", ""};
    }

    Product* prod = productTable.find(productId);
    if (!prod) {
        return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham: " + productId, ""};
    }

    Money oldPrice = prod->price;
    if (oldPrice == newPrice) {
        return {ErrorCode::NONE, "Gia san pham khong thay doi", ""};
    }

    // Đồng bộ chỉ mục Cây AVL: Xóa khóa giá cũ, thêm khóa giá mới
    PriceKey oldKey{oldPrice, productId};
    priceIndex.remove(oldKey);

    PriceKey newKey{newPrice, productId};
    priceIndex.insert(newKey, productId);

    // Cập nhật giá trong Bảng băm
    prod->price = newPrice;

    return {ErrorCode::NONE, "Cap nhat gia san pham thanh cong", ""};
}

Result ProductManager::removeProduct(const std::string& productId) {
    Product* prod = productTable.find(productId);
    if (!prod) {
        return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham can xoa", ""};
    }

    // Xóa khỏi Cây AVL
    PriceKey pKey{prod->price, productId};
    priceIndex.remove(pKey);

    // Xóa khỏi Bảng băm
    productTable.remove(productId);

    return {ErrorCode::NONE, "Xoa san pham thanh cong", ""};
}

std::vector<Product> ProductManager::getProductsByPriceRange(Money minPrice, Money maxPrice) const {
    std::vector<Product> result;
    if (minPrice > maxPrice) {
        return result; // Khoảng không hợp lệ
    }

    // Cây AVL trả về danh sách các productId thỏa mãn theo thứ tự tăng dần của giá
    std::vector<std::string> ids = priceIndex.rangeQueryPrice(minPrice, maxPrice);

    result.reserve(ids.size());
    for (const auto& id : ids) {
        const Product* prod = productTable.find(id);
        if (prod) {
            result.push_back(*prod);
        }
    }
    return result;
}

std::vector<Product> ProductManager::getAllProducts() const {
    return productTable.getAllValues();
}

size_t ProductManager::totalProducts() const {
    return productTable.size();
}

Result ProductManager::importStock(const std::string& productId, Quantity quantity) {
    if (quantity <= 0) {
        return {ErrorCode::INVALID_INPUT, "So luong nhap kho phai lon hon 0", ""};
    }

    Product* prod = productTable.find(productId);
    if (!prod) {
        return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham: " + productId, ""};
    }

    prod->stock += quantity;
    return {ErrorCode::NONE, "Nhap kho thanh cong", ""};
}

Result ProductManager::deductStock(const std::string& productId, Quantity quantity) {
    if (quantity <= 0) {
        return {ErrorCode::INVALID_INPUT, "So luong xuat kho phai lon hon 0", ""};
    }

    Product* prod = productTable.find(productId);
    if (!prod) {
        return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham: " + productId, ""};
    }

    if (prod->stock < quantity) {
        return {ErrorCode::INSUFFICIENT_STOCK, 
                "Ton kho khong du (Hien co: " + std::to_string(prod->stock) + 
                ", Can xuat: " + std::to_string(quantity) + ")", ""};
    }

    prod->stock -= quantity;
    return {ErrorCode::NONE, "Xuat kho thanh cong", ""};
}

Result ProductManager::restock(const std::string& productId, Quantity quantity) {
    if (quantity <= 0) {
        return {ErrorCode::INVALID_INPUT, "So luong hoan tra phai lon hon 0", ""};
    }

    Product* prod = productTable.find(productId);
    if (!prod) {
        return {ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham: " + productId, ""};
    }

    prod->stock += quantity;
    return {ErrorCode::NONE, "Hoan tra ton kho thanh cong", ""};
}

bool ProductManager::checkAvailability(const std::string& productId, Quantity requiredQuantity) const {
    if (requiredQuantity <= 0) return false;
    const Product* prod = productTable.find(productId);
    if (!prod) return false;
    return prod->stock >= requiredQuantity;
}

std::vector<Product> ProductManager::getLowStockAlerts() const {
    std::vector<Product> allProds = productTable.getAllValues();
    std::vector<Product> alerts;
    for (const auto& prod : allProds) {
        if (prod.stock <= prod.minStock) {
            alerts.push_back(prod);
        }
    }
    return alerts;
}

} // namespace shop
