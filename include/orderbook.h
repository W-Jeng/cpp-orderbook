#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <new>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <price_level_v2.h>
#include <order.h>
#include <id_allocator.h>
#include <order_pool.h>
#include <core.h>
#include <boost/unordered/unordered_flat_map.hpp>

class alignas(CACHE_LINE_SIZE) OrderBook {
public:
    inline static constexpr size_t ORDER_REGISTRY_RESERVE = 1'000'000;
    std::chrono::duration<double> elapsed;
    using clock = std::chrono::high_resolution_clock;

    explicit OrderBook(const Instrument& instrument, IdAllocator& id_allocator): 
        _instrument(instrument),
        _id_allocator(id_allocator),
        _order_id_block(id_allocator.get_next_id_block()),
        _order_pool(OrderPool{ORDER_REGISTRY_RESERVE}),
        _trades_completed(0),
        _try_match_flag(false)
    {
        _next_available_order_id = _order_id_block._start;
        _order_registry.reserve(ORDER_REGISTRY_RESERVE*2);
    }
    
    // explicitly allow moving
    OrderBook(OrderBook&&) noexcept = default;
    OrderBook& operator=(OrderBook&&) noexcept = default;

    // explicitly forbid copying
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    // returns order id
    OrderId add_order(Order& client_order) {
        Order* order = _order_pool.allocate();
        order -> copy_from_client(std::move(client_order), get_order_id());
        OrderId order_id = order -> get_id();

        switch (order -> get_side()) {
            case OrderSide::BUY:
                add_order_dispatcher(_bids, order);           
                break;
                
            case OrderSide::SELL:
                add_order_dispatcher(_asks, order);
                break;
        }

        try_match();
        return order_id;
    }
    
    void print_time() {
        std::cout << "Elapsed time (count) in Orderbook Add: " << elapsed.count() << "\n";

        for (auto& [p, price_level]: _bids) {
            std::cout << "Bid Price info: " << p << ", ";
            price_level.print_time();
        }
        for (auto& [p, price_level]: _asks) {
            std::cout << "Ask Price info: " << p << ", ";
            price_level.print_time();
        }
    }

    template <typename MapType>
    void add_order_dispatcher(MapType& side_levels, Order* order) {
        auto [it, inserted] = side_levels.try_emplace(order -> get_price(), order -> get_price());
        _try_match_flag = inserted;
        PriceLevel& p_level = it -> second;

        // auto start = clock::now();
        p_level.add_order(order);
        // auto end = clock::now();
        // elapsed += end-start;
        _order_registry[order -> get_id()] = order;
    }

    bool cancel_order(OrderId order_id) {
        auto it = _order_registry.find(order_id);
        
        if (it == _order_registry.end()) 
            return false;
        
        Order* order = it->second;
        bool cancel_sucess = order->cancel();
        
        if (!cancel_sucess) {
            return false;   
        }
        
        try {
            switch (order -> get_side()) {
                case OrderSide::BUY:
                    cancel_order_dispatcher(_bids, order);           
                    break;

                case OrderSide::SELL:
                    cancel_order_dispatcher(_asks, order);           
                    break;
            }
        } catch (const std::runtime_error& e) {
            std::cout << "Unknown runtime error caught in cancel order dispatcher: " << e.what() << std::endl;   
        } catch (const std::exception& e) {
            std::cout << "Unknown exception caught in cancel order dispatcher: " << e.what() << std::endl;   
        } catch (...) {
            std::cerr << "Unknown exception caught in cancel order dispatcher!" << std::endl;   
        }
        return true;
    }
    
    template<typename MapType>
    void cancel_order_dispatcher(MapType& side_levels, Order* order) {
        auto it = side_levels.find(order -> get_price());
        PriceLevel& price_level = it -> second;
        price_level.remove_order(order -> get_id());
        remove_price_level_if_empty(side_levels, it);
    }
    
    template<typename MapType>
    void remove_price_level_if_empty(MapType& side_levels, typename MapType::iterator price_level_it) {
        if (!(price_level_it -> second).empty())
            return;
            
        // empty, prune the price level!
        side_levels.erase(price_level_it);
    }
    
