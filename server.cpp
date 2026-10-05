#include "third_party/httplib.h"
#include "third_party/json.hpp"
#include "FileStorage.cpp"
#include "ProductManagement.cpp"
#include "OrderLookup.cpp"
#include "PriceSearch.cpp"
#include "PriorityOrders.cpp"
#include "LowStock.cpp"
#include <iostream>
#include <mutex>

using namespace std;
using namespace shop;
using Json = nlohmann::json;

namespace {
Json productJson(const Product& p) {
    return {{"id", p.productId}, {"name", p.name}, {"price", p.price}, {"stock", p.stock}, {"minStock", p.minStock}, {"category", p.category}, {"brand", p.brand}};
}
Json orderJson(const Order& o) {
    Json items = Json::array(), history = Json::array();
    for (const auto& i : o.items)
        items.push_back({{"productId", i.productId}, {"productName", i.productName}, {"quantity", i.quantity}, {"unitPrice", i.unitPrice}});
    for (auto node = o.historyHead; node; node = node->next)
        history.push_back({{"status", statusToString(node->status)}, {"changedAt", node->changedAt}});
    reverse(history.begin(), history.end());
    return {{"orderId", o.orderId}, {"customerName", o.customerName}, {"customerPhone", o.customerPhone},
        {"createdAt", o.createdAt}, {"totalAmount", o.totalAmount}, {"priority", o.priority},
        {"status", statusToString(o.status)}, {"items", items}, {"history", history}};
}
void reply(httplib::Response& res, const Json& data, int status = 200) {
    res.status = status;
    res.set_header("Cache-Control", "no-store");
    res.set_content(data.dump(), "application/json; charset=utf-8");
}
void failure(httplib::Response& res, const string& message, int status = 400) {
    reply(res, {{"error", message}}, status);
}
int64_t integer(const Json& value) {
    constexpr int64_t safe = 9007199254740991LL;
    if (!value.is_number_integer() || (value.is_number_unsigned() && value.get<uint64_t>() > static_cast<uint64_t>(safe)))
        throw invalid_argument("So phai la so nguyen trong gioi han JavaScript.");
    const auto n = value.get<int64_t>();
    if (n < -safe || n > safe) throw invalid_argument("So vuot gioi han JavaScript.");
    return n;
}
}

