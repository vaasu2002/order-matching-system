#include "Order.h"
#include <map>
#include <list>
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
} // namespace OrderEngine