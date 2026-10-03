#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <windows.h>

using namespace std;

enum OrderStatus { PENDING, PROCESSING, SHIPPING, COMPLETED, CANCELLED };

struct OrderItem {
    string productId;
    string productName;
    int quantity;
    double unitPrice;
};

// Struct thống nhất theo đề bài
struct Order {
    string orderId;
    string customerName;
    string customerPhone;
    vector<OrderItem> items;
    double totalAmount;
    string createdAt;
    int priority; // Mức độ ưu tiên
    OrderStatus status;
};

// Comparator cho Max-Heap (Ưu tiên cao hơn nằm ở đỉnh heap)
struct OrderPriorityComparator {
    bool operator()(const Order& a, const Order& b) {
        if (a.priority == b.priority) {
            return a.createdAt > b.createdAt; // Cùng ưu tiên thì đơn tạo trước ra trước
        }
        return a.priority < b.priority;
    }
};

class PriorityOrderProcessor {
private:
    priority_queue<Order, vector<Order>, OrderPriorityComparator> priorityQueue;

public:
    // Nhận 1 đơn hàng mới và đẩy trực tiếp vào Max-Heap
    void pushOrder(const Order& order) {
        priorityQueue.push(order);
    }

    // Nhận nguyên một danh sách đơn hàng được truyền vào
    void pushBulkOrders(const vector<Order>& orders) {
        for (const auto& ord : orders) {
            priorityQueue.push(ord);
        }
    }

    // Lấy đơn hàng có độ ưu tiên cao nhất ra để xử lý
    Order popNextPriorityOrder() {
        if (priorityQueue.empty()) {
            throw runtime_error("Hàng đợi đơn hàng đang rỗng!");
        }

        Order topOrder = priorityQueue.top();
        priorityQueue.pop();
        topOrder.status = PROCESSING;

        return topOrder;
    }

    bool isEmpty() const {
        return priorityQueue.empty();
    }

    size_t getPendingCount() const {
        return priorityQueue.size();
    }
};