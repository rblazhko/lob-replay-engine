# Design notes

## Data structures

The two sides of the book use ordered maps:

- bids: descending price to FIFO order queue;
- asks: ascending price to FIFO order queue.

Orders at the same price live in `std::list`, which preserves insertion order and keeps iterators stable. A hash index maps an order ID to `{side, price, iterator}`.

## Complexity

Let `P` be the number of occupied price levels and `F` the number of fills produced by a market order.

| Operation | Complexity |
| --- | --- |
| Add order | `O(log P)` |
| Cancel order | `O(log P)` for level lookup, `O(1)` queue erase |
| Best bid / ask | `O(1)` |
| Market order | `O(F + L log P)`, where `L` levels become empty |

Aggregated quantity at the best level is currently computed by walking that level. A production version would maintain cached level totals.

## Invariants

The tests exercise four core invariants:

1. the best price executes before worse prices;
2. FIFO order is preserved within a price level;
3. partially filled orders remain cancellable through the ID index;
4. empty price levels and fully filled orders disappear from every index.

## Deliberate trade-offs

- Integer ticks avoid floating-point comparison and rounding issues.
- Duplicate IDs and non-positive inputs fail early with exceptions.
- An unknown cancellation is reported as a replay error rather than silently ignored.
- The CSV parser is intentionally narrow and does not implement quoted fields because the event schema is numeric and enumerated.
- Thread safety is outside the scope of the reference engine. A real feed handler would define ownership and concurrency at a higher level.
