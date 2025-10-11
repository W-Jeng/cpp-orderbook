#include <gtest/gtest.h>
#include <price_level.h>
#include <order.h>
#include <orderbook.h>
#include <id_allocator.h>

TEST(OrderBook, Basic) {
    IdAllocator id_allocator;
    OrderBook orderbook("SPY", id_allocator);

    Order order1("SPY", OrderSide::BUY, 100.0, 300);
    Order order2("SPY", OrderSide::BUY, 103.0, 200);
    Order order3("SPY", OrderSide::BUY, 102.0, 100);
    Order order4("SPY", OrderSide::SELL, 105.0, 300);
    Order order5("SPY", OrderSide::SELL, 104.0, 200);
    Order order6("SPY", OrderSide::SELL, 104.0, 100);

    orderbook.add_order(order1);
    orderbook.add_order(order2);
    orderbook.add_order(order3);
    orderbook.add_order(order4);
    orderbook.add_order(order5);
    orderbook.add_order(order6);
    std::cout << orderbook;

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


