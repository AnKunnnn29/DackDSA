#pragma once
#include <string>
#include <vector>
#include "Models.h"
#include "ProductManager.h"

namespace shop {

/**
 * @brief Tầng Persistence (Lưu trữ bền vững - Mục 7.3 theo yêu cầu đồ án)
 * Nhiệm vụ:
 *  - Nạp dữ liệu sản phẩm từ file CSV vào cấu trúc dữ liệu bộ nhớ khi khởi động.
 *  - Ghi toàn bộ dữ liệu sản phẩm hiện tại ra file CSV khi tắt chương trình.
 *  - Tuyệt đối không thực hiện logic nghiệp vụ hay truy vấn lọc dữ liệu tại tầng này.
 */
class ProductStorage {
public:
    /**
     * @brief Đọc danh sách sản phẩm từ file CSV và nạp vào ProductManager.
     * @param filePath Đường dẫn file CSV.
     * @param manager Tham chiếu tới ProductManager để nạp dữ liệu vào DSA Core.
     * @return Result Mã trạng thái và số lượng bản ghi đã nạp thành công.
     */
    static Result loadFromCSV(const std::string& filePath, ProductManager& manager);

    /**
     * @brief Ghi toàn bộ danh sách sản phẩm từ ProductManager ra file CSV.
     * @param filePath Đường dẫn file CSV đích.
     * @param manager Tham chiếu tới ProductManager để lấy dữ liệu.
     * @return Result Mã trạng thái ghi file.
     */
    static Result saveToCSV(const std::string& filePath, const ProductManager& manager);
};

} // namespace shop
