#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace shop {
using namespace std;
using Money = int64_t; // VND, no floating-point rounding
using Quantity = int64_t;
enum class OrderStatus { PENDING, PROCESSING, SHIPPING, COMPLETED, CANCELLED };
inline int64_t nowMilliseconds() {
    return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}
inline bool isValidTransition(OrderStatus from, OrderStatus to) {
    return (from == OrderStatus::PENDING && (to == OrderStatus::PROCESSING || to == OrderStatus::CANCELLED)) ||
           (from == OrderStatus::PROCESSING && (to == OrderStatus::SHIPPING || to == OrderStatus::CANCELLED)) ||
           (from == OrderStatus::SHIPPING && to == OrderStatus::COMPLETED);
}
// Immutable linked history: copies of Order safely share older history nodes.
struct StatusHistoryNode {
    OrderStatus status;
    int64_t changedAt;
    shared_ptr<const StatusHistoryNode> next;
};
inline const char* statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::PROCESSING: return "PROCESSING";
        case OrderStatus::SHIPPING: return "SHIPPING";
        case OrderStatus::COMPLETED: return "COMPLETED";
        case OrderStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}
struct Product {
    string productId;
    string name;
    Money price = 0;
    Quantity stock = 0;
    Quantity minStock = 0;
};
struct OrderItem {
    string productId;
    string productName;
    Quantity quantity = 0;
    Money unitPrice = 0;
    Money subtotal() const { return quantity * unitPrice; } // validated by creation
};
struct Order {
    string orderId;
    string customerName;
    string customerPhone;
    vector<OrderItem> items;
    Money totalAmount = 0;
    int64_t createdAt = 0; // Unix milliseconds
    int priority = 2;
    OrderStatus status = OrderStatus::PENDING;
    shared_ptr<const StatusHistoryNode> historyHead;
};
struct RequestedItem {
    string productId;
    Quantity quantity = 0;
};
struct CreateOrderRequest {
    string customerName;
    string customerPhone;
    vector<RequestedItem> items;
    int priority = 2;
};
enum class ErrorCode {
    NONE, INVALID_INPUT, DUPLICATE_ID, PRODUCT_NOT_FOUND,
    INSUFFICIENT_STOCK, AMOUNT_OVERFLOW, ORDER_NOT_FOUND,
    INVALID_TRANSITION, PRODUCT_IN_USE, EMPTY_QUEUE, IO_ERROR, INVALID_DATA
};
struct Result {
    ErrorCode code = ErrorCode::NONE;
    string message;
    string orderId;
    bool ok() const { return code == ErrorCode::NONE; }
};
} // namespace shop
