// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Exchange Integration API
// Standard interface for cryptocurrency exchange integrations

#ifndef INTCOIN_EXCHANGE_API_H
#define INTCOIN_EXCHANGE_API_H

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <cstdint>
#include <chrono>

namespace intcoin {
namespace exchange {

/**
 * Order types
 */
enum class OrderType {
    MARKET,
    LIMIT,
    STOP_LOSS,
    STOP_LIMIT,
    TAKE_PROFIT
};

/**
 * Order side
 */
enum class OrderSide {
    BUY,
    SELL
};

/**
 * Order status
 */
enum class OrderStatus {
    PENDING,
    OPEN,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED,
    EXPIRED
};

/**
 * Deposit/withdrawal status
 */
enum class TransferStatus {
    PENDING,
    PROCESSING,
    COMPLETED,
    FAILED,
    CANCELLED
};

/**
 * Trading pair information
 */
struct TradingPair {
    std::string symbol;           // e.g., "INT_BTC"
    std::string base_asset;       // e.g., "INT"
    std::string quote_asset;      // e.g., "BTC"
    uint8_t base_precision;       // Decimal places for base
    uint8_t quote_precision;      // Decimal places for quote
    double min_quantity;          // Minimum order quantity
    double max_quantity;          // Maximum order quantity
    double min_notional;          // Minimum order value
    double tick_size;             // Price tick size
    double step_size;             // Quantity step size
    bool trading_enabled;
};

/**
 * Order book entry
 */
struct OrderBookEntry {
    double price;
    double quantity;
};

/**
 * Order book snapshot
 */
struct OrderBook {
    std::string symbol;
    std::vector<OrderBookEntry> bids;  // Buy orders (highest first)
    std::vector<OrderBookEntry> asks;  // Sell orders (lowest first)
    uint64_t last_update_id;
    uint64_t timestamp;
};

/**
 * Trade information
 */
struct Trade {
    std::string id;
    std::string symbol;
    double price;
    double quantity;
    double quote_quantity;
    uint64_t timestamp;
    bool is_buyer_maker;
};

/**
 * Ticker data (24h statistics)
 */
struct Ticker {
    std::string symbol;
    double price;                 // Current price
    double price_change;          // 24h change
    double price_change_percent;  // 24h change %
    double high;                  // 24h high
    double low;                   // 24h low
    double volume;                // 24h volume (base)
    double quote_volume;          // 24h volume (quote)
    double open;                  // 24h open
    double close;                 // Current close
    uint64_t timestamp;
};

/**
 * OHLCV candlestick
 */
struct Candlestick {
    uint64_t open_time;
    double open;
    double high;
    double low;
    double close;
    double volume;
    uint64_t close_time;
    double quote_volume;
    uint32_t trade_count;
};

/**
 * Order information
 */
struct Order {
    std::string id;
    std::string client_order_id;
    std::string symbol;
    OrderType type;
    OrderSide side;
    OrderStatus status;
    double price;
    double stop_price;
    double quantity;
    double filled_quantity;
    double filled_quote;
    double fee;
    std::string fee_asset;
    uint64_t created_at;
    uint64_t updated_at;
};

/**
 * Account balance for a single asset
 */
struct AssetBalance {
    std::string asset;
    double free;          // Available balance
    double locked;        // In orders
    double total() const { return free + locked; }
};

/**
 * Deposit address
 */
struct DepositAddress {
    std::string asset;
    std::string address;
    std::string tag;      // Memo/tag if required
    std::string network;
};

/**
 * Deposit/withdrawal record
 */
struct TransferRecord {
    std::string id;
    std::string tx_hash;
    std::string asset;
    std::string address;
    double amount;
    double fee;
    TransferStatus status;
    uint64_t timestamp;
    uint32_t confirmations;
    uint32_t required_confirmations;
};

/**
 * Exchange API credentials
 */
struct APICredentials {
    std::string api_key;
    std::string api_secret;
    std::string passphrase;  // Some exchanges require this
};

/**
 * API error
 */
struct APIError {
    int code;
    std::string message;
};

/**
 * API result wrapper
 */
template<typename T>
struct APIResult {
    bool success;
    std::optional<T> data;
    std::optional<APIError> error;

    static APIResult<T> ok(T value) {
        return {true, std::move(value), std::nullopt};
    }

    static APIResult<T> fail(int code, const std::string& message) {
        return {false, std::nullopt, APIError{code, message}};
    }
};

/**
 * Exchange API Interface
 * Implement this for each exchange integration
 */
class ExchangeAPI {
public:
    virtual ~ExchangeAPI() = default;

    // ========== Public API (no auth required) ==========

    // Get exchange information
    virtual std::string get_name() const = 0;
    virtual std::string get_version() const = 0;

    // Get all trading pairs
    virtual APIResult<std::vector<TradingPair>> get_trading_pairs() = 0;

    // Get ticker for symbol
    virtual APIResult<Ticker> get_ticker(const std::string& symbol) = 0;

    // Get all tickers
    virtual APIResult<std::vector<Ticker>> get_all_tickers() = 0;

