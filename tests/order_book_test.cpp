#include "lob/order_book.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

template <typename Function>
void expect_invalid_argument(Function&& action, std::string_view message) {
    try {
        action();
        expect(false, message);
    } catch (const std::invalid_argument&) {
        // Expected.
    } catch (...) {
        expect(false, message);
    }
}

void test_price_time_priority() {
    lob::OrderBook book;
    book.add(1, lob::Side::sell, 10'002, 5);
    book.add(2, lob::Side::sell, 10'001, 3);
    book.add(3, lob::Side::sell, 10'001, 4);

    const auto trades = book.execute_market(lob::Side::buy, 6);
    expect(trades.size() == 2, "market order should create two fills");
    expect(trades[0].resting_order_id == 2, "best price should execute first");
    expect(trades[0].quantity == 3, "first resting order should be fully filled");
    expect(trades[1].resting_order_id == 3, "FIFO should hold within a price level");
    expect(trades[1].quantity == 3, "second resting order should be partially filled");
    expect(book.quantity_at(lob::Side::sell, 10'001) == 1, "one unit should remain");
    expect(book.contains(3), "partially filled order should stay indexed");
}

void test_cancel_and_top_of_book() {
    lob::OrderBook book;
    book.add(10, lob::Side::buy, 9'999, 7);
    book.add(11, lob::Side::buy, 10'000, 2);
    book.add(12, lob::Side::sell, 10'003, 4);

    auto top = book.top();
    expect(top.bid_price == 10'000, "highest bid should be best");
    expect(top.ask_price == 10'003, "lowest ask should be best");
    expect(book.cancel(11), "known order should be cancelled");
    expect(!book.cancel(11), "cancel should be idempotent for an unknown id");

    top = book.top();
    expect(top.bid_price == 9'999, "next price should become best after cancel");
    expect(book.order_count() == 2, "index size should match live orders");
}

void test_market_order_can_exceed_depth() {
    lob::OrderBook book;
    book.add(20, lob::Side::buy, 10'000, 4);
    const auto trades = book.execute_market(lob::Side::sell, 10);

    expect(trades.size() == 1, "available depth should execute once");
    expect(trades[0].quantity == 4, "fill should be capped by available depth");
    expect(book.order_count() == 0, "fully consumed book should be empty");
    expect(!book.top().bid_price.has_value(), "empty side should have no best price");
}

void test_input_validation() {
    lob::OrderBook book;
    expect_invalid_argument(
        [&] { book.add(0, lob::Side::buy, 10'000, 1); },
        "zero id should be rejected"
    );
    expect_invalid_argument(
        [&] { book.add(1, lob::Side::buy, 0, 1); },
        "zero price should be rejected"
    );
    expect_invalid_argument(
        [&] { book.add(1, lob::Side::buy, 10'000, 0); },
        "zero quantity should be rejected"
    );

    book.add(1, lob::Side::buy, 10'000, 1);
    expect_invalid_argument(
        [&] { book.add(1, lob::Side::sell, 10'001, 1); },
        "duplicate id should be rejected"
    );
    expect_invalid_argument(
        [&] { static_cast<void>(book.execute_market(lob::Side::sell, 0)); },
        "zero market quantity should be rejected"
    );
}

}  // namespace

int main() {
    test_price_time_priority();
    test_cancel_and_top_of_book();
    test_market_order_can_exceed_depth();
    test_input_validation();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All order book tests passed\n";
    return 0;
}
