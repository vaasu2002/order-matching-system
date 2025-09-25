#pragma once
#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

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
}

#endif // ORDER_BOOK_H