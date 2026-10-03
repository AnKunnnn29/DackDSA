#include <iostream>
#include <unordered_map>
#include <string>
using namespace std;

//KHAI BÁO CÁC TRẠNG THÁI ĐƠN HÀNG

enum Status {
    PENDING,
    PROCESSING,
    SHIPPING,
    COMPLETED,
    CANCELLED
};

// Hàm chuyển trạng thái sang chuỗi
string statusToString(Status status) {
    switch (status) {
        case PENDING:
            return "PENDING";
        case PROCESSING:
            return "PROCESSING";
        case SHIPPING:
            return "SHIPPING";
        case COMPLETED:
            return "COMPLETED";
        case CANCELLED:
            return "CANCELLED";
    }

    return "UNKNOWN";
}

// NODE - LƯU LỊCH SỬ TRẠNG THÁI

struct HistoryNode {
    Status status;
    HistoryNode* next;

    HistoryNode(Status s) {
        status = s;
        next = NULL;
    }
};

// 3. CẤU TRÚC ORDER

struct Order {
    string orderID;
    string customerName;
    Status status;

    // Linked List lưu lịch sử trạng thái
    HistoryNode* historyHead;

    Order() {
        orderID = "";
        customerName = "";
        status = PENDING;
        historyHead = NULL;
    }

    Order(string id, string name) {
        orderID = id;
        customerName = name;

        // Đơn hàng mới bắt đầu ở PENDING
        status = PENDING;

        // Ban đầu lịch sử rỗng
        historyHead = NULL;

        // Thêm trạng thái PENDING vào lịch sử
        addHistory(PENDING);
    }

    // Thêm trạng thái vào đầu Linked List
 
    void addHistory(Status newStatus) {

        HistoryNode* newNode = new HistoryNode(newStatus);
        newNode->next = historyHead;
        historyHead = newNode;
    }

    // Kiểm tra chuyển trạng thái có hợp lệ không

    bool isValidTransition(Status newStatus) {

        // PENDING → PROCESSING
        // PENDING → CANCELLED
        if (status == PENDING) {
            if (newStatus == PROCESSING ||
                newStatus == CANCELLED) {
                return true;
            }
            return false;
        }

        // PROCESSING → SHIPPING
        // PROCESSING → CANCELLED
        if (status == PROCESSING) {
            if (newStatus == SHIPPING ||
                newStatus == CANCELLED) {
                return true;
            }
            return false;
        }

        // SHIPPING → COMPLETED
        if (status == SHIPPING) {
            if (newStatus == COMPLETED) {
                return true;
            }
            return false;
        }

        // COMPLETED không được chuyển tiếp
        if (status == COMPLETED) {
            return false;
        }

        // CANCELLED không được chuyển tiếp
        if (status == CANCELLED) {
            return false;
        }
        return false;
    }
    // Cập nhật trạng thái

    bool updateStatus(Status newStatus) {

        // Kiểm tra chuyển trạng thái
        if (!isValidTransition(newStatus)) {
            return false;
        }
        // Cập nhật trạng thái hiện tại
        status = newStatus;

        // Lưu vào lịch sử
        addHistory(newStatus);

        return true;
    }
    // Hiển thị lịch sử trạng thái

    void showHistory() {

        cout << "\nLich su trang thai cua don hang "<< orderID << ":\n";
        HistoryNode* current = historyHead;
        while (current != NULL) {
            cout << "-> "<< statusToString(current->status)<< endl;
            current = current->next;
        }
    }

    // Hiển thị thông tin đơn hàng

    void display() {

        cout << "\n-----------------------------\n";

        cout << "Order ID: "
             << orderID << endl;

        cout << "Customer: "
             << customerName << endl;

        cout << "Status: "
             << statusToString(status) << endl;

        cout << "-----------------------------\n";
    }
};

//HỆ THỐNG QUẢN LÝ ĐƠN HÀNG

class OrderManager {

private:

    // Hash Table
    // Key   = Order ID
    // Value = Order
    unordered_map<string, Order> orders;

public:
    // Thêm đơn hàng
    void addOrder(string id, string customerName) {
        // Kiểm tra Order ID đã tồn tại chưa
        if (orders.find(id) != orders.end()) {
            cout << "Order ID da ton tai!\n";
            return;
        }
        // Tạo đơn hàng
        Order newOrder(id, customerName);

        // Đưa vào Hash Table
        orders[id] = newOrder;
        cout << "Them don hang thanh cong!\n";
    }

    // Tìm đơn hàng

    Order* findOrder(string id) {
        auto it = orders.find(id);
        if (it == orders.end()) {
            return NULL;
        }
        return &it->second;
    }

    // Cập nhật trạng thái
    void updateOrderStatus(string id, Status newStatus) {

        Order* order = findOrder(id);

        // Không tìm thấy đơn hàng
        if (order == NULL) {
            cout << "Khong tim thay don hang!\n";
            return;
        }

        // Thử cập nhật
        if (order->updateStatus(newStatus)) {
            cout << "Cap nhat trang thai thanh cong!\n";
            cout << "Trang thai moi: "
                 << statusToString(order->status)
                 << endl;
        }
        else {
            cout << "Khong the chuyen trang thai!\n";
            cout << "Trang thai hien tai: "
                 << statusToString(order->status)
                 << endl;
        }
    }
  
    // Hiển thị đơn hàng

    void showOrder(string id) {
        Order* order = findOrder(id);
        if (order == NULL) {
            cout << "Khong tim thay don hang!\n";
            return;
        }
        order->display();
    }

    // Hiển thị lịch sử
    void showOrderHistory(string id) {
        Order* order = findOrder(id);
        if (order == NULL) {
            cout << "Khong tim thay don hang!\n";
            return;
        }
        order->showHistory();
    }
};

