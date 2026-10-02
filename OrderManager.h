#pragma once
#include "ProductLookup.h"

namespace shop {
using namespace std;
struct OrderStore { dsa::HashTable<Order> records; };

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
