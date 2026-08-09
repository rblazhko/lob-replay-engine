#include "lob/order_book.hpp"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>

int main() {
    constexpr std::size_t order_total = 250'000;
    constexpr std::size_t market_order_total = 25'000;
    lob::OrderBook book;

    const auto started = std::chrono::steady_clock::now();
    for (std::size_t index = 1; index <= order_total; ++index) {
        const auto side = index % 2 == 0 ? lob::Side::buy : lob::Side::sell;
        const lob::Price price = side == lob::Side::buy
            ? 10'000 - static_cast<lob::Price>(index % 50)
            : 10'001 + static_cast<lob::Price>(index % 50);
        book.add(index, side, price, 1 + index % 10);
    }

    std::size_t cancelled = 0;
    for (std::size_t id = 4; id <= order_total; id += 4) {
        cancelled += book.cancel(id) ? 1 : 0;
    }

    std::size_t fills = 0;
    for (std::size_t index = 0; index < market_order_total; ++index) {
        const auto side = index % 2 == 0 ? lob::Side::buy : lob::Side::sell;
        fills += book.execute_market(side, 5).size();
    }
    const auto finished = std::chrono::steady_clock::now();

    const double seconds = std::chrono::duration<double>(finished - started).count();
    const auto operations = order_total + cancelled + market_order_total;
    std::cout << "operations=" << operations << '\n'
              << "fills=" << fills << '\n'
              << "live_orders=" << book.order_count() << '\n'
              << std::fixed << std::setprecision(0)
              << "operations_per_second=" << operations / seconds << '\n';
}
