#pragma once
#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "OrderTypes.h"
#include "Listeners.h"
#include "OrderTracker.h"
#include <atomic>

namespace OrderEngine{

    /**
     * @brief Structure representing a trade execution event.
     * @param OrderPtr Smart pointer type for Order objects (e.g., std::shared_ptr<Order>).
     * @details
     * TradeExecution encapsulates details of a trade execution. This is official record of when
     * two orders match and trade. This can be thought of as the “birth certificate” of every trade.
     */
    template<typename OrderPtr> struct TradeExecution {
        OrderPtr inbound_order; // The "aggressive" order that just arrived (initiator of the trade)
        OrderPtr resting_order; // The "passive" order that was already in the book (the one being hit or lifted)
        Quantity quantity; // 
        Price price;
        Timestamp timestamp;
        FillFlags flags;
        
        TradeExecution(const OrderPtr& inbound, const OrderPtr& resting, 
                    Quantity qty, Price p, FillFlags f = FILL_NORMAL)
            : inbound_order(inbound), resting_order(resting), quantity(qty), 
            price(p), timestamp(std::chrono::high_resolution_clock::now()), flags(f) {}
    };
    
    /**
     * @brief Structure for tracking order book statistics.
     * @details
     * OrderBookStats maintains atomic counters for various order book events, allowing
     * for thread-safe tracking of metrics such as total orders added, cancelled, replaced,
     * trades executed, volume traded, and orders rejected.
     */
    struct OrderBookStats {
        std::atomic<uint64_t> total_orders_added{0};
        std::atomic<uint64_t> total_orders_cancelled{0};
        std::atomic<uint64_t> total_orders_replaced{0};
        std::atomic<uint64_t> total_trades{0};
        std::atomic<uint64_t> total_volume{0};
        std::atomic<uint64_t> total_rejected{0};
        
        void reset() {
            total_orders_added = 0;
            total_orders_cancelled = 0;
            total_orders_replaced = 0;
            total_trades = 0;
            total_volume = 0;
            total_rejected = 0;
        }
    };

} // namespace OrderEngine

#endif // ORDER_BOOK_H