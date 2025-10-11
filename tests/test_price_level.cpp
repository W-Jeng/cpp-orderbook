#include <gtest/gtest.h>
#include <price_level.h>
#include <order.h>

TEST(PriceLevel, Basic) {
    PriceLevel price_level{100.0};

    Order order1{"SPY", OrderSide::BUY, 100.0, 1000};
    ExchangeOrder eorder1(order1, 0);
    Order order2{"SPY", OrderSide::BUY, 100.0, 200};
    ExchangeOrder eorder2(order2, 1);

    price_level.add_order(&eorder1);
    price_level.add_order(&eorder2);
}

TEST(PriceLevel, Fill) {
    constexpr double PRICE = 100.0;
    PriceLevel price_level{PRICE};

    Order order1{"SPY", OrderSide::BUY, PRICE, 1000};
    ExchangeOrder eorder1(order1, 0);
    Order order2{"SPY", OrderSide::BUY, PRICE, 2000};
    ExchangeOrder eorder2(order2, 1);
    Order order3{"SPY", OrderSide::BUY, PRICE, 300};
    ExchangeOrder eorder3(order3, 2);

    price_level.add_order(&eorder1);
    price_level.add_order(&eorder2);
    price_level.add_order(&eorder3);

    ExchangeOrder* order = price_level.front();
    EXPECT_EQ(order -> _id, 0);

    order -> match(100);
    price_level.pop_front_order_if_filled();
    EXPECT_EQ(order -> _status, OrderStatus::PARTIALLY_FILLED);
    EXPECT_EQ(order -> _quantity_filled, 100);

    order -> match(900);
    EXPECT_EQ(order -> _status, OrderStatus::FULLY_FILLED);
    price_level.pop_front_order_if_filled();
    EXPECT_EQ(price_level.front() -> _id, 1);
    EXPECT_EQ(price_level.front() -> _status, OrderStatus::NEW);
}