    // Get order book
    virtual APIResult<OrderBook> get_order_book(const std::string& symbol,
                                                 int limit = 100) = 0;

    // Get recent trades
    virtual APIResult<std::vector<Trade>> get_recent_trades(
        const std::string& symbol, int limit = 100) = 0;

    // Get candlestick data
    virtual APIResult<std::vector<Candlestick>> get_candlesticks(
        const std::string& symbol,
        const std::string& interval,  // "1m", "5m", "1h", "1d", etc.
        int limit = 100) = 0;

    // ========== Private API (auth required) ==========

    // Set credentials
    virtual void set_credentials(const APICredentials& credentials) = 0;

    // Test connectivity and credentials
    virtual APIResult<bool> test_connection() = 0;

    // Get account balances
    virtual APIResult<std::vector<AssetBalance>> get_balances() = 0;

    // Get balance for specific asset
    virtual APIResult<AssetBalance> get_balance(const std::string& asset) = 0;

    // ========== Order Management ==========

    // Create new order
    virtual APIResult<Order> create_order(
        const std::string& symbol,
        OrderSide side,
        OrderType type,
        double quantity,
        std::optional<double> price = std::nullopt,
        std::optional<double> stop_price = std::nullopt,
        const std::string& client_order_id = "") = 0;

    // Cancel order
    virtual APIResult<Order> cancel_order(const std::string& symbol,
                                          const std::string& order_id) = 0;

    // Cancel all orders for symbol
    virtual APIResult<std::vector<Order>> cancel_all_orders(
        const std::string& symbol) = 0;

    // Get order status
    virtual APIResult<Order> get_order(const std::string& symbol,
                                        const std::string& order_id) = 0;

    // Get open orders
    virtual APIResult<std::vector<Order>> get_open_orders(
        const std::string& symbol = "") = 0;

    // Get order history
    virtual APIResult<std::vector<Order>> get_order_history(
        const std::string& symbol = "",
        uint64_t start_time = 0,
        uint64_t end_time = 0,
        int limit = 100) = 0;

    // Get trade history
    virtual APIResult<std::vector<Trade>> get_trade_history(
        const std::string& symbol = "",
        uint64_t start_time = 0,
        uint64_t end_time = 0,
        int limit = 100) = 0;

    // ========== Deposits & Withdrawals ==========

    // Get deposit address
    virtual APIResult<DepositAddress> get_deposit_address(
        const std::string& asset,
        const std::string& network = "") = 0;

    // Get deposit history
    virtual APIResult<std::vector<TransferRecord>> get_deposits(
        const std::string& asset = "",
        uint64_t start_time = 0,
        uint64_t end_time = 0) = 0;

    // Initiate withdrawal
    virtual APIResult<TransferRecord> withdraw(
        const std::string& asset,
        const std::string& address,
        double amount,
        const std::string& tag = "",
        const std::string& network = "") = 0;

    // Get withdrawal history
    virtual APIResult<std::vector<TransferRecord>> get_withdrawals(
        const std::string& asset = "",
        uint64_t start_time = 0,
        uint64_t end_time = 0) = 0;

    // ========== WebSocket Streams ==========

    using TickerCallback = std::function<void(const Ticker&)>;
    using OrderBookCallback = std::function<void(const OrderBook&)>;
    using TradeCallback = std::function<void(const Trade&)>;
    using OrderCallback = std::function<void(const Order&)>;
    using BalanceCallback = std::function<void(const AssetBalance&)>;

    // Subscribe to ticker updates
    virtual void subscribe_ticker(const std::string& symbol,
                                  TickerCallback callback) = 0;

    // Subscribe to order book updates
    virtual void subscribe_order_book(const std::string& symbol,
                                       OrderBookCallback callback) = 0;

    // Subscribe to trade updates
    virtual void subscribe_trades(const std::string& symbol,
                                   TradeCallback callback) = 0;

    // Subscribe to user order updates (requires auth)
    virtual void subscribe_orders(OrderCallback callback) = 0;

    // Subscribe to balance updates (requires auth)
    virtual void subscribe_balances(BalanceCallback callback) = 0;

    // Unsubscribe from stream
    virtual void unsubscribe(const std::string& symbol) = 0;

    // Close all streams
    virtual void close_streams() = 0;
};

/**
 * Exchange factory
 */
class ExchangeFactory {
public:
    // Create exchange API by name
    static std::unique_ptr<ExchangeAPI> create(const std::string& exchange_name);

    // Get list of supported exchanges
    static std::vector<std::string> supported_exchanges();
};

/**
 * Exchange rate service
 * Aggregates rates from multiple exchanges
 */
class ExchangeRateService {
public:
    ExchangeRateService();
    ~ExchangeRateService();

    // Get current INT price in specified currency
    double get_price(const std::string& currency = "USD") const;

    // Get 24h price change
    double get_price_change_24h(const std::string& currency = "USD") const;

    // Get historical price
    double get_historical_price(const std::string& currency,
                                uint64_t timestamp) const;

    // Force refresh rates
    void refresh();

    // Set update interval
    void set_update_interval(std::chrono::seconds interval);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace exchange
} // namespace intcoin

#endif // INTCOIN_EXCHANGE_API_H
