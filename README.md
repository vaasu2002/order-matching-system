# Order Matching Engine
Order matching engine is responsible for instantly pair buyers and sellers for a specific asset by matching buy and sell orders according to predefined rules.


## Entities
1) **PriceLevel:** Represents a single price point in the order book, holding all active orders placed at that price. It tracks individual orders as well as aggregated statistics like total quantity and order count.
2) **OrderTracker:** Manages one side of the order book (all buy orders or all sell orders) by organizing orders into price levels. It maintains fast lookups, price priority, and provides access to the best available price levels.
