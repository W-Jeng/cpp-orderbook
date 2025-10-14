#pragma once

#include <string>
#include <cstdint>
#include <new>

using Instrument = std::string;
using OrderId = uint64_t;
using Price = double;
using Quantity = uint64_t;

constexpr OrderId DEFAULT_ORDER_ID = 0; 
constexpr OrderId FIRST_ORDER_ID = 1; 
constexpr OrderId INVALID_ORDER_ID = UINT64_MAX; 

constexpr std::size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
