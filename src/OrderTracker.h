#pragma once
#ifndef ORDER_TRACKER_H
#define ORDER_TRACKER_H

#include "Order.h"
#include "OrderTypes.h"
#include <map>
#include <list>
#include <vector>
#include <memory>
#include <algorithm>
// DOUBTS: Where to use PriceComparator?
namespace OrderEngine {

    // Forward declaration
    template<typename OrderPtr> class OrderTracker;

    /**
    * @brief Represents a single price point in the order book.  
    * 
    * A PriceLevel groups all active orders submitted at the same price.
    * It maintains both the list of orders (FIFO by entry time) and 
    * aggregate statistics like total open quantity and order count.  
    * Provides efficient operations for adding, removing, updating, 
    * and filling orders at this price.
    * Think of an order book like a building with floors, where each floor represents a different price
    * BUY SIDE:
    * ┌─────────────┬──────────────────────────────────────┬─────────────┐
    * │ Price Level │ Orders at This Price                 │ Total Qty   │
    * ├─────────────┼──────────────────────────────────────┼─────────────┤
    * │ $150.00     │ [John:1000] [Sarah:500] [Mike:800]   │ 2,300       │
    * │ $149.50     │ [Lisa:200] [Tom:600]                 │ 800         │
    * └─────────────┴──────────────────────────────────────┴─────────────┘
    */
    template<typename OrderPtr> class PriceLevel {
    public:
        using OrderList = std::vector<OrderPtr>;
        using OrderIterator = typename OrderList::iterator;
    private:
        Price price_; // $150.00 
        OrderList orders_; // [John, Sarah, Mike]
        Quantity total_quantity_; // 2,300 shares total
        size_t order_count_; // 3 orders at this price
        
    public:
        explicit PriceLevel(Price price) 
            : price_(price), total_quantity_(0), order_count_(0) {}
        
        // Accessors
        Price price() const { return price_; }
        Quantity total_quantity() const { return total_quantity_; }
        size_t order_count() const { return order_count_; }
        bool empty() const { return orders_.empty(); }
        const OrderList& orders() const { return orders_; }
        
        /**
        * @brief Adds a new order to the list of tracked orders.
        * @param order: Smart pointer to the order being added. 
        * @details
        * Updates aggregate statistics (total open quantity and order count)
        * and appends the order to the internal container.
        * @return Iterator pointing to the newly inserted order.
        */
        OrderIterator add_order(const OrderPtr& order) {
            total_quantity_ += order->open_quantity();
            order_count_++;
            return orders_.insert(orders_.end(), order);
        }
        
        void remove_order(OrderIterator it) {
            total_quantity_ -= (*it)->open_quantity();
            --order_count_;
            orders_.erase(it);
        }
        
        void update_quantity(const OrderPtr& order, Quantity old_qty, Quantity new_qty) {
            total_quantity_+=(new_qty- old_qty); // O(1)
        }
        
        // Get the first order (FIFO)
        OrderPtr front_order() const {
            return orders_.empty() ? OrderPtr{} : orders_.front();
        }
        
        // Fill orders at this price level up to specified quantity
        // This takes buy order and tries to fill it by matching against sell orders. It follows FIFO rule,
        // Earlier order gets filled first.
        // Main order matching logic
        Quantity fill_quantity(Quantity max_quantity) {
            Quantity filled = 0; // track how much we've filled so far
            auto it = orders_.begin(); // get first order 
            
            while (it != orders_.end() && filled < max_quantity) {
                auto order = *it;
                Quantity available = order->open_quantity(); // shares available
                Quantity fill_qty = std::min(available, max_quantity - filled); // how many we can fill from current order
                order->set_open_quantity(available - fill_qty); // reduce open quantity
                filled += fill_qty;
                total_quantity_ -= fill_qty;
                
                if (order->open_quantity() == 0) {
                    // Order completely filled, remove it
                    order->set_status(OrderStatus::FILLED);
                    it = orders_.erase(it);
                    --order_count_;
                } 
                else {
                    order->set_status(OrderStatus::PARTIALLY_FILLED);
                    it++;
                }
            }
            return filled;
        }
    };

    
    /**
    * @brief Manages one side of the order book (all buys or all sells).  
    *
    * Organizes orders into PriceLevels keyed by price, ensuring proper
    * price-time priority. Tracks order locations for fast access and 
    * updates, supports order matching against incoming trades, and 
    * provides quick access to the best price levels.  
    */
    template<typename OrderPtr> class OrderTracker {
    public:
        using PriceLevelPtr = std::shared_ptr<PriceLevel<OrderPtr>>;
        using PriceLevelMap = std::map<Price, PriceLevelPtr>;
        // Cache for efficient order lookups
        using OrderLocationMap = std::map<OrderId, std::pair<Price, typename PriceLevel<OrderPtr>::OrderIterator>>;
        
    private:
        /**
            price_levels_[15100] = PriceLevel containing [Order A, Order B, Order C]  // $151.00
            price_levels_[15050] = PriceLevel containing [Order D, Order E]           // $150.50  
            price_levels_[15000] = PriceLevel containing [Order F]                    // $150.00
        */
        PriceLevelMap price_levels_; 

        /**
            Location of order with some ID is the key.
            The value is a pair containing:
            1) The price level where the order resides (e.g., $150.00)
            2) An iterator pointing to the order within that price level's order list.
            order_locations_[12345] = {15000, iterator_to_order_F}  // Order 12345 is at $150.00
            order_locations_[67890] = {15050, iterator_to_order_D}  // Order 67890 is at $150.50
        */
        OrderLocationMap order_locations_; 

        bool is_buy_side_;
        
        // Custom comparator for price levels based on order side
        struct PriceComparator {
            bool is_buy_side;
            
            explicit PriceComparator(bool buy_side) : is_buy_side(buy_side) {}
            
            // BUY SIDE (Bids) : higher prices first (best bid at top)
            // SELL SIDE (Asks): lower prices first (best ask at top)
            bool operator()(Price a, Price b) const {
                return is_buy_side ? a > b : a < b;  
            }
        };
    public:
        explicit OrderTracker(bool is_buy_side) : is_buy_side_(is_buy_side) {}
        
        // Add order to tracker
        bool addOrder(const OrderPtr& order) {
            Price price = order->price();
            
            // Find or create price level
            auto level_it = price_levels_.find(price);
            if (level_it == price_levels_.end()) {
                auto new_level = std::make_shared<PriceLevel<OrderPtr>>(price);
                level_it = price_levels_.emplace(price, new_level).first;
            }
            
            // Add order to price level
            auto order_it = level_it->second->add_order(order);
            
            // Track order location for fast lookup
            order_locations_[order->order_id()] = std::make_pair(price, order_it);
            
            return true;
        }
        
        // Remove order from tracker
        bool remove_order(const OrderPtr& order) {
            auto location_it = order_locations_.find(order->order_id());
            if (location_it == order_locations_.end()) {
                return false; // Order not found
            }
            
            Price price = location_it->second.first;
            auto order_it = location_it->second.second;
            
            // Remove from price level
            auto level_it = price_levels_.find(price);
            if (level_it != price_levels_.end()) {
                level_it->second->remove_order(order_it);
                
                // Remove empty price level
                if (level_it->second->empty()) {
                    price_levels_.erase(level_it);
                }
            }
            else{
                // Order is not found, then why was it in order_locations_?
                // This indicates a data integrity issue, todo: Warning log
                return false;
            }
            
            // Remove from location tracking
            order_locations_.erase(location_it);
            return true;
        }
        
        // Update order quantity
        void update_order_quantity(const OrderPtr& order, Quantity new_qty) {
            auto location_it = order_locations_.find(order->order_id());
            if (location_it != order_locations_.end()) {
                Price price = location_it->second.first;
                auto level_it = price_levels_.find(price);
                if (level_it != price_levels_.end()) {
                    Quantity old_qty = order->open_quantity();
                    order->set_open_quantity(new_qty);
                    level_it->second->update_quantity(order, old_qty, new_qty);
                }
            }
        }
        
        // Get best price (top of book)
        Price best_price() const {
            if (price_levels_.empty()) return 0;
            return price_levels_.begin()->first;
        }
        
        // Get best price level
        PriceLevelPtr best_level() const {
            if (price_levels_.empty()) return nullptr;
            return price_levels_.begin()->second;
        }
        
        // Get price level at specific price
        PriceLevelPtr level_at_price(Price price) const {
            auto it = price_levels_.find(price);
            return (it != price_levels_.end()) ? it->second : nullptr;
        }
        
        // Check if order exists
        bool has_order(OrderId order_id) const {
            return order_locations_.find(order_id) != order_locations_.end();
        }
        
        // Get total quantity at price level
        Quantity quantity_at_price(Price price) const {
            auto level = level_at_price(price);
            return level ? level->total_quantity() : 0;
        }
        
        // Get all price levels (sorted by priority)
        const PriceLevelMap& price_levels() const { return price_levels_; }
        
        // Clear all orders
        void clear() {
            price_levels_.clear();
            order_locations_.clear();
        }
        
        // Statistics
        size_t total_orders() const { return order_locations_.size(); }
        size_t total_price_levels() const { return price_levels_.size(); }
        
        bool empty() const { return price_levels_.empty(); }
        
        // Match against incoming order (for crossing trades)
        std::vector<std::pair<OrderPtr, Quantity>> matchQuantity(Price limit_price, Quantity max_quantity) {
            std::vector<std::pair<OrderPtr, Quantity>> matches;
            Quantity remaining = max_quantity;
            
            auto it = price_levels_.begin();
            while (it != price_levels_.end() && remaining > 0) {
                Price level_price = it->first;
                
                // Check if this price level can match
                bool can_match = is_buy_side_ ? (level_price >= limit_price) : (level_price <= limit_price);
                if (!can_match) break;
                
                auto level = it->second;
                auto& orders = level->orders();
                auto order_it = orders.begin();
                
                while (order_it != orders.end() && remaining > 0) {
                    auto order = *order_it;
                    Quantity available = order->open_quantity();
                    Quantity match_qty = std::min(available, remaining);
                    
                    matches.emplace_back(order, match_qty);
                    remaining -= match_qty;
                    
                    ++order_it;
                }
                
                ++it;
            }
            
            return matches;
        }
    };

} // namespace OrderEngine

#endif // ORDER_TRACKER_H