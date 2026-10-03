#include "PriceSearch.cpp"
#include "PriorityOrders.cpp"
#include <iostream>
#include <random>

using namespace std;
using Clock = chrono::steady_clock;
template<class F> double timeWork(F work, int64_t& sum) {
    const auto start = Clock::now(); sum = work();
    return chrono::duration<double, micro>(Clock::now() - start).count();
}
template<class F, class G> void compare(const string& workload, int n, int operations, F fast, G linear) {
    vector<double> fastTimes, linearTimes;
    int64_t checksum = 0;
    for (int i = 0; i < 4; ++i) {
        int64_t a, b; double x, y;
        if (i % 2 == 0) { x = timeWork(fast, a); y = timeWork(linear, b); }
        else { y = timeWork(linear, b); x = timeWork(fast, a); }
        if (a != b) throw runtime_error("Benchmark result mismatch");
        checksum = a;
        if (i) { fastTimes.push_back(x); linearTimes.push_back(y); }
    }
    sort(fastTimes.begin(), fastTimes.end()); sort(linearTimes.begin(), linearTimes.end());
    cout << workload << ',' << n << ',' << operations << ',' << fastTimes[1] << ',' << linearTimes[1] << ',' << checksum << '\n';
}
int main() {
    cout << "workload,records,operations,optimized_median_us,linear_median_us,checksum\n";
    for (int n : {10000, 100000}) {
        shop::ProductStore products;
        shop::ProductService catalog(products);
        vector<shop::Product> linear;
        for (int i = 0; i < n; ++i) {
            shop::Product p{"P" + to_string(i), "Product", i * 10, 100, 5};
            catalog.addProduct(p); linear.push_back(move(p));
        }
        mt19937 random(42);
        vector<string> queries;
        vector<pair<int64_t,int64_t>> ranges;
        for (int i = 0; i < 1000; ++i) {
            queries.push_back(i % 5 == 0 ? "missing" : "P" + to_string(random() % n));
            int64_t low = (random() % n) * 10;
            ranges.emplace_back(low, low + 100);
        }
        compare("id_lookup", n, 1000, [&] {
            int64_t sum = 0;
            for (const auto& q : queries) { const auto* p = catalog.findById(q); sum += p ? p->price : -1; }
            return sum;
        }, [&] {
            int64_t sum = 0;
            for (const auto& q : queries) {
                auto p = find_if(linear.begin(), linear.end(), [&](const auto& p) { return p.productId == q; });
                sum += p == linear.end() ? -1 : p->price;
            }
            return sum;
        });
        shop::PriceSearchService prices(products);
        vector<shop::Product> found;
        prices.findInRange(0, 0, found); // initial sorting is excluded from repeated query timing
        compare("price_range_cached", n, 1000, [&] {
            int64_t sum = 0;
            for (const auto& range : ranges) { prices.findInRange(range.first, range.second, found); for (const auto& p : found) sum += p.price; }
            return sum;
        }, [&] {
            int64_t sum = 0;
            for (const auto& range : ranges)
                for (const auto& p : linear) if (p.price >= range.first && p.price <= range.second) sum += p.price;
            return sum;
        });
        vector<shop::PriorityEntry> entries;
        for (int i = 0; i < n; ++i) entries.push_back({"ORD" + to_string(i), static_cast<int>(random() % 3 + 1), i});
        auto before = [](const auto& a, const auto& b) {
            return a.priority != b.priority ? a.priority < b.priority : a.createdAt < b.createdAt;
        };
        compare("priority_build_and_pop", n, 1000, [&] {
            shop::OrderMinHeap heap; heap.build(entries);
            int64_t sum = 0;
            for (int i = 0; i < 1000; ++i) { sum += heap.top()->createdAt; heap.pop(); }
            return sum;
        }, [&] {
            auto remaining = entries; int64_t sum = 0;
            for (int i = 0; i < 1000; ++i) {
                auto best = min_element(remaining.begin(), remaining.end(), before);
                sum += best->createdAt; swap(*best, remaining.back()); remaining.pop_back();
            }
            return sum;
        });
    }
}
