#include "ProductStorage.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace shop {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

Result ProductStorage::loadFromCSV(const std::string& filePath, ProductManager& manager) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return {ErrorCode::INVALID_INPUT, "Khong the mo file de doc: " + filePath, ""};
    }

    std::string line;
    // Bỏ qua dòng tiêu đề (Header: productId,name,price,stock,minStock)
    if (!std::getline(file, line)) {
        return {ErrorCode::NONE, "File rong, khong co du lieu", ""};
    }

    size_t loadedCount = 0;
    size_t errorCount = 0;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string pId, pName, priceStr, stockStr, minStockStr;

        if (std::getline(ss, pId, ',') &&
            std::getline(ss, pName, ',') &&
            std::getline(ss, priceStr, ',') &&
            std::getline(ss, stockStr, ',') &&
            std::getline(ss, minStockStr, ',')) {

            pId = trim(pId);
            pName = trim(pName);
            priceStr = trim(priceStr);
            stockStr = trim(stockStr);
            minStockStr = trim(minStockStr);

            try {
                Money price = std::stoll(priceStr);
                Quantity stock = std::stoll(stockStr);
                Quantity minStock = std::stoll(minStockStr);

                Product p{pId, pName, price, stock, minStock};
                Result res = manager.addProduct(p);
                if (res.ok()) {
                    loadedCount++;
                } else {
                    errorCount++;
                }
            } catch (...) {
                errorCount++;
            }
        }
    }

    file.close();
    return {ErrorCode::NONE, "Nap du lieu thanh cong: " + std::to_string(loadedCount) + 
                             " san pham (Bo qua: " + std::to_string(errorCount) + ")", ""};
}

Result ProductStorage::saveToCSV(const std::string& filePath, const ProductManager& manager) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return {ErrorCode::INVALID_INPUT, "Khong the tao hoac ghi file: " + filePath, ""};
    }

    // Ghi tiêu đề CSV
    file << "productId,name,price,stock,minStock\n";

    std::vector<Product> products = manager.getAllProducts();
    for (const auto& p : products) {
        file << p.productId << ","
             << p.name << ","
             << p.price << ","
             << p.stock << ","
             << p.minStock << "\n";
    }

    file.close();
    return {ErrorCode::NONE, "Ghi file thanh cong: " + std::to_string(products.size()) + " san pham", ""};
}

} // namespace shop
