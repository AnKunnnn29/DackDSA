#include "FileStorage.cpp"
#include "OrderLookup.cpp"
#include "PriceSearch.cpp"
#include "PriorityOrders.cpp"
#include "LowStock.cpp"
#include <iostream>

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
        if (input >> value) { input >> ws; if (input.eof()) return value; }
        cout << "Vui long nhap so nguyen hop le.\n";
    }
}
void display(const shop::Product& p) {
    cout << p.productId << " | " << p.name << " | " << p.price << " VND | Ton: "
         << p.stock << " | Nguong: " << p.minStock << '\n';
}
void display(const shop::Order& o) {
    cout << o.orderId << " | " << o.customerName << " | " << o.customerPhone << " | "
         << shop::statusToString(o.status) << " | Uu tien: " << o.priority << '\n';
    for (const auto& item : o.items)
        cout << "  " << item.productId << " | " << item.productName << " | " << item.quantity
             << " x " << item.unitPrice << " = " << item.subtotal() << " VND\n";
    cout << "Tong: " << o.totalAmount << " VND | Unix milliseconds: " << o.createdAt << '\n';
}
void showResult(const shop::Result& result) {
    cout << (result.ok() ? "OK: " : "LOI: ")
         << (result.message.empty() ? "Thao tac thanh cong." : result.message) << '\n';
}
}
int main(int argc, char** argv) {
    string filename = "data/shop.csv";
    bool demo = false;
    for (int i = 1; i < argc; ++i) {
        const string arg = argv[i];
        if (arg == "--data" && i + 1 < argc) filename = argv[++i];
        else if (arg == "--demo") demo = true;
        else {
            cout << "Usage: dsa_demo [--data file.csv] [--demo]\n";
            return arg == "--help" ? 0 : 1;
        }
    }
    shop::ProductStore products;
    shop::OrderStore orders;
    shop::ProductService catalog(products);
    shop::ProductManagementService management(products, orders);
    shop::OrderService orderService(products, orders);
    shop::OrderLookupService orderLookup(orders);
    shop::PriceSearchService priceSearch(products);
    shop::OrderStatusService status(products, orders);
    shop::PriorityOrderService priority(orders, status);
    shop::LowStockService lowStock(products);
    shop::FileStorage storage(products, orders);
    error_code error;
    const auto parent = filesystem::path(filename).parent_path();
    if (!parent.empty()) filesystem::create_directories(parent, error);
    if (error) { cerr << "Khong tao duoc thu muc du lieu: " << error.message() << '\n'; return 1; }
    const bool exists = filesystem::exists(filename, error);
    if (error) { cerr << "Khong kiem tra duoc file du lieu.\n"; return 1; }
    if (exists) {
        const auto loaded = storage.load(filename);
        if (!loaded.ok()) { showResult(loaded); return 1; } // never overwrite a malformed snapshot
    }
    bool dirty = false;
    if (!exists && demo) {
        catalog.addProduct({"P001", "Chuot khong day", 500000, 20, 5});
        catalog.addProduct({"P002", "Ban phim", 750000, 10, 3});
        catalog.addProduct({"P003", "Man hinh", 3000000, 5, 2});
        dirty = true;
    }
    auto save = [&] {
        const auto result = storage.save(filename);
        if (result.ok()) dirty = false;
        else showResult(result);
        return result.ok();
    };
    auto changed = [&](const shop::Result& result) {
        showResult(result);
        if (result.ok()) { dirty = true; save(); }
    };
    cout << "DSA - He thong quan ly kho va don hang\nFile: " << filename
         << "\nTu dong luu sau thao tac thay doi. Uu tien: 1 cao, 2 binh thuong, 3 thap.\n";
    try {
        for (;;) {
            cout << "\n1. Danh sach san pham     2. Tra cuu Product ID\n"
                    "3. Them san pham         4. Sua thong tin san pham\n"
                    "5. Nhap/xuat kho         6. Xoa san pham\n"
                    "7. Tao don hang          8. Danh sach don hang\n"
                    "9. Tra cuu Order ID     10. Tim theo khoang gia\n"
                    "11. Xu ly don uu tien   12. Chuyen trang thai/huy don\n"
                    "13. Lich su trang thai  14. Canh bao ton kho\n"
                    "15. Luu CSV              0. Thoat\n";
            const auto choice = number("Chon: ");
            if (choice == 0) break;
            if (choice == 1) {
                for (const auto& p : catalog.listProducts()) display(p);
            } else if (choice == 2) {
                if (const auto* p = catalog.findById(read("Product ID: "))) display(*p);
                else cout << "Khong tim thay san pham.\n";
            } else if (choice == 3) {
                shop::Product p;
                p.productId = read("Product ID: "); p.name = read("Ten: ");
                p.price = number("Gia VND: "); p.stock = number("Ton kho: "); p.minStock = number("Nguong canh bao: ");
                changed(management.addProduct(move(p)));
            } else if (choice == 4) {
                const auto id = read("Product ID: "), name = read("Ten moi: ");
                const auto price = number("Gia moi: "), minStock = number("Nguong moi: ");
                changed(management.updateInfo(id, name, price, minStock));
            } else if (choice == 5) {
                const auto id = read("Product ID: ");
                const auto direction = number("1 nhap, 2 xuat: "), quantity = number("So luong: ");
                if (direction != 1 && direction != 2) { cout << "Lua chon khong hop le.\n"; continue; }
                changed(management.changeStock(id, quantity, direction == 1));
            } else if (choice == 6) {
                changed(management.removeProduct(read("Product ID: ")));
            } else if (choice == 7) {
                shop::CreateOrderRequest request;
                request.customerName = read("Khach hang: "); request.customerPhone = read("So dien thoai: ");
                const auto p = number("Uu tien 1/2/3: ");
                request.priority = p >= 1 && p <= 3 ? static_cast<int>(p) : 0;
                const auto count = number("So dong san pham 1-1000: ");
                if (count < 1 || count > 1000) { cout << "So dong khong hop le.\n"; continue; }
                for (int64_t i = 0; i < count; ++i) {
                    const auto id = read("Product ID: ");
                    request.items.push_back({id, number("So luong: ")});
                }
                const auto result = orderService.createOrder(request);
                changed(result);
                if (result.ok()) display(*orderLookup.findById(result.orderId));
            } else if (choice == 8) {
                for (const auto& o : orderService.listOrders()) display(o);
            } else if (choice == 9) {
                if (const auto* o = orderLookup.findById(read("Order ID: "))) display(*o);
                else cout << "Khong tim thay don hang.\n";
            } else if (choice == 10) {
                const auto min = number("Gia thap nhat: "), max = number("Gia cao nhat: ");
                vector<shop::Product> found;
                const auto result = priceSearch.findInRange(min, max, found);
                if (!result.ok()) showResult(result);
                else if (found.empty()) cout << "Khong co san pham trong khoang.\n";
                for (const auto& p : found) display(p);
            } else if (choice == 11) {
                const auto result = priority.processNext(); changed(result);
                if (result.ok()) display(*orderLookup.findById(result.orderId));
            } else if (choice == 12) {
                const auto id = read("Order ID: ");
                cout << "1 PENDING, 2 PROCESSING, 3 SHIPPING, 4 COMPLETED, 5 CANCELLED\n";
                const auto target = number("Trang thai: ");
                if (target < 1 || target > 5) { cout << "Trang thai khong hop le.\n"; continue; }
                changed(status.updateStatus(id, static_cast<shop::OrderStatus>(target - 1)));
            } else if (choice == 13) {
                const auto id = read("Order ID: ");
                if (!orderLookup.findById(id)) { cout << "Khong tim thay don hang.\n"; continue; }
                for (const auto& entry : status.history(id))
                    cout << shop::statusToString(entry.first) << " | " << entry.second << '\n';
            } else if (choice == 14) {
                const auto warnings = lowStock.warnings();
                if (warnings.empty()) cout << "Khong co canh bao.\n";
                for (const auto& p : warnings) { cout << (p.stock == 0 ? "HET HANG: " : "SAP HET: "); display(p); }
            } else if (choice == 15) { if (save()) cout << "Da luu.\n"; }
            else cout << "Lua chon khong hop le.\n";
        }
    } catch (const runtime_error&) {
        if (!cin.eof()) throw;
    }
    if (dirty && !save()) { cerr << "Du lieu trong bo nho chua luu duoc.\n"; return 1; }
}
