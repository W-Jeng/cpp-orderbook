#include <iostream>
#include <print>
#include <order.h>
#include <orderbook.h>


int main() {
    Order order{"SPY", OrderSide::BUY, 100.0, 200};
    ExchangeOrder eorder(order, 123);
    std::cout << order << "\n";
    std::cout << eorder << "\n";
    return 0;
}