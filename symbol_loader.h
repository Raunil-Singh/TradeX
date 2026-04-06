#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "market_state.h"

struct SymbolInfo {
    std::string symbol;
    uint32_t symbol_id;
    uint64_t ltp;
    uint64_t lower_limit;
    uint64_t upper_limit;
};

std::vector<SymbolInfo> loadSymbolCSV(const std::string& filename);

void saveEndOfDayCSV(const std::string& filename, shared_data::MarketState* market_state, const std::vector<SymbolInfo>& symbols);