    bool modify_order(OrderId order_id, Price price, Quantity quantity) {
        auto it = _order_registry.find(order_id);
        
        if (it == _order_registry.end()) 
            return false;
        
        Order* order = it->second;
        OrderStatus status = order -> get_status();
        
        if (status == OrderStatus::CANCELLED || status == OrderStatus::FULLY_FILLED) 
            return false;   
        
        OrderSide side = order -> get_side();
        
        try {
            switch (order -> get_side()) {
                case OrderSide::BUY:
                    return modify_order_dispatcher(_bids, order, price, quantity);           
                case OrderSide::SELL:
                    return modify_order_dispatcher(_asks, order, price, quantity);           
            }
        } catch (const std::runtime_error& e) {
            std::cout << "Unknown runtime error caught in modify order dispatcher: " << e.what() << std::endl;   
        } catch (const std::exception& e) {
            std::cout << "Unknown exception caught in modify order dispatcher: " << e.what() << std::endl;   
        } catch (...) {
            std::cerr << "Unknown exception caught in modify order dispatcher!" << std::endl;   
        }
        
        try_match();
        return false;
    }
    
    template<typename MapType>
    bool modify_order_dispatcher(MapType& side_levels, Order* order, Price new_price, Quantity new_qty) {
        if (!validate_modify_order(order, new_price, new_qty))
            return false;   
        
        auto it = side_levels.find(order -> get_price());
        PriceLevel& old_price_level = it -> second;
        bool price_changed = (order -> get_price() != new_price);
        bool quantity_changed = (order -> get_quantity() != new_qty);
        
        if (price_changed) {
            old_price_level.remove_order(order -> get_id());   
            order -> set_price(new_price);
            auto [it, inserted] = side_levels.try_emplace(new_price, new_price);
            _try_match_flag = inserted;
            PriceLevel& new_price_level = it -> second;
            new_price_level.add_order(order);
        }
        
        if (quantity_changed) {
            bool quantity_increased = (new_qty > order -> get_quantity());
            order -> set_quantity(new_qty);
            
            if (quantity_increased && !price_changed) {
                old_price_level.remove_order(order -> get_id());
                old_price_level.add_order(order);
            }
        }
        
        remove_price_level_if_empty(side_levels, it);
        return (price_changed || quantity_changed);
    }
    
    bool validate_modify_order(Order* order, Price new_price, Quantity new_qty) {
        if (new_price <= 0.0 || new_qty <= order -> get_quantity_filled()) 
            return false;   
        
        return true;
    }
    
    bool try_match() {
        if (!_try_match_flag) 
            return false;

        bool order_matched = false;
        
        while (true) {
            auto bid_it = _bids.begin();
            auto ask_it = _asks.begin();
            
            if (bid_it == _bids.end() || ask_it == _asks.end()) {
                return order_matched;   
            }

            bool match = (bid_it -> first) >= (ask_it -> first);
            
            if (!match) {
                _try_match_flag = false;
                return order_matched;
            }
            
            order_matched = true;
            ++_trades_completed;
            
            // implement order match
            PriceLevel& best_bid_price_level = bid_it -> second;
            PriceLevel& best_ask_price_level = ask_it -> second;
            Order* bid_order = best_bid_price_level.front();
            Order* ask_order = best_ask_price_level.front();
            match_orders(bid_order, ask_order);
            best_bid_price_level.pop_front_order_if_filled();
            best_ask_price_level.pop_front_order_if_filled();
            remove_price_level_if_empty(_bids, bid_it);
            remove_price_level_if_empty(_asks, ask_it);
        }
        
        _try_match_flag = false;
        return order_matched;
    }
    
    void match_orders(Order* bid_order, Order* ask_order) {
        Quantity bid_available = bid_order -> get_quantity() - bid_order -> get_quantity_filled();
        Quantity ask_available = ask_order -> get_quantity() - ask_order -> get_quantity_filled();
        Quantity match_qty = std::min(bid_available, ask_available);
        bid_order -> match(match_qty);
        ask_order -> match(match_qty);
    }
    
    OrderId get_order_id() {
        if (_next_available_order_id == _order_id_block._end) {
            _order_id_block = _id_allocator.get_next_id_block();
            _next_available_order_id = _order_id_block._start;
        }
        return _next_available_order_id++;
    }
    
    Price get_best_bid() {
        if (_bids.empty()) {
            return -1.0;
        }
        return _bids.begin() -> first;
    }

    Price get_best_ask() {
        if (_asks.empty()) {
            return -1.0;
        }

        return _asks.begin() -> first;
    }

    uint64_t get_trades_completed() const {
        return _trades_completed;
    }

    const std::map<Price, PriceLevel, std::greater<double>>& get_bids() const {
        return _bids;
    }

    const std::map<Price, PriceLevel, std::less<double>>& get_asks() const {
        return _asks;
    }
    
