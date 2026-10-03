#ifndef DACKDSA_PRIORITYORDERS_CPP
#define DACKDSA_PRIORITYORDERS_CPP
#include "OrderStatus.cpp"

namespace shop {
using namespace std;
struct PriorityEntry {
    string orderId;
    int priority;
    int64_t createdAt;
};
// Self-implemented binary min-heap, never std::priority_queue.
class OrderMinHeap {
    vector<PriorityEntry> data_;
    static bool before(const PriorityEntry& a, const PriorityEntry& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.createdAt != b.createdAt) return a.createdAt < b.createdAt;
        if (a.orderId.size() != b.orderId.size()) return a.orderId.size() < b.orderId.size();
        return a.orderId < b.orderId;
    }
    void siftDown(size_t i) {
        while (true) {
            size_t child = i * 2 + 1;
            if (child >= data_.size()) break;
            if (child + 1 < data_.size() && before(data_[child + 1], data_[child])) ++child;
            if (!before(data_[child], data_[i])) break;
            swap(data_[i], data_[child]);
            i = child;
        }
    }
public:
    void build(vector<PriorityEntry> entries) {
        data_ = move(entries);
        for (size_t i = data_.size() / 2; i > 0; --i) siftDown(i - 1);
    }
    void push(PriorityEntry entry) {
        data_.push_back(move(entry));
        size_t i = data_.size() - 1;
        while (i > 0) {
            size_t parent = (i - 1) / 2;
            if (!before(data_[i], data_[parent])) break;
            swap(data_[i], data_[parent]); i = parent;
        }
    }
    const PriorityEntry* top() const { return data_.empty() ? nullptr : &data_.front(); }
    void pop() {
        if (data_.empty()) return;
        swap(data_.front(), data_.back()); data_.pop_back();
        if (!data_.empty()) siftDown(0);
    }
    size_t size() const { return data_.size(); }
};
class PriorityOrderService {
    OrderStore& orders_;
    OrderStatusService& status_;
    OrderMinHeap heap_;
    uint64_t seenRevision_ = 0;
    void refresh() {
        if (seenRevision_ == orders_.revision) return;
        vector<PriorityEntry> pending;
        orders_.records.forEach([&](const auto&, const Order& o) {
            if (o.status == OrderStatus::PENDING) pending.push_back({o.orderId, o.priority, o.createdAt});
        });
        heap_.build(move(pending));
        seenRevision_ = orders_.revision;
    }
public:
    PriorityOrderService(OrderStore& orders, OrderStatusService& status) : orders_(orders), status_(status) {}
    Result processNext() {
        refresh();
        if (!heap_.top()) return {ErrorCode::EMPTY_QUEUE, "Khong co don PENDING.", {}};
        const auto id = heap_.top()->orderId;
        Result success{ErrorCode::NONE, "Da chuyen don uu tien sang PROCESSING.", id};
        auto result = status_.updateStatus(id, OrderStatus::PROCESSING);
        if (!result.ok()) return result;
        heap_.pop();
        seenRevision_ = orders_.revision; // remaining cached entries are still pending
        return success;
    }
    size_t pendingCount() { refresh(); return heap_.size(); }
};
} // namespace shop
#endif
