#include "OrderManager.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace {
string read(const char* prompt) {
    cout << prompt;
    string value;
    if (!getline(cin, value)) throw runtime_error("EOF");
    return value;
}
int64_t number(const char* prompt) {
    for (;;) {
        istringstream input(read(prompt));
        int64_t value;
        if (input >> value) {
            input >> ws;
            if (input.eof()) return value;
        }
        cout << "Vui long nhap so nguyen hop le.\n";
    }
}
void display(const shop::Product& p) {
    cout << p.productId << " | " << p.name << " | " << p.price
              << " VND | Ton kho: " << p.stock << '\n';
}
void display(const shop::Order& o) {
    cout << "\n" << o.orderId << " | " << o.customerName << " | " << o.customerPhone
              << " | " << shop::statusToString(o.status) << " | Uu tien: " << o.priority << '\n';
    for (const auto& item : o.items)
        cout << "  " << item.productId << " | " << item.productName << " | "
                  << item.quantity << " x " << item.unitPrice << " = " << item.subtotal() << " VND\n";
    cout << "Tong: " << o.totalAmount << " VND\n";
}
}
int main() {
    shop::ProductStore products;
    shop::OrderStore orders;
    shop::ProductService catalog(products);
    shop::OrderService service(products, orders);
    catalog.addProduct({"P001", "Chuot khong day", 500000, 20, 5});
    catalog.addProduct({"P002", "Ban phim", 750000, 10, 3});
    catalog.addProduct({"P003", "Man hinh", 3000000, 5, 2});
    cout << "DSA - Tra cuu san pham va quan ly don hang\n"
                 "Du lieu demo nam trong bo nho; thoat chuong trinh se mat du lieu.\n";
    try {
        for (;;) {
            cout << "\n1. Danh sach san pham\n2. Tra cuu theo ma\n3. Tao don hang\n"
                         "4. Danh sach don hang\n5. Chi tiet don hang\n0. Thoat\n";
            const auto choice = number("Chon: ");
            if (choice == 0) break;
            if (choice == 1) {
                for (const auto& p : catalog.listProducts()) display(p);
            } else if (choice == 2) {
                if (const auto* p = catalog.findById(read("Ma san pham: "))) display(*p);
                else cout << "Khong tim thay san pham.\n";
            } else if (choice == 3) {
                shop::CreateOrderRequest request;
                request.customerName = read("Ten khach hang: ");
                request.customerPhone = read("So dien thoai: ");
                const auto priority = number("Uu tien (1/2/3): ");
                request.priority = priority >= 1 && priority <= 3 ? static_cast<int>(priority) : 0;
                const auto count = number("So dong san pham (1-1000): ");
                if (count <= 0 || count > 1000) {
                    cout << "So dong khong hop le.\n";
                    continue;
                }
                for (int64_t i = 0; i < count; ++i) {
                    auto id = read("Ma san pham: ");
                    auto quantity = number("So luong: ");
                    request.items.push_back({id, quantity});
                }
                const auto result = service.createOrder(request);
                cout << result.message << '\n';
                if (result.ok()) display(*service.findById(result.orderId));
            } else if (choice == 4) {
                const auto list = service.listOrders();
                if (list.empty()) cout << "Chua co don hang.\n";
                for (const auto& o : list) display(o);
            } else if (choice == 5) {
                if (const auto* o = service.findById(read("Ma don hang: "))) display(*o);
                else cout << "Khong tim thay don hang.\n";
            } else cout << "Lua chon khong hop le.\n";
        }
    } catch (const runtime_error&) {
        if (!cin.eof()) throw;
    }
}
