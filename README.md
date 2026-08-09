# LOB Replay Engine

A compact C++20 limit order book for deterministic market-event replay. It implements price-time priority, partial fills, cancellation by order ID, and top-of-book snapshots without external dependencies.

The project is deliberately small: the matching rules fit in one source file, the data model is explicit, and the tests focus on invariants that are easy to get subtly wrong.

## What it demonstrates

- Price-time priority on both sides of the book
- Integer prices and quantities: no floating-point money
- O(1) order lookup and cancellation through stored list iterators
- Deterministic CSV replay with line-level validation errors
- A dependency-free test executable and strict compiler warnings

## Build and run

Requirements: a C++20 compiler and `make`.

```bash
make test
make demo
make benchmark
```

Example replay output:

```text
timestamp_ns,bid_price,bid_quantity,ask_price,ask_quantity,trade_count,executed_quantity
100,10000,10,,0,0,0
110,10000,15,,0,0,0
120,10000,15,10002,8,0,0
130,10000,15,10001,6,0,0
140,10000,15,10002,4,2,10
150,10000,10,10002,4,0,0
160,10000,3,10002,4,1,7
```

The executable also accepts events from standard input:

```bash
./build/lob_replay < data/sample_events.csv
```

## Event format

```text
timestamp_ns,event,order_id,side,price,quantity
```

Supported events:

- `ADD`: all fields are required.
- `CANCEL`: `order_id` is required; unused fields may be empty.
- `MARKET`: `side` is the aggressor side and `quantity` is required.

Prices are stored in integer ticks. For example, `10001` may represent `100.01` when the instrument tick size is one cent.

## Design notes

Each price level owns a linked FIFO queue. An index maps every live order ID to its queue iterator, so cancellation does not scan the book. Bids use descending price order; asks use ascending price order. Market orders repeatedly consume the first order at the best opposite price.

See [docs/design.md](docs/design.md) for complexity, invariants, and deliberate limitations.

## Repository layout

```text
include/lob/order_book.hpp  public API and data types
src/order_book.cpp          matching and book state
src/replay.cpp              strict CSV replay CLI
src/benchmark.cpp           deterministic throughput smoke benchmark
tests/order_book_test.cpp   dependency-free unit tests
data/sample_events.csv      reproducible example
```

## Scope

This is a reference engine, not a production exchange. It does not model network I/O, persistence, self-trade prevention, hidden liquidity, auction states, or exchange-specific order types. Incoming limit orders are treated as already-resting exchange events; aggressive flow is represented explicitly by `MARKET` events.

The benchmark is a local regression aid, not a cross-machine performance claim. It reports the workload and measured throughput so results can be compared only under controlled compiler and hardware conditions.

## License

MIT
