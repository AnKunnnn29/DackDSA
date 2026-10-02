#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

using namespace std;

// Struct thống nhất theo đề bài
struct Product {
    string productId;
    string name;
    double price;
    int stock;
    int minStock;
};

class LowStockNotifier {
public:
    // Hàm nhận vào danh sách sản phẩm và tổng số lượng sản phẩm ban đầu/định mức (totalStock)
    void checkAndWarnLowStock(const vector<Product>& products, int totalStock) const {
        if (totalStock <= 0) return;

        cout << "\n================ CẢNH BÁO SẢN PHẨM SẮP HẾT HÀNG (< 15%) ================\n";
        bool hasWarning = false;

        for (const auto& prod : products) {
            // Kiểm tra nếu lượng tồn kho hiện tại < 15% tổng số lượng
            if ((double)prod.stock / totalStock < 0.15) {
                hasWarning = true;
                double percentage = ((double)prod.stock / totalStock) * 100.0;
                
                cout << "[CẢNH BÁO] Mã SP: " << prod.productId 
                     << " | Tên: " << prod.name 
                     << " | Tồn kho: " << prod.stock << "/" << totalStock 
                     << " (" << percentage << "%)";

                if (prod.stock == 0) {
                    cout << " -> [ĐÃ HẾT HÀNG!]\n";
                } else {
                    cout << " -> [SẮP HẾT HÀNG - CẦN NHẬP THÊM!]\n";
                }
            }
        }

        if (!hasWarning) {
            cout << "Tất cả sản phẩm đều đang ở mức an toàn (>= 15%).\n";
        }
        cout << "========================================================================\n";
    }
};