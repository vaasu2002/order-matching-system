#pragma once
#ifndef DEPTH_TRACKER_H
#define DEPTH_TRACKER_H

#include "OrderTypes.h"
#include "OrderTracker.h"
#include <sstream>
#include <iomanip>

namespace OrderEngine {

    /**
     * @brief Represents a single level of market depth.
     * 
     * DepthLevel encapsulates the price, total quantity, and order count
     * at a specific price point in the order book. It provides utility
     * functions for comparison and display.
     */
    struct DepthLevel {
        Price price;
        Quantity quantity;
        size_t order_count;
        
        DepthLevel() : price(0), quantity(0), order_count(0) {}
        
        DepthLevel(Price p, Quantity q, size_t count) 
            : price(p), quantity(q), order_count(count) {}
        
        bool empty() const { 
            return quantity == 0; 
        }
        
        void clear() { 
            price = 0; 
            quantity = 0; 
            order_count = 0; 
        }
        
        bool operator==(const DepthLevel& other) const {
            return price == other.price && 
                quantity == other.quantity && 
                order_count == other.order_count;
        }
        
        bool operator!=(const DepthLevel& other) const {
            return !(*this == other);
        }
        
        std::string ToString() const {
            std::ostringstream oss;
            oss << "$" << std::fixed << std::setprecision(2) << (price / 100.0) 
                << " | " << quantity << " shares | " << order_count << " orders";
            return oss.str();
        }
    };

    
} // namespace OrderEngine

#endif // DEPTH_TRACKER_H