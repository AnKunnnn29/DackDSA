#include "FileStorage.cpp"
#include "OrderLookup.cpp"
#include "PriceSearch.cpp"
#include "PriorityOrders.cpp"
#include "LowStock.cpp"
#include <iostream>
#include <random>
#include <stdexcept>

using namespace std;
int checks = 0;
void check(bool condition, const char* text, int line) {
    ++checks;
    if (!condition) throw runtime_error("FAIL line " + to_string(line) + ": " + text);
}
#define CHECK(x) check((x), #x, __LINE__)

void structures() {
    dsa::HashTable<int> hash(0);
    CHECK(hash.insert("stable", 42));
    const auto* ptr = hash.find("stable");
    for (int i = 0; i < 10000; ++i) CHECK(hash.insert("P" + to_string(i), i));
    CHECK(hash.find("stable") == ptr);
    CHECK(!hash.insert("stable", 99));
    CHECK(*ptr == 42);
    for (int i = 0; i < 10000; ++i) CHECK(*hash.find("P" + to_string(i)) == i);
    CHECK(hash.erase("P0")); CHECK(!hash.find("P0")); CHECK(!hash.erase("P0"));
    using shop::PriorityEntry;
    auto before = [](const PriorityEntry& a, const PriorityEntry& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.createdAt != b.createdAt) return a.createdAt < b.createdAt;
        if (a.orderId.size() != b.orderId.size()) return a.orderId.size() < b.orderId.size();
        return a.orderId < b.orderId;
    };
    vector<PriorityEntry> entries;
    mt19937 random(42);
    for (int i = 0; i < 2000; ++i)
        entries.push_back({"ORD" + to_string(i), static_cast<int>(random() % 3 + 1), static_cast<int64_t>(random() % 100)});
    auto sorted = entries;
    sort(sorted.begin(), sorted.end(), before);
    shop::OrderMinHeap heap;
    heap.build(entries);
    for (const auto& expected : sorted) {
        CHECK(heap.top() && heap.top()->orderId == expected.orderId); heap.pop();
    }
    CHECK(heap.size() == 0); heap.pop(); CHECK(heap.top() == nullptr);
    for (const auto& entry : entries) heap.push(entry);
    for (const auto& expected : sorted) {
        CHECK(heap.top() && heap.top()->orderId == expected.orderId); heap.pop();
    }
}

