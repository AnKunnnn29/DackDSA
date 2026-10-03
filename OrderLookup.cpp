#ifndef DACKDSA_ORDERLOOKUP_CPP
#define DACKDSA_ORDERLOOKUP_CPP
#include "OrderManager.cpp"

namespace shop {
// Dinh Hai's order-ID lookup uses the shared hash store, not a second order list.
class OrderLookupService {
    const OrderStore& orders_;
public:
    explicit OrderLookupService(const OrderStore& orders) : orders_(orders) {}
    const Order* findById(const string& id) const {
        return orderManagerValidId(id) ? orders_.records.find(id) : nullptr;
    }
};
} // namespace shop
#endif
