#include <iostream>
#include <print>
#include <order.h>
#include <orderbook.h>
#include <id_allocator.h>
#include <book_map.h>

int main() {
    Order order{"SPY", OrderSide::BUY, 100.0, 200};
    ExchangeOrder eorder(order, 123);
    IdAllocator id_allocator;
    OrderBook order_book("SPY", id_allocator);
    OrderId order_id = order_book.add_order(order);

    std::cout << order << "\n";
    std::cout << "assigned order id: " << order_id << "\n";

    order_book.modify_order(order_id, 3.01, 100);
    order_book.modify_order(order_id, 3.01, 200);
    order_book.modify_order(order_id, 4.01, 300);
    order_book.cancel_order(order_id);

    std::cout << eorder << "\n";
    return 0;
}