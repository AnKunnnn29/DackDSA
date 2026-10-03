#ifndef DACKDSA_ORDERMANAGER_CPP
#define DACKDSA_ORDERMANAGER_CPP

#include "ProductLookup.cpp"

namespace shop {
using namespace std;
struct OrderStore {
    dsa::HashTable<Order> records;
    uint64_t revision = 1;
};

class OrderService {
    ProductStore& products_;
    OrderStore& orders_;
    uint64_t nextId_ = 1;
public:
    OrderService(ProductStore& products, OrderStore& orders)
        : products_(products), orders_(orders) {}
    Result createOrder(const CreateOrderRequest& request);
    const Order* findById(const string& id) const;
    vector<Order> listOrders() const;
};
} // namespace shop

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace std;

namespace shop {
namespace {
bool orderManagerBlank(const string& value) {
    return value.empty() || all_of(value.begin(), value.end(),
        [](unsigned char c) { return isspace(c) != 0; });
}
bool orderManagerValidId(const string& id) {
    return !orderManagerBlank(id) && none_of(id.begin(), id.end(),
        [](unsigned char c) { return isspace(c) != 0; });
}
Result orderManagerFailure(ErrorCode code, const string& message) {
    return {code, message, {}};
}
}
Result OrderService::createOrder(const CreateOrderRequest& request) {
    if (orderManagerBlank(request.customerName) || orderManagerBlank(request.customerPhone) || request.items.empty() ||
        request.priority < 1 || request.priority > 3)
        return orderManagerFailure(ErrorCode::INVALID_INPUT, "Khach hang, danh sach hang hoac uu tien khong hop le.");

    // Aggregate duplicate IDs before validating stock. Preserve first appearance.
    dsa::HashTable<size_t> positions;
    vector<RequestedItem> merged;
    const auto max = numeric_limits<int64_t>::max();
    for (const auto& item : request.items) {
        if (!orderManagerValidId(item.productId) || item.quantity <= 0)
            return orderManagerFailure(ErrorCode::INVALID_INPUT, "Ma san pham hoac so luong khong hop le.");
        if (const auto* position = positions.find(item.productId)) {
            auto& quantity = merged[*position].quantity;
            if (item.quantity > max - quantity)
                return orderManagerFailure(ErrorCode::AMOUNT_OVERFLOW, "Tong so luong vuot gioi han.");
            quantity += item.quantity;
        } else {
            positions.insert(item.productId, merged.size());
            merged.push_back(item);
        }
    }
    Order order;
    order.customerName = request.customerName;
    order.customerPhone = request.customerPhone;
    order.priority = request.priority;
    vector<Product*> affected;
    order.items.reserve(merged.size());
    affected.reserve(merged.size());
    for (const auto& item : merged) {
        auto* product = products_.records.find(item.productId);
        if (!product)
            return orderManagerFailure(ErrorCode::PRODUCT_NOT_FOUND, "Khong tim thay san pham: " + item.productId);
        if (product->stock < item.quantity)
            return orderManagerFailure(ErrorCode::INSUFFICIENT_STOCK, "Khong du ton kho: " + item.productId);
        if (product->price != 0 && item.quantity > max / product->price)
            return orderManagerFailure(ErrorCode::AMOUNT_OVERFLOW, "Thanh tien vuot gioi han.");
        const Money subtotal = product->price * item.quantity;
        if (subtotal > max - order.totalAmount)
            return orderManagerFailure(ErrorCode::AMOUNT_OVERFLOW, "Tong tien vuot gioi han.");
        order.totalAmount += subtotal;
        order.items.push_back({product->productId, product->name, item.quantity, product->price});
        affected.push_back(product);
    }
    // Skip IDs already present (e.g. when integrating a persistence loader).
    do {
        ostringstream id;
        id << "ORD" << setw(6) << setfill('0') << nextId_++;
        order.orderId = id.str();
    } while (orders_.records.find(order.orderId));
    order.createdAt = chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()).count();
    order.historyHead = make_shared<StatusHistoryNode>(StatusHistoryNode{
        OrderStatus::PENDING, order.createdAt, nullptr});
    const auto id = order.orderId;
    // All allocations, including the response, precede stock changes.
    Result success{ErrorCode::NONE, "Tao don hang thanh cong.", id};
    if (!orders_.records.insert(id, move(order)))
        return orderManagerFailure(ErrorCode::DUPLICATE_ID, "Ma don hang da ton tai.");
    for (size_t i = 0; i < merged.size(); ++i)
        affected[i]->stock -= merged[i].quantity;
    ++products_.revision;
    ++orders_.revision;
    return success;
}
const Order* OrderService::findById(const string& id) const {
    return orderManagerValidId(id) ? orders_.records.find(id) : nullptr;
}
vector<Order> OrderService::listOrders() const {
    vector<Order> orders;
    orders.reserve(orders_.records.size());
    orders_.records.forEach([&](const auto&, const Order& o) { orders.push_back(o); });
    sort(orders.begin(), orders.end(), [](const Order& a, const Order& b) {
        if (a.createdAt != b.createdAt) return a.createdAt > b.createdAt;
        // IDs have a numeric suffix that can grow beyond six digits.
        if (a.orderId.size() != b.orderId.size()) return a.orderId.size() > b.orderId.size();
        return a.orderId > b.orderId;
    });
    return orders;
}
} // namespace shop

#endif // DACKDSA_ORDERMANAGER_CPP
