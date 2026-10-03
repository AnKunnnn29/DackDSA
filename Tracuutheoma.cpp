#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

using Money = int64_t;
using Quantity = int64_t;

struct Product {
    string productId;
    string name;
    Money price = 0;
    Quantity stock = 0;
    Quantity minStock = 0;
};

// Chuc nang: Tra cuu san pham theo ma
// Yeu cau: Vector dsSanPham da duoc sap xep tang dan theo productId
int timKiemTheoMaSanPham(const vector<Product>& dsSanPham, int quantity, string target) {
    int left = 0;
    int right = quantity - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (dsSanPham[mid].productId == target) {
            return mid; // Tra ve chi so vi tri tim thay
        }

        if (dsSanPham[mid].productId < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return -1; // Tra ve -1 neu khong tim thay
}