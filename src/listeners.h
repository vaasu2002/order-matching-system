#pragma once
#ifndef LISTENERS_H
#define LISTENERS_H

#include "OrderTypes.h"
#include <memory>

namespace OrderEngine {
    class Order;
    // Forward declarations
    struct Trade;
    struct DepthLevel;

    /**
     * @brief Interface for listening to order lifecycle events.
     * @param OrderPtr Smart pointer type for Order objects (e.g., std::shared_ptr<Order>).
     * @details
     * OrderListener is Event listener interface, it is based on observer design pattern.
     * It is nervous system of Order matching engine.
     */
    template<typename OrderPtr> class OrderListener {
    public:
        virtual ~OrderListener() = default;
        
        // Order lifecycle events
        virtual void on_accept(const OrderPtr& order) {}
        virtual void on_reject(const OrderPtr& order, const std::string& reason) {}
        virtual void on_fill(const OrderPtr& order, const OrderPtr& matched_order, 
                            Quantity fill_qty, Price fill_price) {}
        virtual void on_cancel(const OrderPtr& order, Quantity cancelled_qty) {}
        virtual void on_replace(const OrderPtr& old_order, const OrderPtr& new_order) {}
        virtual void on_replace_reject(const OrderPtr& order, const std::string& reason) {}
    };

    // Trade event listener interface
    template<typename OrderPtr> class TradeListener {
    public:
        virtual ~TradeListener() = default;
        
        virtual void on_trade(const OrderPtr& inbound_order, const OrderPtr& matched_order,
                            Quantity quantity, Price price, bool inbound_order_filled,
                            bool matched_order_filled) = 0;
    };

    // Order book event listener interface
    template<typename OrderBookType> class OrderBookListener {
    public:
        virtual ~OrderBookListener() = default;
        
        virtual void on_order_book_change(const OrderBookType* book) {}
        virtual void on_bbo_change(const OrderBookType* book, Price bid, Price ask) {}
    };

    // Depth book event listener interface  
    template<typename OrderBookType> class DepthListener {
    public:
        virtual ~DepthListener() = default;
        
        virtual void on_depth_change(const OrderBookType* book, bool is_bid, 
                                Price price, Quantity new_qty, Quantity delta) = 0;
    };

} // namespace OrderEngine

#endif // LISTENERS_H