#include <gtest/gtest.h>
#include <price_level_v2.h>
#include <order.h>
#include <orderbook.h>
#include <id_allocator.h>

TEST(OrderBook, Basic) {
    IdAllocator id_allocator;
    OrderBook orderbook("SPY", id_allocator);

    Order order1("SPY", OrderSide::BUY, 100.0, 300);
    Order order2("SPY", OrderSide::BUY, 103.0, 200);
    Order order3("SPY", OrderSide::BUY, 102.0, 100);
    Order order4("SPY", OrderSide::SELL, 105.0, 3000);
    Order order5("SPY", OrderSide::SELL, 104.0, 200);
    Order order6("SPY", OrderSide::SELL, 104.0, 100);

    orderbook.add_order(order1);
    orderbook.add_order(order2);
    orderbook.add_order(order3);
    orderbook.add_order(order4);
    orderbook.add_order(order5);
    orderbook.add_order(order6);

    EXPECT_EQ(orderbook.get_best_bid(), 103.0);
    EXPECT_EQ(orderbook.get_best_ask(), 104.0);
}

TEST(OrderBook, Match) {
    IdAllocator id_allocator;
    OrderBook orderbook("SPY", id_allocator);

    Order order1("SPY", OrderSide::BUY, 100.0, 300);
    Order order2("SPY", OrderSide::SELL, 100.0, 200);

    orderbook.add_order(order1);
    orderbook.add_order(order2);

    EXPECT_EQ(orderbook.get_best_bid(), 100.0);
    EXPECT_EQ(orderbook.get_best_ask(), -1.0);

    Order order3("SPY", OrderSide::BUY, 101.0, 100);
    Order order4("SPY", OrderSide::SELL, 100.0, 200);

    orderbook.add_order(order3);
    EXPECT_EQ(orderbook.get_best_bid(), 101.0);

    // match 2 orders
    orderbook.add_order(order4);
    EXPECT_EQ(orderbook.get_best_bid(), -1.0);
    EXPECT_EQ(orderbook.get_best_ask(), -1.0);

    Order order5("SPY", OrderSide::BUY, 100.0, 500);
    Order order6("SPY", OrderSide::SELL, 101.0, 500);
    orderbook.add_order(order5);
    orderbook.add_order(order6);

    EXPECT_EQ(orderbook.get_best_bid(), 100.0);
    EXPECT_EQ(orderbook.get_best_ask(), 101.0);
}

TEST(OrderBook, ModifyOrder) {
    IdAllocator id_allocator;
    OrderBook orderbook("SPY", id_allocator);
    const std::map<Price, PriceLevel, std::greater<double>>& bids = orderbook.get_bids();

    Order order1("SPY", OrderSide::BUY, 100.0, 300);
    OrderId id1 = orderbook.add_order(order1);
    orderbook.modify_order(id1, 99.0, 300);
    EXPECT_EQ(orderbook.get_best_bid(), 99.0);

    Order order2("SPY", OrderSide::BUY, 99.0, 300);
    EXPECT_EQ(orderbook.get_best_bid(), 99.0);
    OrderId id2 = orderbook.add_order(order2);

    // decrease quantity, no order changed
    orderbook.modify_order(id1, 99.0, 200);
    const PriceLevel& best_level = bids.begin() -> second;
    const Order* order_bb1 = best_level.front();
    EXPECT_EQ(order_bb1 -> get_id(), id1);

    // increase quantity, queue priority changed
    orderbook.modify_order(id1, 99.0, 300);
    const Order* order_bb2 = best_level.front();
    EXPECT_EQ(order_bb2 -> get_id(), id2);
}

TEST(OrderBook, CancelOrder) {
    IdAllocator id_allocator;
    OrderBook orderbook("SPY", id_allocator);
    const std::map<Price, PriceLevel, std::greater<double>>& bids = orderbook.get_bids();

    Order order1("SPY", OrderSide::BUY, 100.0, 300);
    Order order2("SPY", OrderSide::BUY, 99.0, 300);
    Order order3("SPY", OrderSide::BUY, 99.0, 500);
    Order order4("SPY", OrderSide::BUY, 99.0, 1000);
    OrderId id1 = orderbook.add_order(order1);
    OrderId id2 = orderbook.add_order(order2);
    OrderId id3 = orderbook.add_order(order3);
    OrderId id4 = orderbook.add_order(order4);
    EXPECT_EQ(orderbook.get_best_bid(), 100.0);

    orderbook.cancel_order(id3);
    EXPECT_EQ(orderbook.get_best_bid(), 100.0);
    
    orderbook.cancel_order(id1);
    EXPECT_EQ(orderbook.get_best_bid(), 99.0);

    const PriceLevel& best_level = bids.begin() -> second;
    const Order* order_bb1 = best_level.front();
    EXPECT_EQ(order_bb1 -> get_id(), id2);
    EXPECT_EQ(best_level.size(), 2);

    orderbook.cancel_order(id2);
    const Order* order_bb2 = best_level.front();
    EXPECT_EQ(orderbook.get_best_bid(), 99.0);
    EXPECT_EQ(order_bb2 -> get_id(), id4);
    EXPECT_EQ(best_level.size(), 1);

    // invalided the whole best level, no order left
    orderbook.cancel_order(id4);
    EXPECT_EQ(orderbook.get_best_bid(), -1.0);
    EXPECT_EQ(bids.empty(), true);
}
