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

    /**
     * @brief Tracks market depth up to a specified number of levels.
     * @details
     * DepthTracker maintains an aggregated view of the order book,
     * capturing the top N levels of bids and asks. It updates its state
     * based on provided OrderTracker instances for bids and asks, and
     * detects changes between updates. It provides various utility functions
     * to access depth information, calculate market quality metrics,
     * and format the depth for display.
     */
    template<size_t MAX_LEVELS = 10> class DepthTracker {
    public:
        using DepthArray = std::array<DepthLevel, MAX_LEVELS>;
        // Depth change information for listeners
        struct DepthChange {
            bool is_bid;
            size_t level;
            Price price;
            Quantity old_quantity;
            Quantity new_quantity;
            size_t old_order_count;
            size_t new_order_count;
            
            DepthChange(bool bid, size_t lvl, Price p, Quantity old_qty, Quantity new_qty,
                    size_t old_count, size_t new_count)
                : is_bid(bid), level(lvl), price(p), old_quantity(old_qty), 
                new_quantity(new_qty), old_order_count(old_count), new_order_count(new_count) {}
        };
        
    private:
        DepthArray bid_levels_;         // Buy side levels (highest to lowest price)
        DepthArray ask_levels_;         // Sell side levels (lowest to highest price)
        size_t bid_count_;              // Number of active bid levels
        size_t ask_count_;              // Number of active ask levels
        bool changed_;                  // Has depth changed since last check?
        std::vector<DepthChange> changes_; // Record of what changed
        
        // Previous state for change detection
        DepthArray prev_bid_levels_;
        DepthArray prev_ask_levels_;
        size_t prev_bid_count_;
        size_t prev_ask_count_;
        
    public:
        DepthTracker() : bid_count_(0), ask_count_(0), changed_(false), 
                        prev_bid_count_(0), prev_ask_count_(0) {
            // Initialize all levels as empty
            for (auto& level : bid_levels_) level.clear();
            for (auto& level : ask_levels_) level.clear();
            for (auto& level : prev_bid_levels_) level.clear();
            for (auto& level : prev_ask_levels_) level.clear();
        }
        
        // Update depth from order trackers
        template<typename OrderPtr>
        void update_from_tracker(const OrderTracker<OrderPtr>& bid_tracker,
                            const OrderTracker<OrderPtr>& ask_tracker) {
            // Save current state for change detection
            save_previous_state();
            
            // Clear change tracking
            changed_ = false;
            changes_.clear();
            
            // Update both sides
            update_bid_side(bid_tracker);
            update_ask_side(ask_tracker);
            
            // Detect changes
            detect_changes();
        }
        
        // Get depth levels (const access)
        const DepthArray& bid_levels() const { return bid_levels_; }
        const DepthArray& ask_levels() const { return ask_levels_; }
        size_t bid_count() const { return bid_count_; }
        size_t ask_count() const { return ask_count_; }
        
        // Best bid/offer access
        Price best_bid() const { 
            return bid_count_ > 0 ? bid_levels_[0].price : 0; 
        }
        
        Price best_ask() const { 
            return ask_count_ > 0 ? ask_levels_[0].price : 0; 
        }
        
        Quantity best_bid_qty() const { 
            return bid_count_ > 0 ? bid_levels_[0].quantity : 0; 
        }
        
        Quantity best_ask_qty() const { 
            return ask_count_ > 0 ? ask_levels_[0].quantity : 0; 
        }
        
        size_t best_bid_orders() const {
            return bid_count_ > 0 ? bid_levels_[0].order_count : 0;
        }
        
        size_t best_ask_orders() const {
            return ask_count_ > 0 ? ask_levels_[0].order_count : 0;
        }
        
        // Spread calculation
        Price spread() const {
            if (bid_count_ > 0 && ask_count_ > 0) {
                return ask_levels_[0].price - bid_levels_[0].price;
            }
            return 0;
        }
        
        // Mid price calculation
        Price mid_price() const {
            if (bid_count_ > 0 && ask_count_ > 0) {
                return (bid_levels_[0].price + ask_levels_[0].price) / 2;
            } else if (bid_count_ > 0) {
                return bid_levels_[0].price;
            } else if (ask_count_ > 0) {
                return ask_levels_[0].price;
            }
            return 0;
        }
        
        // Change detection
        bool has_changed() const { return changed_; }
        void clear_changed_flag() { changed_ = false; changes_.clear(); }
        const std::vector<DepthChange>& get_changes() const { return changes_; }
        
        // Get specific level
        const DepthLevel& get_bid_level(size_t level) const {
            static DepthLevel empty_level;
            return (level < bid_count_) ? bid_levels_[level] : empty_level;
        }
        
        const DepthLevel& get_ask_level(size_t level) const {
            static DepthLevel empty_level;
            return (level < ask_count_) ? ask_levels_[level] : empty_level;
        }
        
        // Utility functions
        bool empty() const {
            return bid_count_ == 0 && ask_count_ == 0;
        }
        
        void clear() {
            for (auto& level : bid_levels_) level.clear();
            for (auto& level : ask_levels_) level.clear();
            bid_count_ = 0;
            ask_count_ = 0;
            changed_ = false;
            changes_.clear();
        }
        
        // Market quality metrics
        double liquidity_score() const {
            double score = 0.0;
            
            // Weight levels by proximity to top of book
            for (size_t i = 0; i < bid_count_; ++i) {
                double weight = 1.0 / (i + 1);  // Level 0 = weight 1.0, Level 1 = 0.5, etc.
                score += bid_levels_[i].quantity * weight;
            }
            
            for (size_t i = 0; i < ask_count_; ++i) {
                double weight = 1.0 / (i + 1);
                score += ask_levels_[i].quantity * weight;
            }
            
            return score;
        }
        
        double spread_percentage() const {
            Price best_bid_price = best_bid();
            Price best_ask_price = best_ask();
            
            if (best_bid_price > 0 && best_ask_price > 0) {
                return ((double)(best_ask_price - best_bid_price) / best_bid_price) * 100.0;
            }
            return 0.0;
        }
        
        // Total quantity available within price range
        Quantity total_bid_quantity(Price min_price = 0) const {
            Quantity total = 0;
            for (size_t i = 0; i < bid_count_; ++i) {
                if (bid_levels_[i].price >= min_price) {
                    total += bid_levels_[i].quantity;
                }
            }
            return total;
        }
        
        Quantity total_ask_quantity(Price max_price = INT64_MAX) const {
            Quantity total = 0;
            for (size_t i = 0; i < ask_count_; ++i) {
                if (ask_levels_[i].price <= max_price) {
                    total += ask_levels_[i].quantity;
                }
            }
            return total;
        }
        
        // Display/debugging
        std::string to_string() const {
            std::ostringstream oss;
            
            oss << "\n=== Market Depth ===\n";
            oss << "BIDS (" << bid_count_ << " levels):\n";
            for (size_t i = 0; i < bid_count_; ++i) {
                oss << "  [" << i << "] " << bid_levels_[i].to_string() << "\n";
            }
            
            oss << "ASKS (" << ask_count_ << " levels):\n";
            for (size_t i = 0; i < ask_count_; ++i) {
                oss << "  [" << i << "] " << ask_levels_[i].to_string() << "\n";
            }
            
            oss << "Spread: $" << std::fixed << std::setprecision(2) << (spread() / 100.0);
            oss << ", Mid: $" << (mid_price() / 100.0) << "\n";
            
            return oss.str();
        }
        
        // Get formatted market depth table
        std::string format_market_depth(size_t max_levels = MAX_LEVELS) const {
            std::ostringstream oss;
            
            // Header
            oss << "\n" << std::string(60, '=') << "\n";
            oss << std::setw(30) << "MARKET DEPTH" << std::setw(30) << "\n";
            oss << std::string(60, '=') << "\n";
            oss << std::left << std::setw(15) << "BID SIZE" 
                << std::setw(10) << "BID" 
                << std::setw(10) << "ASK" 
                << std::setw(15) << "ASK SIZE" 
                << std::setw(10) << "ORDERS" << "\n";
            oss << std::string(60, '-') << "\n";
            
            size_t max_display = std::min(max_levels, std::max(bid_count_, ask_count_));
            
            for (size_t i = 0; i < max_display; ++i) {
                // Bid side
                if (i < bid_count_) {
                    oss << std::right << std::setw(10) << bid_levels_[i].quantity 
                        << "(" << std::setw(2) << bid_levels_[i].order_count << ")";
                    oss << std::setw(10) << std::fixed << std::setprecision(2) 
                        << (bid_levels_[i].price / 100.0);
                } else {
                    oss << std::setw(15) << " " << std::setw(10) << " ";
                }
                
                // Ask side  
                if (i < ask_count_) {
                    oss << std::setw(10) << std::fixed << std::setprecision(2) 
                        << (ask_levels_[i].price / 100.0);
                    oss << std::setw(10) << ask_levels_[i].quantity 
                        << "(" << std::setw(2) << ask_levels_[i].order_count << ")";
                } else {
                    oss << std::setw(10) << " " << std::setw(15) << " ";
                }
                
                oss << "\n";
            }
            
            oss << std::string(60, '=') << "\n";
            
            return oss.str();
        }

    private:
        // Save current state for change detection
        void save_previous_state() {
            prev_bid_levels_ = bid_levels_;
            prev_ask_levels_ = ask_levels_;
            prev_bid_count_ = bid_count_;
            prev_ask_count_ = ask_count_;
        }
        
        // Update bid side from tracker
        template<typename OrderPtr>
        void update_bid_side(const OrderTracker<OrderPtr>& tracker) {
            size_t level_idx = 0;
            const auto& price_levels = tracker.price_levels();
            
            // Clear existing bid levels
            for (auto& level : bid_levels_) {
                level.clear();
            }
            bid_count_ = 0;
            
            // Fill bid levels from tracker (already sorted by price priority)
            for (const auto& [price, price_level] : price_levels) {
                if (level_idx >= MAX_LEVELS) break;
                if (price_level->empty()) continue;
                
                bid_levels_[level_idx] = DepthLevel(
                    price, 
                    price_level->total_quantity(), 
                    price_level->order_count()
                );
                
                ++level_idx;
            }
            
            bid_count_ = level_idx;
        }
        
        // Update ask side from tracker
        template<typename OrderPtr>
        void update_ask_side(const OrderTracker<OrderPtr>& tracker) {
            size_t level_idx = 0;
            const auto& price_levels = tracker.price_levels();
            
            // Clear existing ask levels
            for (auto& level : ask_levels_) {
                level.clear();
            }
            ask_count_ = 0;
            
            // Fill ask levels from tracker (already sorted by price priority)
            for (const auto& [price, price_level] : price_levels) {
                if (level_idx >= MAX_LEVELS) break;
                if (price_level->empty()) continue;
                
                ask_levels_[level_idx] = DepthLevel(
                    price, 
                    price_level->total_quantity(), 
                    price_level->order_count()
                );
                
                ++level_idx;
            }
            
            ask_count_ = level_idx;
        }
        
        // Detect what changed between updates
        void detect_changes() {
            // Check bid levels for changes
            size_t max_bid_levels = std::max(bid_count_, prev_bid_count_);
            for (size_t i = 0; i < max_bid_levels; ++i) {
                const DepthLevel* current = (i < bid_count_) ? &bid_levels_[i] : nullptr;
                const DepthLevel* previous = (i < prev_bid_count_) ? &prev_bid_levels_[i] : nullptr;
                
                bool level_changed = false;
                Price price = 0;
                Quantity old_qty = 0, new_qty = 0;
                size_t old_count = 0, new_count = 0;
                
                if (current && previous) {
                    // Both exist - check for changes
                    if (*current != *previous) {
                        level_changed = true;
                        price = current->price;
                        old_qty = previous->quantity;
                        new_qty = current->quantity;
                        old_count = previous->order_count;
                        new_count = current->order_count;
                    }
                } else if (current && !previous) {
                    // New level added
                    level_changed = true;
                    price = current->price;
                    old_qty = 0;
                    new_qty = current->quantity;
                    old_count = 0;
                    new_count = current->order_count;
                } else if (!current && previous) {
                    // Level removed
                    level_changed = true;
                    price = previous->price;
                    old_qty = previous->quantity;
                    new_qty = 0;
                    old_count = previous->order_count;
                    new_count = 0;
                }
                
                if (level_changed) {
                    changed_ = true;
                    changes_.emplace_back(true, i, price, old_qty, new_qty, old_count, new_count);
                }
            }
            
            // Check ask levels for changes
            size_t max_ask_levels = std::max(ask_count_, prev_ask_count_);
            for (size_t i = 0; i < max_ask_levels; ++i) {
                const DepthLevel* current = (i < ask_count_) ? &ask_levels_[i] : nullptr;
                const DepthLevel* previous = (i < prev_ask_count_) ? &prev_ask_levels_[i] : nullptr;
                
                bool level_changed = false;
                Price price = 0;
                Quantity old_qty = 0, new_qty = 0;
                size_t old_count = 0, new_count = 0;
                
                if (current && previous) {
                    // Both exist - check for changes
                    if (*current != *previous) {
                        level_changed = true;
                        price = current->price;
                        old_qty = previous->quantity;
                        new_qty = current->quantity;
                        old_count = previous->order_count;
                        new_count = current->order_count;
                    }
                } else if (current && !previous) {
                    // New level added
                    level_changed = true;
                    price = current->price;
                    old_qty = 0;
                    new_qty = current->quantity;
                    old_count = 0;
                    new_count = current->order_count;
                } else if (!current && previous) {
                    // Level removed
                    level_changed = true;
                    price = previous->price;
                    old_qty = previous->quantity;
                    new_qty = 0;
                    old_count = previous->order_count;
                    new_count = 0;
                }
                
                if (level_changed) {
                    changed_ = true;
                    changes_.emplace_back(false, i, price, old_qty, new_qty, old_count, new_count);
                }
            }
        }
    };

    // Specialized depth trackers for different use cases
    using BBO_Tracker = DepthTracker<1>;        // Best Bid/Offer only
    using StandardDepth = DepthTracker<5>;      // 5 levels (common for retail)
    using DeepDepth = DepthTracker<10>;         // 10 levels (institutional)
    using VeryDeepDepth = DepthTracker<20>;
} // namespace OrderEngine

#endif // DEPTH_TRACKER_H