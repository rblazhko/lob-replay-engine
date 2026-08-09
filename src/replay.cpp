#include "lob/order_book.hpp"

#include <charconv>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::vector<std::string_view> split_csv(std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t begin = 0;
    while (begin <= line.size()) {
        const auto comma = line.find(',', begin);
        if (comma == std::string_view::npos) {
            fields.push_back(line.substr(begin));
            break;
        }
        fields.push_back(line.substr(begin, comma - begin));
        begin = comma + 1;
    }
    return fields;
}

template <typename Integer>
Integer parse_integer(std::string_view text, std::string_view field_name) {
    Integer value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw std::invalid_argument("invalid " + std::string(field_name));
    }
    return value;
}

lob::Side parse_side(std::string_view text) {
    if (text == "BUY") {
        return lob::Side::buy;
    }
    if (text == "SELL") {
        return lob::Side::sell;
    }
    throw std::invalid_argument("side must be BUY or SELL");
}

void print_optional(std::optional<lob::Price> value) {
    if (value) {
        std::cout << *value;
    }
}

void replay(std::istream& input) {
    lob::OrderBook book;
    std::string line;
    std::size_t line_number = 0;

    std::cout << "timestamp_ns,bid_price,bid_quantity,ask_price,ask_quantity,"
                 "trade_count,executed_quantity\n";

    while (std::getline(input, line)) {
        ++line_number;
        if (line.empty() || line.front() == '#') {
            continue;
        }
        if (line_number == 1 && line.starts_with("timestamp_ns,")) {
            continue;
        }

        try {
            const auto fields = split_csv(line);
            if (fields.size() != 6) {
                throw std::invalid_argument("expected six CSV fields");
            }

            const auto timestamp = parse_integer<std::uint64_t>(fields[0], "timestamp");
            const auto event = fields[1];
            std::vector<lob::Trade> trades;

            if (event == "ADD") {
                book.add(
                    parse_integer<lob::OrderId>(fields[2], "order id"),
                    parse_side(fields[3]),
                    parse_integer<lob::Price>(fields[4], "price"),
                    parse_integer<lob::Quantity>(fields[5], "quantity")
                );
            } else if (event == "CANCEL") {
                const auto id = parse_integer<lob::OrderId>(fields[2], "order id");
                if (!book.cancel(id)) {
                    throw std::invalid_argument("cannot cancel an unknown order id");
                }
            } else if (event == "MARKET") {
                trades = book.execute_market(
                    parse_side(fields[3]),
                    parse_integer<lob::Quantity>(fields[5], "quantity")
                );
            } else {
                throw std::invalid_argument("event must be ADD, CANCEL, or MARKET");
            }

            const auto executed = std::accumulate(
                trades.begin(),
                trades.end(),
                lob::Quantity{0},
                [](lob::Quantity total, const lob::Trade& trade) {
                    return total + trade.quantity;
                }
            );
            const auto top = book.top();

            std::cout << timestamp << ',';
            print_optional(top.bid_price);
            std::cout << ',' << top.bid_quantity << ',';
            print_optional(top.ask_price);
            std::cout << ',' << top.ask_quantity << ',' << trades.size() << ',' << executed << '\n';
        } catch (const std::exception& error) {
            throw std::runtime_error(
                "line " + std::to_string(line_number) + ": " + error.what()
            );
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            replay(std::cin);
            return 0;
        }
        if (argc != 2) {
            std::cerr << "usage: lob_replay [events.csv]\n";
            return 2;
        }

        std::ifstream input(argv[1]);
        if (!input) {
            throw std::runtime_error("cannot open input file");
        }
        replay(input);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
