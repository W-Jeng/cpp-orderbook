#include <gtest/gtest.h>
#include <price_level.h>
#include <order.h>

TEST(PriceLevel, Basic) {
    PriceLevel price_level{100.0};

    Order order1{"SPY", OrderSide::BUY, 100.0, 1000};
    Order eorder1(std::move(order1), 0);
    Order order2{"SPY", OrderSide::BUY, 100.0, 200};
    Order eorder2(std::move(order2), 1);

    price_level.add_order(&eorder1);
    price_level.add_order(&eorder2);
}

TEST(PriceLevel, Fill) {
    constexpr double PRICE = 100.0;
    PriceLevel price_level{PRICE};

    Order order1{"SPY", OrderSide::BUY, PRICE, 1000};
    Order eorder1(std::move(order1), 0);
    Order order2{"SPY", OrderSide::BUY, PRICE, 2000};
    Order eorder2(std::move(order2), 1);
    Order order3{"SPY", OrderSide::BUY, PRICE, 300};
    Order eorder3(std::move(order3), 2);

    price_level.add_order(&eorder1);
    price_level.add_order(&eorder2);
    price_level.add_order(&eorder3);

    Order* order = price_level.front();
    EXPECT_EQ(order -> get_id(), 0);

    order -> match(100);
    price_level.pop_front_order_if_filled();
    EXPECT_EQ(order -> get_status(), OrderStatus::PARTIALLY_FILLED);
    EXPECT_EQ(order -> get_quantity_filled(), 100);

    order -> match(900);
    EXPECT_EQ(order -> get_status(), OrderStatus::FULLY_FILLED);
    price_level.pop_front_order_if_filled();
    EXPECT_EQ(price_level.front() -> get_id(), 1);
    EXPECT_EQ(price_level.front() -> get_status(), OrderStatus::NEW);
}

