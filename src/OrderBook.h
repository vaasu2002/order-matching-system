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

    /**
     * @brief The OrderBook class manages buy and sell orders, matches trades, and notifies listeners of events.
     * @remarks 
     * The architecture design decision is to keep one order book instance per stock
     * 1. Stocks Are Independent
     * 2. Provides performance isolation
     * 3. Circuit breakers can be implemented per stock
     * 
     */
    template<typename OrderPtr> class OrderBook{

        public:
        using OrderTracker = OrderEngine::OrderTracker<OrderPtr>;
        using DepthTracker = OrderEngine::DepthTracker<10>; // 10 levels of depth
        using TradeExecution = OrderEngine::TradeExecution<OrderPtr>;
        using OrderListenerPtr = std::shared_ptr<OrderListener<OrderPtr>>;
        using TradeListenerPtr = std::shared_ptr<TradeListener<OrderPtr>>;
        using OrderBookListenerPtr = std::shared_ptr<OrderBookListener<OrderBook<OrderPtr>>>;
        using DepthListenerPtr = std::shared_ptr<DepthListener<OrderBook<OrderPtr>>>;
        
        private:
        Symbol mSymbol;
        OrderTracker mBidTracker;     // Manages all buy orders
        OrderTracker mAskTracker;     // Manages all sell orders
        OrderTracker mStopBidTracker; // Manages all stop buy orders
        OrderTracker mStopAskTracker; // Manages all stop sell orders
        DepthTracker mDepthTracker;   // Manages order book depth

        // Market state
        std::atomic<Price> mMarketPrice;
        std::atomic<Price> mLastTradePrice;
        std::atomic<Quantity> mLastTradeQuantity;

        // Event listeners
        std::vector<OrderListenerPtr> mOrderListeners;
        std::vector<TradeListenerPtr> mTradeListeners;
        std::vector<OrderBookListenerPtr> mBookListeners;
        std::vector<DepthListenerPtr> mDepthListeners;

        // Statistics
        OrderBookStats mStats;

        // Thread safety
        mutable std::recursive_mutex mBookMutex;

        // Trade execution queue for batch processing
        std::vector<TradeExecution> mPendingTrades;

        public:
        explicit OrderBook(const Symbol& symbol) : mSymbol(symbol), 
            mBidTracker(true),   
            mAskTracker(false),   
            mStopBidTracker(true),
            mStopAskTracker(false),
            mMarketPrice(0),
            mLastTradePrice(0),
            mLastTradeQuantity(0){
                mPendingTrades.reserve(1000); 
        }
        
        ~OrderBook() = default;

        // ========== Configuration ==========

        void setmarketprice(Price price) {
            mMarketPrice.store(price);
            check_stop_orders();
        }

        // ========== Listener Management ==========

        void addOrderListener(OrderListenerPtr listener) {
            std::lock_guard<std::recursive_mutex> lock(mBookMutex);
            mOrderListeners.push_back(listener);
        }
        
        void addTradeListener(TradeListenerPtr listener) {
            std::lock_guard<std::recursive_mutex> lock(mBookMutex);
            mTradeListeners.push_back(listener);
        }
    
        void addBookListener(OrderBookListenerPtr listener) {
            std::lock_guard<std::recursive_mutex> lock(mBookMutex);
            mBookListeners.push_back(listener);
        }
    
        void addDepthListener(DepthListenerPtr listener) {
            std::lock_guard<std::recursive_mutex> lock(mBookMutex);
            mDepthListeners.push_back(listener);
        }

        // ========== Core Order Operations ==========

        /**
         * @brief Add a new order to the order book.
         * @param order The order to be added.
         * @param conditions Special conditions for order execution (default is NO_CONDITIONS).
         * @details
         * - Validates the order parameters.
         * @todo 
         * - Implement handling for different order types (limit, stop, etc.).
         * - Implement notifications to listeners for order acceptance.
         * - Update market data and depth after adding the order.
         * @return True if the order was successfully added, false otherwise.
         */
        bool addOrder(const OrderPtr& order, OrderConditions conditions = NO_CONDITIONS){
            
            std::lock_guard<std::recursive_mutex> lock(mBookMutex); // acquire lock
            
            if (!validateOrder(order)) {
                rejectOrder(order, "Invalid order parameters");
                return false;
            }
            
            bool filled = false;
            
            if(order->is_market()){
                filled = processMarketOrder(order, conditions);
            } 
            // todo: add order processing for stop order and limit order
            // todo: add notification that order is accepted
            // todo: update market data and depth
            return filled;
        }

        private:
        
        // ========== Event Notifications ==========

        /**
         * @brief Method to handle rejection of order
         */
        void rejectOrder(const OrderPtr& order, const std::string& reason) {
            order->set_status(OrderStatus::REJECTED);
            mStats.total_rejected++;
            for (const auto& listener : mOrderListeners) {
                listener->on_reject(order, reason);
            }
            //todo: add warn log
        }

        // ========== Validation ==========

        /**
         * @brief Method to handle order validation
         * @todo Convert this to chain of responsibility, currently there is 
         * OCP violation and DRY violation.
         */
        bool validateOrder(const OrderPtr& order) const{
            if(!order) return false;
            if(order->symbol() != mSymbol) return false;
            if(order->quantity() == 0) return false;
            if(order->open_quantity() > order->quantity()) return false;
            if(!order->is_market() && order->price() <= 0) return false;
            if(order->is_stop() && order->stop_price() <= 0) return false;
            return true;
        }

        // ========== Order Processing ==========

        bool processMarketOrder(const OrderPtr& inBoundorderPtr, OrderConditions conditions){
            bool filled = false;
            if(inBoundorderPtr->is_buy()){
                filled = matchMarketBuyOrder(inBoundorderPtr, conditions);
            }
            else {
                // todo: implement matchMarketSellOrder
            }

            if (inBoundorderPtr->open_quantity() > 0) {
                inBoundorderPtr->set_status(OrderStatus::CANCELLED);
                // todo: notifyOrderCancelled(inBoundorderPtr, inBoundorderPtr->open_quantity()); mOrderListeners
            }
            return filled;
        }
        
        /**
         * @brief Match a market buy order against the order book.
         * @param order The incoming market buy order.
         * @param conditions Special conditions for order execution.
         * @details
         * In the market buy order, the buyer is willing to pay any price to get the shares immediately.
         * So we match against the lowest ask prices available in the order book.
         */
        bool matchMarketBuyOrder(const OrderPtr& order, OrderConditions conditions){
            Price limitPrice = std::numeric_limits<Price>::max(); // No price limit for market orders
            return matchBuyOrder(order,conditions,limitPrice);
        }

        /**
         * @brief Match a buy order against the order book.
         * @param order The incoming buy order.
         * @param conditions Special conditions for order execution.
         * @param limitPrice The maximum price the buyer is willing to pay. (decided by order type)
         */
        bool matchBuyOrder(const OrderPtr& inBoundOrderPtr, OrderConditions conditions, Price limitPrice) {

            Quantity inBoundOrderRemaining = inBoundOrderPtr->open_quantity();
            bool any_fill = false;

            // Get matching sell(ask) orders from the ask-order tracker, format: std::vector<std::pair<OrderPtr, Quantity>>
            // These are order lying in sell section of order booking waiting to be matched with buy orders
            // These are resting order (orders lying in order book to be matched)
            auto matches = mAskTracker.matchQuantity(limitPrice, inBoundOrderRemaining); 
            
            for (const auto& [restingOrderPtr, restingOrderRemainingQty] : matches) {

                if (inBoundOrderRemaining == 0){
                    break;
                }
                
                // Check all-or-none conditions
                if (IsAllOrNone(conditions) && restingOrderRemainingQty < inBoundOrderRemaining) {
                    continue;
                }
                
                Quantity fillQty = std::min(restingOrderRemainingQty, inBoundOrderRemaining);
                Price fillPrice = restingOrderPtr->price();
                
                // Execute the trade
                executeTrade(inBoundOrderPtr, restingOrderPtr, fillQty, fillPrice);
                
                inBoundOrderRemaining -= fillQty;
                any_fill = true;
                
                // Update order quantities
                inBoundOrderPtr->set_open_quantity(inBoundOrderRemaining);
                
                if (inBoundOrderRemaining == 0) {
                    inBoundOrderPtr->set_status(OrderStatus::FILLED);
                    break;
                } else {
                    inBoundOrderPtr->set_status(OrderStatus::PARTIALLY_FILLED);
                }
            }
            
            return any_fill;
        }

        /**
         * @brief Execute a trade between an inbound order and a resting order.
         * @param inBoundOrderPtr The incoming order that initiated the trade.
         * @param restingOrderPtr The existing order in the book that is being matched.
         * @param quantity The quantity of shares being traded.
         * @param price The price at which the trade is executed.
         * @details
         * - Creates a TradeExecution record
         * - Updates order statuses and quantities
         * - Notifies listeners of the trade event
         */
        void executeTrade(const OrderPtr& inBoundOrderPtr, const OrderPtr& restingOrderPtr, 
                            Quantity quantity, Price price) {
        
            FillFlags flags = FILL_NORMAL;
            if (inBoundOrderPtr->open_quantity() == quantity){
                flags |= FILL_COMPLETE;
            }
            else{
                flags |= FILL_PARTIAL;
            }

            // Create trade execution record
            TradeExecution trade(inBoundOrderPtr, restingOrderPtr, quantity, price, flags);
            mPendingTrades.push_back(trade);
                     
            // ==== Updating Meta Data ==== 

            // Update statistics
            mStats.total_trades++;
            mStats.total_volume += quantity;
            // Update market price
            mLastTradePrice.store(price);
            mLastTradeQuantity.store(quantity);
            mMarketPrice.store(price);
        
            // Update resting order
            Quantity restingRemainingQty = restingOrderPtr->open_quantity() - quantity;
            restingOrderPtr->set_open_quantity(restingRemainingQty);
            
            if (restingRemainingQty == 0) {
                restingOrderPtr->set_status(OrderStatus::FILLED);
            } else {
                restingOrderPtr->set_status(OrderStatus::PARTIALLY_FILLED);
            }
            
            // todo: log the trade
            // todo: notify trade listeners that trade is executed
        }

        // ========== Utility Functions ==========

        /**
         * @todo Seperate utility functions
         */

        bool IsAllOrNone(OrderConditions conditions) const {
            return (conditions & ALL_OR_NONE) == 1;
        }
        
        bool isImmediateOrCancel(OrderConditions conditions) const {
            return (conditions & IMMEDIATE_OR_CANCEL) == 1;
        }
    };

} // namespace OrderEngine

#endif // ORDER_BOOK_H