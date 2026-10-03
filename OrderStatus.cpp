#ifndef DACKDSA_ORDERSTATUS_CPP
#define DACKDSA_ORDERSTATUS_CPP
#include "OrderManager.cpp"

namespace shop {
using namespace std;
class OrderStatusService {
    ProductStore& products_;
    OrderStore& orders_;
public:
    OrderStatusService(ProductStore& products, OrderStore& orders) : products_(products), orders_(orders) {}
    Result updateStatus(const string& id, OrderStatus target) {
        auto* order = orders_.records.find(id);
        if (!order) return {ErrorCode::ORDER_NOT_FOUND, "Khong tim thay don hang.", {}};
        if (!isValidTransition(order->status, target))
            return {ErrorCode::INVALID_TRANSITION, "Khong duoc chuyen sang trang thai nay.", {}};
        vector<Product*> restore;
        if (target == OrderStatus::CANCELLED) {
            restore.reserve(order->items.size());
            for (const auto& item : order->items) {
                auto* product = products_.records.find(item.productId);
                if (!product) return {ErrorCode::PRODUCT_NOT_FOUND, "Khong the hoan kho: san pham khong ton tai.", {}};
                if (item.quantity > numeric_limits<Quantity>::max() - product->stock)
                    return {ErrorCode::AMOUNT_OVERFLOW, "Hoan kho vuot gioi han.", {}};
                restore.push_back(product);
            }
        }
        // Allocate immutable history before applying any state/stock changes.
        const auto changedAt = max(nowMilliseconds(), order->historyHead ? order->historyHead->changedAt : order->createdAt);
        auto history = make_shared<StatusHistoryNode>(StatusHistoryNode{target, changedAt, order->historyHead});
        for (size_t i = 0; i < restore.size(); ++i) restore[i]->stock += order->items[i].quantity;
        order->status = target;
        order->historyHead = move(history);
        ++orders_.revision;
        if (!restore.empty()) ++products_.revision;
        return {};
    }
    vector<pair<OrderStatus, int64_t>> history(const string& id) const {
        vector<pair<OrderStatus, int64_t>> result;
        const auto* order = orders_.records.find(id);
        if (order)
            for (auto node = order->historyHead; node; node = node->next)
                result.emplace_back(node->status, node->changedAt);
        reverse(result.begin(), result.end()); // oldest -> newest for display
        return result;
    }
};
} // namespace shop
#endif
