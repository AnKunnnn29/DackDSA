#pragma once
#include <string>
#include <vector>
#include "Models.h"
#include "HashTable.h"
#include "AVLTree.h"

namespace shop {

/**
 * @brief Tầng Core xử lý Quản lý Sản phẩm và Tồn kho (Product & Inventory Management)
 * Tích hợp 2 cấu trúc dữ liệu tự cài đặt từ đầu:
 *  1. HashTable (Primary Index theo productId) -> Tra cứu tức thời O(1) (MC1)
 *  2. AVLTree (Secondary Index theo Price) -> Lọc theo dải giá O(log N + K) có thứ tự (MC2)
 */
class ProductManager {
private:
    HashTable<std::string, Product> productTable;
    AVLTree<PriceKey, std::string> priceIndex;

public:
    ProductManager() = default;
    ~ProductManager() = default;

    // ==========================================
    // 1. QUẢN LÝ THÔNG TIN SẢN PHẨM
    // ==========================================

    /**
     * @brief Thêm mới một sản phẩm vào hệ thống.
     * Tự động đồng bộ vào Bảng băm và Cây AVL chỉ mục giá.
     */
    Result addProduct(const Product& product);

    /**
     * @brief Tra cứu thông tin chi tiết sản phẩm theo mã (MC1 - O(1)).
     */
    const Product* getProduct(const std::string& productId) const;
    Product* getProduct(const std::string& productId);

    /**
     * @brief Cập nhật thông tin cơ bản của sản phẩm (Tên, Ngưỡng cảnh báo tồn kho).
     */
    Result updateProductInfo(const std::string& productId, const std::string& newName, Quantity newMinStock);

    /**
     * @brief Cập nhật giá sản phẩm.
     * Tự động xóa khóa giá cũ và thêm khóa giá mới vào Cây AVL để bảo toàn tính sắp xếp.
     */
    Result updateProductPrice(const std::string& productId, Money newPrice);

    /**
     * @brief Xóa sản phẩm khỏi hệ thống (đồng bộ xóa cả ở Bảng băm và Cây AVL).
     */
    Result removeProduct(const std::string& productId);

    /**
     * @brief Truy vấn các sản phẩm có giá trong đoạn [minPrice, maxPrice] (MC2 - O(log N + K)).
     * Kết quả trả về đã được sắp xếp tăng dần theo giá.
     */
    std::vector<Product> getProductsByPriceRange(Money minPrice, Money maxPrice) const;

    /**
     * @brief Lấy danh sách tất cả sản phẩm hiện có trong hệ thống.
     */
    std::vector<Product> getAllProducts() const;

    /**
     * @brief Tổng số lượng loại sản phẩm trong danh mục.
     */
    size_t totalProducts() const;

    // ==========================================
    // 2. QUẢN LÝ SỐ LƯỢNG HÀNG TỒN KHO
    // ==========================================

    /**
     * @brief Nhập kho thêm số lượng cho một sản phẩm (Stock In).
     */
    Result importStock(const std::string& productId, Quantity quantity);

    /**
     * @brief Xuất trừ tồn kho khi đơn hàng được xác nhận hoặc xuất bán (Stock Out).
     * Đảm bảo tính phi âm của tồn kho vật lý (chống bán khống).
     */
    Result deductStock(const std::string& productId, Quantity quantity);

    /**
     * @brief Hoàn trả số lượng tồn kho (khi đơn hàng bị hủy / hoàn trả).
     */
    Result restock(const std::string& productId, Quantity quantity);

    /**
     * @brief Kiểm tra tính khả dụng của hàng tồn kho cho một số lượng yêu cầu.
     */
    bool checkAvailability(const std::string& productId, Quantity requiredQuantity) const;

    /**
     * @brief Cảnh báo danh sách sản phẩm sắp hết hàng (stock <= minStock).
     */
    std::vector<Product> getLowStockAlerts() const;
};

} // namespace shop