int main(int argc, char** argv) {
    string filename = "data/shop.csv", webRoot = "web";
    int port = 8080;
    try {
        for (int i = 1; i < argc; ++i) {
            const string arg = argv[i];
            if (i + 1 >= argc) throw invalid_argument("Thieu gia tri tham so.");
            if (arg == "--data") filename = argv[++i];
            else if (arg == "--web") webRoot = argv[++i];
            else if (arg == "--port") { const string raw = argv[++i]; size_t used; port = stoi(raw, &used); if (used != raw.size() || port < 1 || port > 65535) throw invalid_argument("Port khong hop le."); }
            else throw invalid_argument("Tham so khong hop le.");
        }
        ProductStore products; OrderStore orders;
        FileStorage storage(products, orders);
        const auto loaded = storage.load(filename);
        if (!loaded.ok()) { cerr << loaded.message << '\n'; return 1; }
        // JSON numbers must remain exact in the browser.
        constexpr int64_t safe = 9007199254740991LL;
        bool representable = true;
        products.records.forEach([&](const auto&, const Product& p) { if (p.price > safe || p.stock > safe || p.minStock > safe) representable = false; });
        orders.records.forEach([&](const auto&, const Order& o) { if (o.totalAmount > safe || o.createdAt > safe) representable = false;
            for (const auto& i : o.items) if (i.quantity > safe || i.unitPrice > safe) representable = false;
            for (auto node = o.historyHead; node; node = node->next) if (node->changedAt > safe) representable = false; });
        if (!representable) { cerr << "Du lieu vuot gioi han so cua giao dien web.\n"; return 1; }
        mutex dataMutex;
        PriceSearchService priceSearch(products);
        httplib::Server server;
        server.set_payload_max_length(65536);
        if (!server.set_mount_point("/", webRoot)) { cerr << "Khong tim thay thu muc web.\n"; return 1; }
        server.Get("/api/data", [&](const httplib::Request&, httplib::Response& res) {
            lock_guard<mutex> lock(dataMutex);
            Json ps = Json::array(), os = Json::array();
            for (const auto& p : ProductService(products).listProducts()) ps.push_back(productJson(p));
            for (const auto& o : OrderService(products, orders).listOrders()) os.push_back(orderJson(o));
            Json warnings = Json::array();
            for (const auto& p : LowStockService(products).warnings()) warnings.push_back(productJson(p));
            reply(res, {{"products", ps}, {"orders", os}, {"warnings", warnings}});
        });
        server.Get("/api/products/lookup", [&](const auto& req, auto& res) {
            lock_guard<mutex> lock(dataMutex);
            const auto* p = ProductService(products).findById(req.get_param_value("id"));
            if (!p) { failure(res, "Khong tim thay san pham.", 404); return; }
            reply(res, {{"product", productJson(*p)}});
        });
        server.Get("/api/orders/lookup", [&](const auto& req, auto& res) {
            lock_guard<mutex> lock(dataMutex);
            const auto* o = OrderLookupService(orders).findById(req.get_param_value("id"));
            if (!o) { failure(res, "Khong tim thay don hang.", 404); return; }
            reply(res, {{"order", orderJson(*o)}});
        });
        server.Get("/api/products/range", [&](const auto& req, auto& res) {
            int64_t minPrice, maxPrice;
            if (!csv::integer(req.get_param_value("min"), minPrice) || !csv::integer(req.get_param_value("max"), maxPrice) ||
                minPrice > safe || maxPrice > safe) { failure(res, "Khoang gia khong hop le."); return; }
            lock_guard<mutex> lock(dataMutex);
            vector<Product> matches;
            auto result = priceSearch.findInRange(minPrice, maxPrice, matches);
            if (!result.ok()) { failure(res, result.message); return; }
            Json ps = Json::array(); for (const auto& p : matches) ps.push_back(productJson(p));
            reply(res, {{"products", ps}});
        });
        server.Get("/api/export", [&](const auto&, auto& res) {
            lock_guard<mutex> lock(dataMutex);
            ifstream input(filename, ios::binary);
            if (!input) { failure(res, "Khong doc duoc CSV.", 500); return; }
            string data((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
            res.set_header("Content-Disposition", "attachment; filename=shop.csv");
            res.set_header("Cache-Control", "no-store");
            res.set_content(data, "text/csv; charset=utf-8");
        });
        server.Post("/api/save", [&](const auto& req, auto& res) {
            const string origin = req.get_header_value("Origin");
            if ((!origin.empty() && origin != "http://127.0.0.1:" + to_string(port) && origin != "http://localhost:" + to_string(port)) ||
                req.get_header_value("Content-Type").find("application/json") != 0) {
                failure(res, "Yeu cau phai den tu giao dien cung server va dung JSON.", 403); return;
            }
            try {
                lock_guard<mutex> lock(dataMutex);
                const auto result = storage.save(filename);
                if (!result.ok()) { failure(res, result.message, 500); return; }
                reply(res, {{"message", "Da luu CSV."}});
            } catch (const exception&) { failure(res, "Khong the luu CSV.", 500); }
        });
        auto mutate = [&](const httplib::Request& req, httplib::Response& res, int operation) {
            const string origin = req.get_header_value("Origin");
            if ((!origin.empty() && origin != "http://127.0.0.1:" + to_string(port) && origin != "http://localhost:" + to_string(port)) ||
                req.get_header_value("Content-Type").find("application/json") != 0) {
                failure(res, "Yeu cau phai den tu giao dien cung server va dung JSON.", 403); return;
            }
            try {
                const auto body = Json::parse(req.body);
                lock_guard<mutex> lock(dataMutex);
                // Apply to temporary stores, save first, then commit. Failed CSV writes leave live state unchanged.
                ProductStore stagedProducts; OrderStore stagedOrders;
                products.records.forEach([&](const auto& id, const Product& p) { stagedProducts.records.insert(id, p); });
                orders.records.forEach([&](const auto& id, const Order& o) { stagedOrders.records.insert(id, o); });
                Result result;
                string id;
                if (operation == 2) {
                    OrderStatusService status(stagedProducts, stagedOrders);
                    result = PriorityOrderService(stagedOrders, status).processNext();
                    id = result.orderId;
                } else if (operation == 1) {
                    id = req.matches[1].str();
                    auto* current = stagedOrders.records.find(id);
                    if (!current) { failure(res, "Khong tim thay don hang.", 404); return; }
                    if (body.at("expectedStatus").get<string>() != statusToString(current->status)) {
                        failure(res, "Don da doi trang thai. Tai lai du lieu truoc khi thao tac.", 409); return;
                    }
                    OrderStatus target;
                    if (!csv::status(body.at("status").get<string>(), target)) { failure(res, "Trang thai khong hop le."); return; }
                    result = OrderStatusService(stagedProducts, stagedOrders).updateStatus(id, target);
                } else {
                    CreateOrderRequest request;
                    request.customerName = body.at("customerName").get<string>();
                    request.customerPhone = body.at("customerPhone").get<string>();
                    const auto priority = integer(body.at("priority"));
                    if (priority < 1 || priority > 3) throw invalid_argument("Uu tien phai tu 1 den 3.");
                    request.priority = static_cast<int>(priority);
                    const auto& items = body.at("items");
                    if (!items.is_array() || items.size() > 100 || request.customerName.size() > 400 || request.customerPhone.size() > 120)
                        throw invalid_argument("Du lieu don hang khong hop le.");
                    for (const auto& item : items) request.items.push_back({item.at("productId").get<string>(), integer(item.at("quantity"))});
                    result = OrderService(stagedProducts, stagedOrders).createOrder(request);
                    id = result.orderId;
                    if (result.ok() && stagedOrders.records.find(id)->totalAmount > safe) { failure(res, "Tong tien vuot gioi han giao dien."); return; }
                }
                if (!result.ok()) { failure(res, result.message, result.code == ErrorCode::INVALID_TRANSITION ? 409 : 400); return; }
                bool safeStock = true;
                stagedProducts.records.forEach([&](const auto&, const Product& p) { if (p.stock > safe) safeStock = false; });
                if (!safeStock) { failure(res, "Ton kho vuot gioi han giao dien."); return; }
                Json response = {{"order", orderJson(*stagedOrders.records.find(id))}};
                // Prepare serialized response before persisting the transaction.
                const string responseText = response.dump();
                result = FileStorage(stagedProducts, stagedOrders).save(filename);
                if (!result.ok()) { failure(res, result.message, 500); return; }
                products.records.swap(stagedProducts.records); orders.records.swap(stagedOrders.records);
                ++products.revision; ++orders.revision;
                res.status = operation == 0 ? 201 : 200;
                res.set_header("Cache-Control", "no-store");
                res.set_content(responseText, "application/json; charset=utf-8");
            } catch (const Json::exception&) { failure(res, "JSON thieu truong hoac sai kieu du lieu."); }
              catch (const invalid_argument& e) { failure(res, e.what()); }
              catch (const exception&) { failure(res, "Server khong the xu ly yeu cau.", 500); }
        };
        server.Post("/api/orders", [&](const auto& req, auto& res) { mutate(req, res, false); });
        server.Post(R"(/api/orders/(ORD[0-9]+)/status)", [&](const auto& req, auto& res) { mutate(req, res, true); });
        server.Post("/api/orders/process-next", [&](const auto& req, auto& res) { mutate(req, res, 2); });
        auto mutateProduct = [&](const httplib::Request& req, httplib::Response& res, int operation) {
            const string origin = req.get_header_value("Origin");
            if ((!origin.empty() && origin != "http://127.0.0.1:" + to_string(port) && origin != "http://localhost:" + to_string(port)) ||
                req.get_header_value("Content-Type").find("application/json") != 0) {
                failure(res, "Yeu cau phai den tu giao dien cung server va dung JSON.", 403); return;
            }
            try {
                const auto body = Json::parse(req.body);
                const string id = body.at("id").get<string>();
                lock_guard<mutex> lock(dataMutex);
                const auto* current = products.records.find(id);
                if (operation != 2 && !current) { failure(res, "Khong tim thay san pham.", 404); return; }
                if (operation != 2 && body.at("expectedProduct") != productJson(*current)) {
                    failure(res, "San pham da thay doi. Dong form va tai lai trang truoc khi thao tac.", 409); return;
                }
                ProductStore stagedProducts; OrderStore stagedOrders;
                products.records.forEach([&](const auto& key, const Product& p) { stagedProducts.records.insert(key, p); });
                orders.records.forEach([&](const auto& key, const Order& o) { stagedOrders.records.insert(key, o); });
                ProductManagementService management(stagedProducts, stagedOrders);
                Result result;
                if (operation == 2) {
                    Product p; p.productId = id; p.name = body.at("name").get<string>();
                    p.price = integer(body.at("price")); p.stock = integer(body.at("stock")); p.minStock = integer(body.at("minStock"));
                    p.category = body.value("category", string{}); p.brand = body.value("brand", string{});
                    if (id.size() > 120 || p.name.size() > 400) throw invalid_argument("Ma hoac ten san pham qua dai.");
                    result = management.addProduct(move(p));
                } else if (operation == 3) {
                    result = management.removeProduct(id);
                } else if (operation == 1) {
                    result = management.changeStock(id, integer(body.at("quantity")), body.at("incoming").get<bool>());
                } else {
                    const string name = body.at("name").get<string>();
                    if (name.size() > 400) throw invalid_argument("Ten san pham qua dai.");
                    result = management.updateInfo(id, name, integer(body.at("price")), integer(body.at("minStock")));
                    if (result.ok()) result = management.updateClassification(id,
                        body.value("category", current->category), body.value("brand", current->brand));
                }
                if (!result.ok()) { failure(res, result.message); return; }
                const auto* changed = stagedProducts.records.find(id);
                if (changed && changed->stock > safe) { failure(res, "Ton kho vuot gioi han giao dien."); return; }
                const string response = (operation == 3 ? Json{{"deletedId", id}} : Json{{"product", productJson(*changed)}}).dump();
                result = FileStorage(stagedProducts, stagedOrders).save(filename);
                if (!result.ok()) { failure(res, result.message, 500); return; }
                products.records.swap(stagedProducts.records);
                ++products.revision;
                res.set_header("Cache-Control", "no-store");
                res.set_content(response, "application/json; charset=utf-8");
            } catch (const Json::exception&) { failure(res, "JSON thieu truong hoac sai kieu du lieu."); }
              catch (const invalid_argument& e) { failure(res, e.what()); }
              catch (const exception&) { failure(res, "Server khong the xu ly yeu cau.", 500); }
        };
        server.Post("/api/products/update", [&](const auto& req, auto& res) { mutateProduct(req, res, false); });
        server.Post("/api/products/stock", [&](const auto& req, auto& res) { mutateProduct(req, res, true); });
        server.Post("/api/products/add", [&](const auto& req, auto& res) { mutateProduct(req, res, 2); });
        server.Post("/api/products/remove", [&](const auto& req, auto& res) { mutateProduct(req, res, 3); });
        cout << "Mo http://127.0.0.1:" << port << " | CSV: " << filename << "\nKhong chay console cung ghi CSV khi server dang hoat dong.\n" << flush;
        if (!server.listen("127.0.0.1", port)) { cerr << "Khong mo duoc port.\n"; return 1; }
    } catch (const exception& e) { cerr << e.what() << '\n'; return 1; }
}
