#pragma once
#include "OrderTypes.h"
// Canonical location for Order class
namespace OrderEngine {
  class Order {
  private:
      OrderId order_id_;
      Symbol symbol_;
      OrderSide side_;
      Quantity quantity_; // original order quantity
      Quantity open_quantity_; // currrently unfilled quantity
      Price price_;
      Price stop_price_;
      OrderType order_type_;
      TimeInForce time_in_force_;
      OrderStatus status_;
      Timestamp timestamp_;
  public:
      Order(OrderId id, const Symbol& symbol, OrderSide side, Quantity qty,
            Price price, OrderType type = OrderType::LIMIT,
            TimeInForce tif = TimeInForce::GOOD_TILL_CANCELLED)
          : order_id_(id), symbol_(symbol), side_(side), quantity_(qty),
            open_quantity_(qty), price_(price), stop_price_(0),
            order_type_(type), time_in_force_(tif), status_(OrderStatus::PENDING),
            timestamp_(std::chrono::high_resolution_clock::now()) {}

      OrderId order_id() const { return order_id_; }
      Symbol symbol() const { return symbol_; }
      OrderSide side() const { return side_; }
      Quantity quantity() const { return quantity_; }
      Quantity open_quantity() const { return open_quantity_; }
      Price price() const { return price_; }
      Price stop_price() const { return stop_price_; }
      OrderType order_type() const { return order_type_; }
      TimeInForce time_in_force() const { return time_in_force_; }
      Timestamp timestamp() const { return timestamp_; }
      OrderStatus status() const { return status_; }

      void set_quantity(Quantity qty) { quantity_ = qty; }
      void set_open_quantity(Quantity qty) { open_quantity_ = qty; }
      void set_price(Price price) { price_ = price; }
      void set_status(OrderStatus status) { status_ = status; }
      void set_stop_price(Price price) { stop_price_ = price; }

      // Optional methods for advanced features
      bool is_buy() const { return side() == OrderSide::BUY; }
      bool is_sell() const { return side() == OrderSide::SELL; }
      bool is_market() const { return order_type() == OrderType::MARKET; }
      bool is_limit() const { return order_type() == OrderType::LIMIT; }
      bool is_stop() const { return order_type() == OrderType::STOP || order_type() == OrderType::STOP_LIMIT; }
      bool is_all_or_none() const { return false; }
      bool is_immediate_or_cancel() const { return time_in_force() == TimeInForce::IMMEDIATE_OR_CANCEL; }
      bool is_fill_or_kill() const { return time_in_force() == TimeInForce::FILL_OR_KILL; }
  };
} // namespace OrderEngine