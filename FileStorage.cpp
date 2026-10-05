#ifndef DACKDSA_FILESTORAGE_CPP
#define DACKDSA_FILESTORAGE_CPP
#include "ProductManagement.cpp"
#include "OrderStatus.cpp"
#include <charconv>
#include <filesystem>
#include <fstream>

namespace shop {
namespace csv {
using namespace std;
inline bool read(istream& input, vector<vector<string>>& rows) {
    vector<string> row;
    string field;
    bool quoted = false, closed = false, started = false;
    char c;
    while (input.get(c)) {
        started = true;
        if (quoted) {
            if (c == '"') {
                if (input.peek() == '"') { input.get(c); field += '"'; }
                else { quoted = false; closed = true; }
            } else field += c;
            continue;
        }
        if (c == ',' || c == '\n') {
            row.push_back(move(field)); field.clear(); closed = false;
            if (c == '\n') { rows.push_back(move(row)); row.clear(); started = false; }
        } else if (c == '\r' && input.peek() == '\n') {
            continue;
        } else if (c == '"' && field.empty() && !closed) quoted = true;
        else {
            if (closed || c == '"') return false;
            field += c;
        }
    }
    if (quoted || input.bad()) return false;
    if (started) { row.push_back(move(field)); rows.push_back(move(row)); }
    return true;
}
inline void write(ostream& output, const vector<string>& fields) {
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i) output << ',';
        output << '"';
        for (char c : fields[i]) { if (c == '"') output << '"'; output << c; }
        output << '"';
    }
    output << '\n';
}
inline bool integer(const string& text, int64_t& value) {
    if (text.empty()) return false;
    const auto parsed = from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == errc{} && parsed.ptr == text.data() + text.size();
}
inline bool status(const string& text, OrderStatus& result) {
    for (auto candidate : {OrderStatus::PENDING, OrderStatus::PROCESSING, OrderStatus::SHIPPING,
                           OrderStatus::COMPLETED, OrderStatus::CANCELLED}) {
        if (text == statusToString(candidate)) { result = candidate; return true; }
    }
    return false;
}
}
class FileStorage {
    ProductStore& products_;
    OrderStore& orders_;
    static Result invalid() { return {ErrorCode::INVALID_DATA, "File CSV khong hop le; du lieu hien tai duoc giu nguyen.", {}}; }
public:
    FileStorage(ProductStore& products, OrderStore& orders) : products_(products), orders_(orders) {}
    Result save(const string& filename) const {
        namespace fs = filesystem;
        const fs::path target(filename), temporary(filename + ".tmp"), backup(filename + ".bak");
        error_code error;
        if (fs::exists(temporary, error) || error || fs::exists(backup, error) || error)
            return {ErrorCode::IO_ERROR, "File .tmp/.bak da ton tai; kiem tra truoc khi luu lai.", {}};
        ofstream output(temporary, ios::binary);
        if (!output) return {ErrorCode::IO_ERROR, "Khong mo duoc file de luu.", {}};
        csv::write(output, {"VERSION", "2"});
        products_.records.forEach([&](const auto&, const Product& p) {
            csv::write(output, {"PRODUCT", p.productId, p.name, to_string(p.price), to_string(p.stock), to_string(p.minStock), p.category, p.brand});
        });
        orders_.records.forEach([&](const auto&, const Order& o) {
            csv::write(output, {"ORDER", o.orderId, o.customerName, o.customerPhone, to_string(o.totalAmount),
                to_string(o.createdAt), to_string(o.priority), statusToString(o.status)});
            for (const auto& item : o.items)
                csv::write(output, {"ITEM", o.orderId, item.productId, item.productName, to_string(item.quantity), to_string(item.unitPrice)});
            vector<const StatusHistoryNode*> history;
            for (auto node = o.historyHead; node; node = node->next) history.push_back(node.get());
            for (auto it = history.rbegin(); it != history.rend(); ++it)
                csv::write(output, {"HISTORY", o.orderId, statusToString((*it)->status), to_string((*it)->changedAt)});
        });
        output.flush();
        bool written = output.good();
        output.close(); written = written && !output.fail();
        if (!written) {
            fs::remove(temporary, error);
            return {ErrorCode::IO_ERROR, "Ghi file that bai; file cu duoc giu nguyen.", {}};
        }
        const bool hadFile = fs::exists(target, error);
        if (error) { fs::remove(temporary, error); return {ErrorCode::IO_ERROR, "Khong kiem tra duoc file dich.", {}}; }
        if (hadFile) {
            fs::rename(target, backup, error);
            if (error) { fs::remove(temporary, error); return {ErrorCode::IO_ERROR, "Khong tao duoc backup.", {}}; }
        }
        fs::rename(temporary, target, error);
        if (error) {
            if (hadFile) {
                error_code restoreError;
                fs::rename(backup, target, restoreError);
                if (restoreError) return {ErrorCode::IO_ERROR, "Luu that bai; file cu nam trong .bak, can khoi phuc.", {}};
            }
            fs::remove(temporary, error);
            return {ErrorCode::IO_ERROR, "Khong hoan tat luu file; file cu duoc giu nguyen.", {}};
        }
        if (hadFile) {
            fs::remove(backup, error);
            if (error) return {ErrorCode::NONE, "Da luu; con file .bak can don truoc lan luu tiep.", {}};
        }
        return {ErrorCode::NONE, "Da luu CSV.", {}};
    }
    Result load(const string& filename) {
        ifstream input(filename, ios::binary);
        if (!input) return {ErrorCode::IO_ERROR, "Khong mo duoc file CSV.", {}};
        vector<vector<string>> rows;
        if (!csv::read(input, rows) || rows.empty()) return invalid();
        const bool legacy = rows.front() == vector<string>{"VERSION", "1"};
        if (!legacy && rows.front() != vector<string>{"VERSION", "2"}) return invalid();
        ProductStore loadedProducts;
        OrderStore loadedOrders;
        ProductService loader(loadedProducts);
        for (size_t line = 1; line < rows.size(); ++line) {
            const auto& r = rows[line];
            if (r.empty()) return invalid();
            if (r[0] == "PRODUCT") {
                Product p;
                if (r.size() != (legacy ? 6u : 8u) || !csv::integer(r[3], p.price) || !csv::integer(r[4], p.stock) ||
                    !csv::integer(r[5], p.minStock)) return invalid();
                p.productId = r[1]; p.name = r[2];
                if (legacy) {
                    // Classify the existing demo catalog once while loading the old format.
                    const pair<const char*, const char*> types[] = {
                        {"Gia do laptop", "Giá đỡ laptop"}, {"Laptop", "Laptop"}, {"Chuot", "Chuột"},
                        {"Ban phim", "Bàn phím"}, {"Man hinh", "Màn hình"}, {"Tai nghe", "Tai nghe"},
                        {"SSD", "Ổ cứng SSD"}, {"O cung HDD", "Ổ cứng HDD"}, {"RAM", "RAM"},
                        {"Webcam", "Webcam"}, {"Loa", "Loa"}, {"Sac du phong", "Sạc dự phòng"},
                        {"Cap USB", "Cáp kết nối"}, {"Router", "Router WiFi"}, {"USB", "USB"},
                        {"Microphone", "Microphone"}, {"May in", "Máy in"}, {"Bo chia HDMI", "Bộ chia HDMI"}};
                    for (const auto& type : types)
                        if (p.name.rfind(type.first, 0) == 0) { p.category = type.second; break; }
                    const char* brands[] = {"Logitech", "Keychron", "Corsair", "Razer", "Dell", "Sony", "Samsung",
                        "Kingston", "Asus", "JBL", "Anker", "Ugreen", "TP-Link", "Sandisk", "Baseus", "Seagate", "HyperX", "Canon"};
                    for (const auto* brand : brands)
                        if (p.name.find(brand) != string::npos) { p.brand = brand; break; }
                } else { p.category = r[6]; p.brand = r[7]; }
                if (!loader.addProduct(move(p)).ok()) return invalid();
            } else if (r[0] == "ORDER") {
                Order o; int64_t priority;
                if (r.size() != 8 || !orderManagerValidId(r[1]) || productLookupBlank(r[2]) || productLookupBlank(r[3]) ||
                    !csv::integer(r[4], o.totalAmount) || o.totalAmount < 0 || !csv::integer(r[5], o.createdAt) || o.createdAt <= 0 ||
                    !csv::integer(r[6], priority) || priority < 1 || priority > 3 || !csv::status(r[7], o.status)) return invalid();
                if (r[1].size() < 4 || r[1].substr(0,3) != "ORD" ||
                    !all_of(r[1].begin() + 3, r[1].end(), [](unsigned char c) { return c >= '0' && c <= '9'; })) return invalid();
                o.orderId = r[1]; o.customerName = r[2]; o.customerPhone = r[3]; o.priority = static_cast<int>(priority);
                if (!loadedOrders.records.insert(r[1], move(o))) return invalid();
            } else if (r[0] == "ITEM") {
                OrderItem item;
                if (r.size() != 6 || !orderManagerValidId(r[2]) || productLookupBlank(r[3]) ||
                    !csv::integer(r[4], item.quantity) || item.quantity <= 0 || !csv::integer(r[5], item.unitPrice) || item.unitPrice < 0) return invalid();
                auto* order = loadedOrders.records.find(r[1]);
                if (!order) return invalid();
                item.productId = r[2]; item.productName = r[3]; order->items.push_back(move(item));
            } else if (r[0] == "HISTORY") {
                OrderStatus status; int64_t time;
                if (r.size() != 4 || !csv::status(r[2], status) || !csv::integer(r[3], time)) return invalid();
                auto* order = loadedOrders.records.find(r[1]);
                if (!order) return invalid();
                if (order->historyHead) {
                    if (!isValidTransition(order->historyHead->status, status) || time < order->historyHead->changedAt) return invalid();
                } else if (status != OrderStatus::PENDING || time != order->createdAt) return invalid();
                order->historyHead = make_shared<StatusHistoryNode>(StatusHistoryNode{status, time, order->historyHead});
            } else return invalid();
        }
        bool valid = true;
        loadedOrders.records.forEach([&](const auto&, const Order& order) {
            if (order.items.empty() || !order.historyHead || order.historyHead->status != order.status) { valid = false; return; }
            Money total = 0;
            dsa::HashTable<int> ids;
            for (const auto& item : order.items) {
                if (!loadedProducts.records.find(item.productId) || !ids.insert(item.productId, 1) ||
                    (item.unitPrice && item.quantity > numeric_limits<Money>::max() / item.unitPrice)) { valid = false; return; }
                const Money subtotal = item.quantity * item.unitPrice;
                if (subtotal > numeric_limits<Money>::max() - total) { valid = false; return; }
                total += subtotal;
            }
            if (total != order.totalAmount) valid = false;
        });
        if (!valid) return invalid();
        // Commit the validated snapshot without creating orders or changing stock twice.
        products_.records.swap(loadedProducts.records);
        orders_.records.swap(loadedOrders.records);
        ++products_.revision; ++orders_.revision;
        return {ErrorCode::NONE, "Da nap CSV.", {}};
    }
};
} // namespace shop
#endif