void workflows() {
    using namespace shop;
    ProductStore products; OrderStore orders;
    ProductService catalog(products);
    ProductManagementService management(products, orders);
    OrderService creation(products, orders);
    OrderLookupService lookup(orders);
    PriceSearchService prices(products);
    OrderStatusService status(products, orders);
    PriorityOrderService priority(orders, status);
    LowStockService low(products);
    CHECK(priority.processNext().code == ErrorCode::EMPTY_QUEUE);
    CHECK(catalog.addProduct({"P001", "Mouse", 500000, 20, 5}).ok());
    CHECK(catalog.addProduct({"P002", "Keyboard", 750000, 10, 3}).ok());
    CHECK(catalog.addProduct({"P003", "Screen", 3000000, 5, 5}).ok());
    CHECK(catalog.addProduct({"P001", "Duplicate", 1, 1, 0}).code == ErrorCode::DUPLICATE_ID);
    CHECK(catalog.addProduct({"bad id", "Bad", 1, 1, 0}).code == ErrorCode::INVALID_INPUT);
    CHECK(catalog.addProduct({"BAD", "Bad", -1, 1, 0}).code == ErrorCode::INVALID_INPUT);
    CHECK(catalog.addProduct({"BAD", "Bad", 1, -1, 0}).code == ErrorCode::INVALID_INPUT);
    CHECK(catalog.findById("p001") == nullptr);
    CHECK(low.warnings().size() == 1); // inclusive minStock, no totalStock percentage
    CHECK(low.warnings()[0].productId == "P003");
    vector<Product> found;
    CHECK(prices.findInRange(500000, 750000, found).ok()); CHECK(found.size() == 2);
    CHECK(prices.findInRange(750000, 750000, found).ok()); CHECK(found.size() == 1);
    CHECK(prices.findInRange(800000, 900000, found).ok()); CHECK(found.empty());
    CHECK(prices.findInRange(10, 1, found).code == ErrorCode::INVALID_INPUT);
    CHECK(management.updateInfo("P003", "Screen new", 750000, 5).ok());
    CHECK(prices.findInRange(750000, 750000, found).ok()); CHECK(found.size() == 2);
    CHECK(found[0].productId == "P002" && found[1].productId == "P003");
    CHECK(management.changeStock("P001", 2, true).ok()); CHECK(catalog.findById("P001")->stock == 22);
    CHECK(management.changeStock("P001", 2, false).ok()); CHECK(catalog.findById("P001")->stock == 20);
    CHECK(management.changeStock("P001", 21, false).code == ErrorCode::INSUFFICIENT_STOCK);
    CHECK(management.changeStock("P001", 0, true).code == ErrorCode::INVALID_INPUT);
    CHECK(management.removeProduct("missing").code == ErrorCode::PRODUCT_NOT_FOUND);
    auto first = creation.createOrder({"An", "090", {{"P001", 2}, {"P002", 1}, {"P001", 3}}, 3});
    CHECK(first.ok());
    const auto* order = lookup.findById(first.orderId);
    CHECK(order && order->items.size() == 2 && order->totalAmount == 3250000);
    CHECK(catalog.findById("P001")->stock == 15);
    CHECK(status.history(first.orderId).size() == 1);
    auto reject = [&](const CreateOrderRequest& request, ErrorCode code) {
        const auto stock = catalog.findById("P001")->stock;
        const auto count = orders.records.size();
        CHECK(creation.createOrder(request).code == code);
        CHECK(catalog.findById("P001")->stock == stock && orders.records.size() == count);
    };
    reject({"An", "090", {{"P001", 1}, {"missing", 1}}, 2}, ErrorCode::PRODUCT_NOT_FOUND);
    reject({"An", "090", {{"P001", 1}, {"P002", 100}}, 2}, ErrorCode::INSUFFICIENT_STOCK);
    reject({"An", "090", {{"P001", 10}, {"P001", 10}}, 2}, ErrorCode::INSUFFICIENT_STOCK);
    reject({"An", "090", {{"P001", 0}}, 2}, ErrorCode::INVALID_INPUT);
    reject({"An", "090", {}, 2}, ErrorCode::INVALID_INPUT);
    reject({" ", "090", {{"P001", 1}}, 2}, ErrorCode::INVALID_INPUT);
    reject({"An", "090", {{"P001", 1}}, 4}, ErrorCode::INVALID_INPUT);
    const auto max = numeric_limits<Quantity>::max();
    reject({"An", "090", {{"P001", max}, {"P001", 1}}, 2}, ErrorCode::AMOUNT_OVERFLOW);
    CHECK(catalog.addProduct({"BIG", "Big", max, 2, 0}).ok());
    reject({"An", "090", {{"BIG", 2}}, 2}, ErrorCode::AMOUNT_OVERFLOW);
    reject({"An", "090", {{"BIG", 1}, {"P001", 1}}, 2}, ErrorCode::AMOUNT_OVERFLOW);
    CHECK(catalog.findById("BIG")->stock == 2);
    CHECK(management.updateInfo("P001", "New Mouse", 600000, 5).ok());
    CHECK(order->items[0].productName == "Mouse" && order->items[0].unitPrice == 500000);
    CHECK(management.removeProduct("P001").code == ErrorCode::PRODUCT_IN_USE);
    CHECK(management.removeProduct("BIG").ok());
    auto high = creation.createOrder({"Binh", "091", {{"P001", 1}}, 1});
    CHECK(high.ok()); CHECK(priority.pendingCount() == 2);
    CHECK(priority.processNext().orderId == high.orderId);
    CHECK(lookup.findById(high.orderId)->status == OrderStatus::PROCESSING); // original shared record
    CHECK(status.history(high.orderId).size() == 2);
    CHECK(status.updateStatus(first.orderId, OrderStatus::CANCELLED).ok());
    CHECK(catalog.findById("P001")->stock == 19);
    CHECK(catalog.findById("P002")->stock == 10);
    CHECK(status.updateStatus(first.orderId, OrderStatus::CANCELLED).code == ErrorCode::INVALID_TRANSITION);
    CHECK(catalog.findById("P001")->stock == 19);
    CHECK(priority.pendingCount() == 0); CHECK(priority.processNext().code == ErrorCode::EMPTY_QUEUE);
    CHECK(status.updateStatus(high.orderId, OrderStatus::COMPLETED).code == ErrorCode::INVALID_TRANSITION);
    CHECK(status.updateStatus(high.orderId, OrderStatus::SHIPPING).ok());
    CHECK(status.updateStatus(high.orderId, OrderStatus::CANCELLED).code == ErrorCode::INVALID_TRANSITION);
    CHECK(status.updateStatus(high.orderId, OrderStatus::COMPLETED).ok());
    CHECK(status.history(high.orderId).size() == 4);
    CHECK(status.updateStatus(high.orderId, OrderStatus::PROCESSING).code == ErrorCode::INVALID_TRANSITION);
    const Order copy = *lookup.findById(high.orderId);
    CHECK(copy.historyHead && copy.historyHead->status == OrderStatus::COMPLETED);
    auto equal1 = creation.createOrder({"C", "092", {{"P001", 1}}, 2});
    auto equal2 = creation.createOrder({"D", "093", {{"P001", 1}}, 2});
    CHECK(equal1.ok() && equal2.ok());
    CHECK(priority.processNext().orderId == equal1.orderId);
    CHECK(priority.processNext().orderId == equal2.orderId);
    CHECK(status.updateStatus(equal1.orderId, OrderStatus::CANCELLED).ok()); // PROCESSING cancellation
    CHECK(catalog.addProduct({"FREE", "Free", 0, 1, 0}).ok());
    auto free = creation.createOrder({"E", "094", {{"FREE", 1}}, 2});
    CHECK(free.ok() && lookup.findById(free.orderId)->totalAmount == 0);
    CHECK(catalog.findById("FREE")->stock == 0);
    CHECK(management.changeStock("FREE", max, true).ok());
    CHECK(status.updateStatus(free.orderId, OrderStatus::CANCELLED).code == ErrorCode::AMOUNT_OVERFLOW);
    CHECK(lookup.findById(free.orderId)->status == OrderStatus::PENDING);
    CHECK(management.changeStock("FREE", 1, true).code == ErrorCode::AMOUNT_OVERFLOW);
    CHECK(management.changeStock("FREE", max, false).ok());
    CHECK(status.updateStatus(free.orderId, OrderStatus::CANCELLED).ok());
    CHECK(catalog.findById("FREE")->stock == 1);

    // Round trip with commas, quotation marks, Unicode bytes and embedded newlines.
    CHECK(management.updateInfo("P002", "Bàn phím, \"VIP\"\nDòng 2", 750000, 3).ok());
    const auto directory = filesystem::temp_directory_path() /
        ("DackDSA-tests-" + to_string(nowMilliseconds()) + "-" + to_string(random_device{}()));
    CHECK(filesystem::create_directory(directory));
    const auto file = (directory / "shop.csv").string(), bad = (directory / "bad.csv").string();
    FileStorage storage(products, orders);
    CHECK(storage.save(file).ok()); CHECK(storage.save(file).ok()); // replace existing snapshot safely
    const auto originalStock = catalog.findById("P001")->stock;
    ProductStore recoveredProducts; OrderStore recoveredOrders;
    FileStorage recovery(recoveredProducts, recoveredOrders);
    CHECK(recovery.load(file).ok());
    CHECK(recoveredProducts.records.find("P001")->stock == originalStock);
    CHECK(recoveredProducts.records.find("P002")->name == "Bàn phím, \"VIP\"\nDòng 2");
    CHECK(recoveredOrders.records.size() == orders.records.size());
    CHECK(recoveredOrders.records.find(high.orderId)->historyHead->status == OrderStatus::COMPLETED);
    CHECK(recoveredOrders.records.find(first.orderId)->status == OrderStatus::CANCELLED);
    OrderService newCreation(recoveredProducts, recoveredOrders);
    auto additional = newCreation.createOrder({"F", "095", {{"P001", 1}}, 1});
    CHECK(additional.ok() && additional.orderId != first.orderId);
    // Fill the cached price view/heap before loading a replacement snapshot.
    CHECK(prices.findInRange(750000, 750000, found).ok());
    priority.pendingCount();
    CHECK(management.updateInfo("P002", "Changed", 1, 3).ok());
    CHECK(storage.load(file).ok());
    CHECK(catalog.findById("P001")->stock == originalStock); // no second reservation
    CHECK(prices.findInRange(750000, 750000, found).ok() && found.size() == 2);
    CHECK(priority.pendingCount() == 0);
    const auto beforeCount = orders.records.size();
    auto rejectFile = [&](const string& contents) {
        { ofstream output(bad, ios::binary); output << contents; }
        CHECK(storage.load(bad).code == ErrorCode::INVALID_DATA);
        CHECK(orders.records.size() == beforeCount && catalog.findById("P001")->stock == originalStock);
    };
    rejectFile("VERSION,1\nPRODUCT,P1,Name,-1,10,5\n");
    rejectFile("VERSION,1\nPRODUCT,P1,Name,1,10,5\nPRODUCT,P1,Name,1,10,5\n");
    rejectFile("VERSION,1\n\"unterminated");
    rejectFile("VERSION,1\nORDER,ORD1,A,090,1,1,2,PENDING\n");
    rejectFile("VERSION,1\nPRODUCT,P1,Name,1,10,5\nORDER,ORD1,A,090,2,1,2,PENDING\nITEM,ORD1,P1,Name,1,1\nHISTORY,ORD1,PENDING,1\n");
    CHECK(storage.load((directory / "missing.csv").string()).code == ErrorCode::IO_ERROR);
    const auto blocker = file + ".tmp";
    { ofstream output(blocker); output << "blocked"; }
    CHECK(storage.save(file).code == ErrorCode::IO_ERROR);
    filesystem::remove(blocker);
    filesystem::remove(file); filesystem::remove(bad); filesystem::remove(directory);
}
int main() {
    try { structures(); workflows(); cout << "PASS: " << checks << " checks\n"; }
    catch (const exception& error) { cerr << error.what() << '\n'; return 1; }
}
