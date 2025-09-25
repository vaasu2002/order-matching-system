#pragma once

#ifndef ORDER_TYPES_H
#define ORDER_TYPES_H

#include <cstdint>
#include <string>
#include <memory>
#include <chrono>

namespace OrderEngine {
    using Price = int64_t;          // Price in smallest currency unit (paisa)
    using Quantity = uint64_t;      // Order quantity
    using OrderId = uint64_t;       // Unique order identifier
    using Symbol = std::string;     // Trading symbol
    using Timestamp = std::chrono::high_resolution_clock::time_point;

    /* Special price values
     * - MARKET_PRICE   : Represents a market order (execute immediately at best available price).
     * - PRICE_UNCHANGED: Special flag meaning "do not change the price" when modifying an existing order.
     * - SIZE_UNCHANGED : Special flag meaning "do not change the size" when modifying an existing order.
    */
    static constexpr Price MARKET_PRICE = 0;
    static constexpr Price PRICE_UNCHANGED = -1;
    static constexpr Quantity SIZE_UNCHANGED = UINT64_MAX;

    // Represents which side of a financial order the trader is on.
    enum class OrderSide : char {
        BUY = 'B',
        SELL = 'S'
    };

    /* Supported order types
     * - LIMIT     : Executes at a specified price or better.
     * - MARKET    : Executes immediately at the best available price.
     * - STOP      : Converts to a market order once a trigger price is hit.
     * - STOP_LIMIT: Converts to a limit order once a trigger price is hit.
    */
    enum class OrderType : char {
        LIMIT = 'L',
        MARKET = 'M',
        STOP = 'T',
        STOP_LIMIT = 'S'
    };

    /* Order time in force
     * - GOOD_TILL_CANCELLED: Remains active until explicitly cancelled (GTC).
     * - IMMEDIATE_OR_CANCEL: Must execute immediately, any unfilled portion is cancelled (IOC).
     * - FILL_OR_KILL       : Must be filled entirely and immediately, otherwise cancelled (FOK).
     * - DAY                : Remains active only for the trading day on which it was placed.
    */
    enum class TimeInForce : char {
        GOOD_TILL_CANCELLED = 'G',    // GTC
        IMMEDIATE_OR_CANCEL = 'I',    // IOC
        FILL_OR_KILL = 'F',          // FOK
        DAY = 'D'                    // DAY
    };

    /* Bitmask flags representing special order conditions. Multiple conditions can be combined
     * - NO_CONDITIONS      : Default, no special execution constraints. (DEFAULT)
     * - ALL_OR_NONE        : Order must be filled completely or not at all.
     * - IMMEDIATE_OR_CANCEL: Order must execute immediately; unfilled portion is cancelled.
     * - FILL_OR_KILL (FOK) : Combination of ALL_OR_NONE and IMMEDIATE_OR_CANCEL — 
     *                        must be fully filled immediately, otherwise cancelled.
     * - HIDDEN             : Order is not displayed in the public order book.
     * - ICEBERG            : Only a portion of the total order is displayed, 
     *                        with hidden quantity revealed as visible portions are filled.
    */
    enum OrderConditions : uint32_t {
        NO_CONDITIONS = 0,
        ALL_OR_NONE = 1 << 0,
        IMMEDIATE_OR_CANCEL = 1 << 1,
        FILL_OR_KILL = (ALL_OR_NONE | IMMEDIATE_OR_CANCEL),
        HIDDEN = 1 << 2,
        ICEBERG = 1 << 3
    };

    /* Order lifecycle states
     * - PENDING         : Order has been received but not yet processed.
     * - ACCEPTED        : Order has passed validation and is now active.
     * - PARTIALLY_FILLED: Order has been partially executed, remaining 
     *                     quantity is still active.
     * - FILLED          : Order has been completely executed.
     * - CANCELLED       : Order was cancelled before completion.
     * - REJECTED        : Order was rejected (e.g., invalid or not allowed).
     * - REPLACED        : Order has been modified and replaced with a new one.
    */
    enum class OrderStatus : char {
        PENDING = 'P',
        ACCEPTED = 'A',
        PARTIALLY_FILLED = 'F',
        FILLED = 'C',
        CANCELLED = 'X',
        REJECTED = 'R',
        REPLACED = 'E'
    };

    /* Bitmask flags describing the characteristics of a trade fill.
     * Flags can be combined to capture both execution role and completion status.
     * - FILL_NORMAL    : Default, no special flags.
     * - FILL_AGGRESSIVE: The order actively removed liquidity (aggressor).
     * - FILL_PASSIVE   : The order provided liquidity by resting in the book.
     * - FILL_PARTIAL   : Fill covered only part of the order’s quantity.
     * - FILL_COMPLETE  : Fill fully satisfied the order’s quantity.
    */
    enum FillFlags : uint32_t {
        FILL_NORMAL = 0,
        FILL_AGGRESSIVE = 1 << 0,     
        FILL_PASSIVE = 1 << 1,       
        FILL_PARTIAL = 1 << 2,
        FILL_COMPLETE = 1 << 3    
    };

} // namespace OrderEngine

#endif // ORDER_TYPES_H