    Instrument get_instrument() const {
        return _instrument;
    }

private:
    const Instrument _instrument;
    std::map<Price, PriceLevel, std::greater<double>> _bids;
    std::map<Price, PriceLevel, std::less<double>> _asks;
    boost::unordered_flat_map<OrderId, Order*> _order_registry;
    IdAllocator& _id_allocator;
    OrderPool _order_pool;
    OrderIdBlock _order_id_block;
    OrderId _next_available_order_id;
    uint64_t _trades_completed;
    bool _try_match_flag;
    char pad[64];
};

inline std::string price_to_string(double p, int precision = 2) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(precision);
    oss << p;
    return oss.str();
}

std::unordered_map<std::string, std::string> get_default_header(const std::vector<std::string>& headers) {
    std::unordered_map<std::string, std::string> mp;

    for (const std::string& h: headers) {
        mp[h] = h;
    }

    return mp;
}

const std::string row_to_string(
    const std::vector<std::string>& header, 
    const std::unordered_map<std::string, std::string>& row,
    const std::unordered_map<std::string, int>& col_max_size
) {
    std::string printable = "|";

    for (const auto& h: header) {
        if (h == "Ask Price")
            printable += "|";
        
        int total_space = col_max_size.at(h);
        int num_empty_spaces = total_space - row.at(h).size();
        int left_empty_space = num_empty_spaces/2 + 1;
        int right_empty_space = num_empty_spaces - left_empty_space + 2;
        printable += std::string(left_empty_space, ' ') + row.at(h) + std::string(right_empty_space, ' ') + "|";
    }

    return printable;
}

inline std::ostream& operator<<(std::ostream& os, const OrderBook& orderbook) {
    // We will go for the format of 
    // Bid Orders | Bid Qty | Bid Price || Ask Price | Ask Qty | Ask Orders
    // --------------------------------------------------------------------
    //      2     |  1000   |    1.0    ||    1.01   |  500    |    5
    //      1     |   500   |    0.98   ||    1.05   |  300    |    2
    // ---------------------------------------------------------------------

    std::vector<std::string> headers = {
        "Bid Orders", "Bid Qty", "Bid Price", "Ask Price", "Ask Qty", "Ask Orders"
    };
    std::unordered_map<std::string, int> column_max_size;
    int total_col_size = 0;

    // default size
    for (const auto& header: headers) {
        column_max_size[header] = header.size();
        total_col_size += header.size();
    }

    std::vector<std::unordered_map<std::string, std::string>> orderbook_table_rows;
    orderbook_table_rows.push_back(get_default_header(headers));

    const std::map<Price, PriceLevel, std::greater<double>> bids = orderbook.get_bids();
    const std::map<Price, PriceLevel, std::less<double>> asks = orderbook.get_asks();

    size_t rows = std::max(bids.size(), asks.size());
    auto bit = bids.begin();
    auto ait = asks.begin();

    for (size_t i = 0; i < rows; ++i) {
        std::unordered_map<std::string, std::string> row = get_default_header(headers);

        if (bit != bids.end()) {
            const auto& [bprice, blevel] = *bit;
            row["Bid Orders"] = std::to_string(blevel.size());               
            row["Bid Qty"]    = std::to_string(blevel.total_qty());
            row["Bid Price"]  = price_to_string(bprice, 2);
            ++bit;
        } else {
            row["Bid Orders"] = "";
            row["Bid Qty"]    = "";
            row["Bid Price"]  = "";
        }

        if (ait != asks.end()) {
            const auto& [aprice, alevel] = *ait;
            row["Ask Orders"] = std::to_string(alevel.size());
            row["Ask Qty"]    = std::to_string(alevel.total_qty());
            row["Ask Price"]  = price_to_string(aprice, 2);
            ++ait;
        } else {
            row["Ask Orders"] = "";
            row["Ask Qty"]    = "";
            row["Ask Price"]  = "";
        }
        int temp_total = 0;

        for (const auto& header: headers) {
            column_max_size[header] = std::max(column_max_size[header], static_cast<int>(row[header].size()));
            temp_total += column_max_size[header];
        }

        total_col_size = std::max(total_col_size, temp_total);
        orderbook_table_rows.push_back(std::move(row));
    }

    std::vector<std::string> printables;
    
    for (auto& mp: orderbook_table_rows) {
        printables.push_back(row_to_string(headers, mp, column_max_size));
    }

    std::string res;

    for (int i = 0; i < printables.size(); ++i) {
        res += printables[i] + "\n";

        if (i == 0 || i == printables.size()-1) {
            res += std::string(total_col_size+20, '-') + "\n";
        }
    }

    return os << '\n' << res;
}