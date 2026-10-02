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

// Chuc nang: Tim kiem san pham theo khoang gia
// Tim vi tri dau tien co price >= minPrice
int timViTriDauTienTrongKhoang(const vector<Product>& dsSanPham, int quantity, Money minPrice) {
    int left = 0;
    int right = quantity - 1;
    int index = -1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (dsSanPham[mid].price >= minPrice) {
            index = mid;     // Ghi nhan vi tri thoa man
            right = mid - 1; // Tim tiep nua ben trai
        } else {
            left = mid + 1;
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
            index = mid;    // Ghi nhận vị trí thỏa mãn
            left = mid + 1; // Tìm tiếp ở nửa bên phải
        } else {
            right = mid - 1;
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
        cout << "Ma SP  : " << dsSanPham[index].productId << " | ";
        cout << "Ten SP : " << dsSanPham[index].name << " | "; 
        cout << "Gia    : " << dsSanPham[index].price << " VNĐ | ";
        cout << "Ton kho: " << dsSanPham[index].stock << endl;
    }
}

int main() {
    // Dữ liệu sản phẩm mẫu chuẩn kiểu int64_t
    vector<Product> dsSanPham = {
        {"P001", "Chuot khong day", 300000, 20, 5},
        {"P002", "Tai nghe Gaming", 550000, 15, 3},
        {"P003", "Ban phim co", 750000, 10, 2},
        {"P004", "Man hinh 24 inch", 2500000, 8, 2}
    };
    
    int quantity = dsSanPham.size(); // Số lượng sản phẩm

    cout << "========== MENU QUAN LY SAN PHAM ==========" << endl;
    cout << "1. Tra cuu san pham theo ma" << endl;
    cout << "2. Tim kiem san pham theo khoang gia" << endl;
    cout << "Lua chon cua ban (1 hoac 2): ";
    
    int luaChon;
    cin >> luaChon;

    if (luaChon == 1) {
        string target;
        cout << "\nNhap ma san pham can tim (VD: P001, P002...): ";
        cin >> target;

        int index = timKiemTheoMaSanPham(dsSanPham, quantity, target);

        if (index != -1) {
            cout << "\n--- THONG TIN SAN PHAM TIM THAY ---" << endl;
            cout << "Ma san pham : " << dsSanPham[index].productId << endl;
            cout << "Ten san pham: " << dsSanPham[index].name << endl;
            cout << "Gia ban     : " << dsSanPham[index].price << " VNĐ" << endl;
            cout << "So luong ton: " << dsSanPham[index].stock << endl;
        } else {
            cout << "\nKhong tim thay san pham co ma: " << target << " !" << endl;
        }

    } else if (luaChon == 2) {
        Money giaTu, giaDen;
        cout << "\nNhap gia thap nhat: ";
        cin >> giaTu;
        cout << "Nhap gia cao nhat: ";
        cin >> giaDen;

        timKiemTheoKhoangGia(dsSanPham, quantity, giaTu, giaDen);

    } else {
        cout << "\nLua chon khong hop le!!!" << endl;
    }

    return 0;
}