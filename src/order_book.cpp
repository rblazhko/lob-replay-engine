#include "lob/order_book.hpp"

#include <algorithm>
#include <stdexcept>

namespace lob {

void OrderBook::add(OrderId id, Side side, Price price, Quantity quantity) {
    if (id == 0) {
        throw std::invalid_argument("order id must be positive");
    }
    if (price <= 0) {
        throw std::invalid_argument("price must be positive");
    }
    if (quantity == 0) {
        throw std::invalid_argument("quantity must be positive");
    }
    if (locations_.contains(id)) {
        throw std::invalid_argument("duplicate order id");
    }

    if (side == Side::buy) {
        auto& queue = bids_[price];
        queue.push_back(Order{id, side, price, quantity});
        locations_.emplace(id, Location{side, price, std::prev(queue.end())});
        return;
    }

    auto& queue = asks_[price];
    queue.push_back(Order{id, side, price, quantity});
    locations_.emplace(id, Location{side, price, std::prev(queue.end())});
}

bool OrderBook::cancel(OrderId id) {
    const auto location = locations_.find(id);
    if (location == locations_.end()) {
        return false;
    }

    const auto [side, price, order] = location->second;
    if (side == Side::buy) {
        auto level = bids_.find(price);
        level->second.erase(order);
        if (level->second.empty()) {
            bids_.erase(level);
        }
    } else {
        auto level = asks_.find(price);
        level->second.erase(order);
        if (level->second.empty()) {
            asks_.erase(level);
        }
    }

    locations_.erase(location);
    return true;
}

std::vector<Trade> OrderBook::execute_market(Side aggressor_side, Quantity quantity) {
    if (quantity == 0) {
        throw std::invalid_argument("market order quantity must be positive");
    }
    if (aggressor_side == Side::buy) {
        return consume(asks_, aggressor_side, quantity);
    }
    return consume(bids_, aggressor_side, quantity);
}

bool OrderBook::contains(OrderId id) const {
    return locations_.contains(id);
}

std::size_t OrderBook::order_count() const noexcept {
    return locations_.size();
}

Quantity OrderBook::quantity_at(Side side, Price price) const {
    const auto sum_level = [](const auto& level) {
        Quantity total = 0;
        for (const auto& order : level) {
            total += order.remaining;
        }
        return total;
    };

    if (side == Side::buy) {
        const auto level = bids_.find(price);
        return level == bids_.end() ? 0 : sum_level(level->second);
    }
    const auto level = asks_.find(price);
    return level == asks_.end() ? 0 : sum_level(level->second);
}

TopOfBook OrderBook::top() const {
    TopOfBook result;
    if (!bids_.empty()) {
        result.bid_price = bids_.begin()->first;
        result.bid_quantity = quantity_at(Side::buy, *result.bid_price);
    }
    if (!asks_.empty()) {
        result.ask_price = asks_.begin()->first;
        result.ask_quantity = quantity_at(Side::sell, *result.ask_price);
    }
    return result;
}

template <typename Levels>
std::vector<Trade> OrderBook::consume(
    Levels& levels,
    Side aggressor_side,
    Quantity quantity
) {
    std::vector<Trade> trades;
    while (quantity > 0 && !levels.empty()) {
        auto best_level = levels.begin();
        auto& queue = best_level->second;
        auto& resting = queue.front();
        const Quantity executed = std::min(quantity, resting.remaining);

        trades.push_back(Trade{
            resting.id,
            aggressor_side,
            resting.price,
            executed,
        });

        quantity -= executed;
        resting.remaining -= executed;
        if (resting.remaining == 0) {
            locations_.erase(resting.id);
            queue.pop_front();
        }
        if (queue.empty()) {
            levels.erase(best_level);
        }
    }
    return trades;
}

const char* to_string(Side side) noexcept {
    return side == Side::buy ? "BUY" : "SELL";
}

}  // namespace lob
