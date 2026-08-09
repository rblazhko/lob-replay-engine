#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace lob {

using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint64_t;

enum class Side { buy, sell };

struct Order {
    OrderId id;
    Side side;
    Price price;
    Quantity remaining;
};

struct Trade {
    OrderId resting_order_id;
    Side aggressor_side;
    Price price;
    Quantity quantity;
};

struct TopOfBook {
    std::optional<Price> bid_price;
    Quantity bid_quantity{0};
    std::optional<Price> ask_price;
    Quantity ask_quantity{0};
};

class OrderBook {
public:
    void add(OrderId id, Side side, Price price, Quantity quantity);
    [[nodiscard]] bool cancel(OrderId id);
    [[nodiscard]] std::vector<Trade> execute_market(Side aggressor_side, Quantity quantity);

    [[nodiscard]] bool contains(OrderId id) const;
    [[nodiscard]] std::size_t order_count() const noexcept;
    [[nodiscard]] Quantity quantity_at(Side side, Price price) const;
    [[nodiscard]] TopOfBook top() const;

private:
    using OrderQueue = std::list<Order>;
    using BidLevels = std::map<Price, OrderQueue, std::greater<Price>>;
    using AskLevels = std::map<Price, OrderQueue, std::less<Price>>;

    struct Location {
        Side side;
        Price price;
        OrderQueue::iterator order;
    };

    BidLevels bids_;
    AskLevels asks_;
    std::unordered_map<OrderId, Location> locations_;

    template <typename Levels>
    std::vector<Trade> consume(Levels& levels, Side aggressor_side, Quantity quantity);
};

[[nodiscard]] const char* to_string(Side side) noexcept;

}  // namespace lob
