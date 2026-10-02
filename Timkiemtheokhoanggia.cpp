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

// Chuc nang: Tim kiem san pham theo khoang gia
// Tim vi tri dau tien co price >= minPrice
int timViTriDauTienTrongKhoang(const vector<Product>& dsSanPham, int quantity, Money minPrice) {
    int left = 0;
    int right = quantity - 1;
    int index = -1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (dsSanPham[mid].price >= minPrice) {
            index = mid;      // Ghi nhan vi tri hop le
            right = mid - 1;  // Thu hep tim phia ben trai xem co phan tu nao nho hon ma van >= minPrice khong
        } else {
            left = mid + 1;   // Neu price < minPrice thi phai tim sang ben phai
        }
    }
    return index;
}

// Tim vi tri cuoi cung co price <= maxPrice
int timViTriCuoiCungTrongKhoang(const vector<Product>& dsSanPham, int quantity, Money maxPrice) {
    int left = 0;
    int right = quantity - 1;
    int index = -1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (dsSanPham[mid].price <= maxPrice) {
            index = mid;     // Ghi nhan vi tri hop le
            left = mid + 1;  // Thu hep tim phia ben phai xem co phan tu nao lon hon ma van <= maxPrice khong
        } else {
            right = mid - 1; // Neu price > maxPrice thi phai tim sang ben trai
        }
    }
    return index;
}

void timKiemTheoKhoangGia(const vector<Product>& dsSanPham, int quantity, Money minPrice, Money maxPrice) {
    int leftBound = timViTriDauTienTrongKhoang(dsSanPham, quantity, minPrice);
    int rightBound = timViTriCuoiCungTrongKhoang(dsSanPham, quantity, maxPrice);

    if (leftBound == -1 || rightBound == -1 || leftBound > rightBound) {
        cout << "\nKhong tim thay san pham nao trong khoang gia nay!" << endl;
        return;
    }

    cout << "\n=== DANH SACH SAN PHAM TRONG KHOANG GIA ===" << endl;
    for (int index = leftBound; index <= rightBound; index++) {
        cout << "Ma SP  : " << dsSanPham[index].productId << endl;
        cout << "Ten SP : " << dsSanPham[index].name << endl; 
        cout << "Gia    : " << dsSanPham[index].price << " VNĐ" << endl;
        cout << "Ton kho: " << dsSanPham[index].stock << endl;
    }
}